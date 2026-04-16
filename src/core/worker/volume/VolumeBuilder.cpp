#include "core/worker/volume/VolumeBuilder.h"

#include <cmath>
#include <cstring>

#include <QRegularExpression>

#include <dcmtk/dcmdata/dctk.h>

#include <spdlog/spdlog.h>

namespace
{

QString readStringValue(DcmDataset *dataset, const DcmTagKey &tagKey)
{
    OFString value;
    if (dataset->findAndGetOFString(tagKey, value).good()) {
        return QString::fromUtf8(value.c_str()).trimmed();
    }

    return {};
}

double readFloat64Value(DcmDataset *dataset, const DcmTagKey &tagKey, double defaultValue)
{
    Float64 value = defaultValue;
    if (dataset->findAndGetFloat64(tagKey, value).good()) {
        return value;
    }

    return defaultValue;
}

double readSpacingComponent(DcmDataset *dataset, int componentIndex, double defaultValue)
{
    const QString pixelSpacing = readStringValue(dataset, DCM_PixelSpacing);
    if (pixelSpacing.isEmpty()) {
        return defaultValue;
    }

    const QStringList parts = pixelSpacing.split(QRegularExpression(QStringLiteral(R"([\\ ])")), Qt::SkipEmptyParts);
    if (componentIndex < 0 || componentIndex >= parts.size()) {
        return defaultValue;
    }

    bool ok = false;
    const double value = parts[componentIndex].toDouble(&ok);
    return ok ? value : defaultValue;
}

} // namespace

std::optional<VolumeData> VolumeBuilder::build(const DicomSeries &series, QString *errorMessage) const
{
    if (series.slices.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("The selected series does not contain any slices.");
        }
        return std::nullopt;
    }

    VolumeData volumeData;
    volumeData.seriesInstanceUid = series.seriesInstanceUid;
    volumeData.seriesDescription = series.seriesDescription;
    volumeData.modality = series.modality;
    volumeData.depth = series.slices.size();

    int expectedPixelCount = -1;

    for (int sliceIndex = 0; sliceIndex < series.slices.size(); ++sliceIndex) {
        const DicomSliceInfo &slice = series.slices.at(sliceIndex);

        DcmFileFormat fileFormat;
        if (!fileFormat.loadFile(slice.filePath.toUtf8().constData()).good()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to load slice file: %1").arg(slice.filePath);
            }
            return std::nullopt;
        }

        DcmDataset *dataset = fileFormat.getDataset();
        if (dataset == nullptr) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to read DICOM dataset: %1").arg(slice.filePath);
            }
            return std::nullopt;
        }

        Uint16 rows                = 0;
        Uint16 columns             = 0;
        Uint16 bitsAllocated       = 0;
        Uint16 pixelRepresentation = 0;

        if (!dataset->findAndGetUint16(DCM_Rows, rows).good()
            || !dataset->findAndGetUint16(DCM_Columns, columns).good()
            || !dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated).good()
            || !dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation).good()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Missing required pixel metadata: %1").arg(slice.filePath);
            }
            return std::nullopt;
        }

        if (bitsAllocated != 16) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Only 16-bit CT slices are supported for now: %1").arg(slice.filePath);
            }
            return std::nullopt;
        }

        const int pixelCount = static_cast<int>(rows) * static_cast<int>(columns);
        if (pixelCount <= 0) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Invalid slice dimensions: %1").arg(slice.filePath);
            }
            return std::nullopt;
        }

        if (sliceIndex == 0) {
            volumeData.width        = static_cast<int>(columns);
            volumeData.height       = static_cast<int>(rows);
            volumeData.spacingX     = readSpacingComponent(dataset, 1, 1.0);
            volumeData.spacingY     = readSpacingComponent(dataset, 0, 1.0);
            volumeData.spacingZ     = readFloat64Value(dataset, DCM_SliceThickness, 1.0);
            volumeData.windowCenter = readFloat64Value(dataset, DCM_WindowCenter  , 0.0);
            volumeData.windowWidth  = readFloat64Value(dataset, DCM_WindowWidth   , 0.0);
            expectedPixelCount      = pixelCount;
            volumeData.voxels.reserve(volumeData.depth * pixelCount);
        } else if (volumeData.width != static_cast<int>(columns)
                   || volumeData.height != static_cast<int>(rows)
                   || expectedPixelCount != pixelCount) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Inconsistent slice dimensions in series.");
            }
            return std::nullopt;
        }

        /** @note HU = rawPixel * RescaleSlope + RescaleIntercept */
        const double rescaleSlope     = readFloat64Value(dataset, DCM_RescaleSlope    , 1.0);
        const double rescaleIntercept = readFloat64Value(dataset, DCM_RescaleIntercept, 0.0);

        unsigned long valueCount = 0;
        const Uint16 *pixelData = nullptr;
        if (!dataset->findAndGetUint16Array(DCM_PixelData, pixelData, &valueCount).good() || pixelData == nullptr
            || static_cast<int>(valueCount) < pixelCount) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Failed to read 16-bit pixel data: %1").arg(slice.filePath);
            }
            return std::nullopt;
        }

        for (int i = 0; i < pixelCount; ++i) {
            double rawValue = 0.0;
            if (pixelRepresentation == 0) {
                rawValue = static_cast<double>(pixelData[i]);
            } else {
                Sint16 signedValue = 0;
                const Uint16 rawWord = pixelData[i];
                std::memcpy(&signedValue, &rawWord, sizeof(Sint16));
                rawValue = static_cast<double>(signedValue);
            }

            const double huValue = rawValue * rescaleSlope + rescaleIntercept;
            volumeData.voxels.push_back(static_cast<qint16>(std::lround(huValue)));
        }
    }

    if (!volumeData.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Volume build completed with invalid output data.");
        }
        return std::nullopt;
    }

    return volumeData;
}
