#include "core/worker/volume/VolumeBuilder.h"
#include "common/Util.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <QRegularExpression>
#include <QStringList>

#include <dcmtk/dcmdata/dctk.h>

namespace
{

bool readStoredPixelValue(DcmDataset *dataset, const DcmTagKey &tagKey, Uint16 pixelRepresentation, Sint16 *outValue)
{
    if (dataset == nullptr || outValue == nullptr) {
        return false;
    }

    Uint16 unsignedValue = 0;
    if (dataset->findAndGetUint16(tagKey, unsignedValue).good()) {
        if (pixelRepresentation == 0) {
            *outValue = static_cast<Sint16>(unsignedValue);
        } else {
            std::memcpy(outValue, &unsignedValue, sizeof(Sint16));
        }
        return true;
    }

    Sint16 signedValue = 0;
    if (dataset->findAndGetSint16(tagKey, signedValue).good()) {
        *outValue = signedValue;
        return true;
    }

    return false;
}

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

    return defaultValue; /** @note 隐式转换Float64 -> double */
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

    bool         ok    = false;
    const double value = parts[componentIndex].toDouble(&ok);
    return ok ? value : defaultValue;
}

double dot(const DicomVector3 &lhs, const DicomVector3 &rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

DicomVector3 subtract(const DicomVector3 &lhs, const DicomVector3 &rhs)
{
    return DicomVector3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/** @note 根据几何信息推导 Z 方向间距，而不是直接用 SliceThickness */
bool tryDeriveSpacingZFromGeometry(const DicomSeries &series, double *spacingZ, QStringList *warnings)
{
    if (spacingZ == nullptr || series.slices.size() < 2 || !series.hasGeometryHints) {
        return false;
    }

    const DicomSliceInfo &firstSlice = series.slices.front();
    if (!firstSlice.hasImagePositionPatient || !firstSlice.hasSliceNormal) {
        return false;
    }

    const DicomVector3  normal = firstSlice.sliceNormal;
    std::vector<double> deltas;
    deltas.reserve(static_cast<std::size_t>(series.slices.size() - 1)); /** @note 层间距离候选值 */

    // 通过投影得到“沿切片法向的坐标”，解决图像未严格沿某个轴排列的问题
    double previousProjection = dot(firstSlice.imagePositionPatient, normal);
    for (int i = 1; i < series.slices.size(); ++i) {
        const DicomSliceInfo &slice = series.slices.at(i);
        if (!slice.hasImagePositionPatient) {
            return false;
        }

        const double currentProjection = dot(slice.imagePositionPatient, normal);
        const double delta             = std::abs(currentProjection - previousProjection);
        if (delta > 1e-6) { // 忽略极小值
            deltas.push_back(delta);
        }
        previousProjection = currentProjection;
    }

    if (deltas.empty()) {
        return false;
    }

    // 取中位数作为 spacingZ，不容易被单独几个异常的slice拉偏
    std::sort(deltas.begin(), deltas.end());
    const std::size_t middle = deltas.size() / 2;
    const double      median = (deltas.size() % 2 == 0)
                                   ? 0.5 * (deltas[middle - 1] + deltas[middle])
                                   : deltas[middle];

    if (median <= 0.0) {
        return false;
    }

    // 检测是否非均匀层距：如果某些 delta 和中位数相差较大，就发出警告
    if (warnings != nullptr) {
        for (double delta : deltas) {
            if (std::abs(delta - median) > std::max(1e-3, median * 0.05)) {
                warnings->push_back(QStringLiteral("Non-uniform slice spacing detected; spacingZ uses median projected distance."));
                break;
            }
        }
    }

    *spacingZ = median;
    return true;
}

} // namespace

std::optional<VolumeData> VolumeBuilder::build(
    const DicomSeries &series,
    QString           *errorMessage,
    QStringList       *warnings,
    QString           *buildSummary) const
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
    volumeData.modality          = series.modality;
    volumeData.depth             = series.slices.size();

    QStringList localWarnings;
    double      derivedSpacingZ    = 1.0;
    const bool  hasGeometrySpacing = tryDeriveSpacingZFromGeometry(series, &derivedSpacingZ, &localWarnings);

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

        if (!dataset->findAndGetUint16(DCM_Rows, rows).good() || !dataset->findAndGetUint16(DCM_Columns, columns).good() || !dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated).good() || !dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation).good()) {
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

        /** @note 根据第一张slice提前reserve */
        if (sliceIndex == 0) {
            volumeData.width            = static_cast<int>(columns);
            volumeData.height           = static_cast<int>(rows);
            volumeData.spacingX         = readSpacingComponent(dataset, 1, 1.0);
            volumeData.spacingY         = readSpacingComponent(dataset, 0, 1.0);
            volumeData.windowCenter     = readFloat64Value(dataset, DCM_WindowCenter, 0.0);
            volumeData.windowWidth      = readFloat64Value(dataset, DCM_WindowWidth, 0.0);
            volumeData.rescaleSlope     = readFloat64Value(dataset, DCM_RescaleSlope, 1.0);
            volumeData.rescaleIntercept = readFloat64Value(dataset, DCM_RescaleIntercept, 0.0);

            Sint16 rawPaddingValue = 0;
            if (readStoredPixelValue(dataset, DCM_PixelPaddingValue, pixelRepresentation, &rawPaddingValue)) {
                const double paddingHu          = static_cast<double>(rawPaddingValue) * volumeData.rescaleSlope + volumeData.rescaleIntercept;
                volumeData.hasPixelPaddingValue = true;
                volumeData.pixelPaddingValue    = static_cast<qint16>(std::lround(paddingHu));
            }

            Sint16 rawPaddingRangeLimit = 0;
            if (readStoredPixelValue(dataset, DCM_PixelPaddingRangeLimit, pixelRepresentation, &rawPaddingRangeLimit)) {
                const double paddingRangeHu          = static_cast<double>(rawPaddingRangeLimit) * volumeData.rescaleSlope + volumeData.rescaleIntercept;
                volumeData.hasPixelPaddingRangeLimit = true;
                volumeData.pixelPaddingRangeLimit    = static_cast<qint16>(std::lround(paddingRangeHu));
            }

            if (hasGeometrySpacing) { // 优先采用几何信息推导的spacingZ
                volumeData.spacingZ                     = derivedSpacingZ;
                volumeData.usedSliceThicknessAsSpacingZ = false;
                volumeData.spacingZSource               = QStringLiteral("ImagePositionPatientProjectedDelta");
            } else if (slice.hasSliceThickness) { // 回退到使用sliceThickness作为spacingZ
                volumeData.spacingZ                     = slice.sliceThickness;
                volumeData.usedSliceThicknessAsSpacingZ = true;
                volumeData.spacingZSource               = QStringLiteral("SliceThicknessFallback");
            } else { // 最糟糕的情况
                volumeData.spacingZ                     = 1.0;
                volumeData.usedSliceThicknessAsSpacingZ = false;
                volumeData.spacingZSource               = QStringLiteral("DefaultFallback");
                localWarnings.push_back(QStringLiteral("Could not derive spacingZ from geometry or SliceThickness; defaulted to 1.0."));
            }

            // 保存 VolumeData patient orientation
            const DicomSliceInfo &lastSlice = series.slices.back();
            if (series.hasGeometryHints && slice.hasImageOrientationPatient && slice.hasSliceNormal && slice.hasImagePositionPatient && lastSlice.hasImagePositionPatient) {
                const DicomVector3 sliceDirection = subtract(lastSlice.imagePositionPatient, slice.imagePositionPatient);
                volumeData.volumeXDirection       = slice.rowDirection;
                volumeData.volumeYDirection       = slice.columnDirection;
                volumeData.volumeZDirection       = (dot(sliceDirection, slice.sliceNormal) >= 0.0) ? slice.sliceNormal : util::reversedDirection(slice.sliceNormal);
                volumeData.hasPatientOrientation  = true;
            }

            expectedPixelCount = pixelCount;
            volumeData.voxels.reserve(volumeData.depth * pixelCount);

            if (!series.hasGeometryHints) {
                localWarnings.push_back(QStringLiteral("ImagePositionPatient/ImageOrientationPatient are incomplete or inconsistent; geometry sorting fell back to a temporary strategy."));
            }
            if (!slice.hasWindowCenter || !slice.hasWindowWidth) {
                localWarnings.push_back(QStringLiteral("Window center/width are incomplete; default values may be used."));
            }
            if (volumeData.hasPixelPaddingRangeLimit && !volumeData.hasPixelPaddingValue) {
                localWarnings.push_back(QStringLiteral("PixelPaddingRangeLimit is present without PixelPaddingValue; padding suppression is disabled."));
                volumeData.hasPixelPaddingRangeLimit = false;
            }
        } else if (volumeData.width != static_cast<int>(columns) || volumeData.height != static_cast<int>(rows) || expectedPixelCount != pixelCount) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Inconsistent slice dimensions in series.");
            }
            return std::nullopt;
        }

        /** @note HU = rawPixel * RescaleSlope + RescaleIntercept */
        const double rescaleSlope     = readFloat64Value(dataset, DCM_RescaleSlope, 1.0);
        const double rescaleIntercept = readFloat64Value(dataset, DCM_RescaleIntercept, 0.0);

        if (sliceIndex > 0 && (rescaleSlope != volumeData.rescaleSlope || rescaleIntercept != volumeData.rescaleIntercept)) {
            if (!localWarnings.contains(QStringLiteral("Per-slice rescale parameters differ; build summary uses values from the first slice."))) {
                localWarnings.push_back(QStringLiteral("Per-slice rescale parameters differ; build summary uses values from the first slice."));
            }
        }

        /**
         *  @note
         *  - 一律先按 Uint16 读 PixelData
         *  - 如果 PixelRepresentation == 0
         *      - 直接按无符号解释
         *  - 如果 PixelRepresentation == 1
         *      - 保留 16-bit 原始位模式
         *      - 再按 Sint16 解释
         */
        unsigned long valueCount = 0;
        const Uint16 *pixelData  = nullptr;
        if (!dataset->findAndGetUint16Array(DCM_PixelData, pixelData, &valueCount).good() || pixelData == nullptr || static_cast<int>(valueCount) < pixelCount) {
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
                Sint16       signedValue = 0;
                const Uint16 rawWord     = pixelData[i];
                std::memcpy(&signedValue, &rawWord, sizeof(Sint16));
                rawValue = static_cast<double>(signedValue);
            }

            const double huValue = rawValue * rescaleSlope + rescaleIntercept;
            volumeData.voxels.push_back(static_cast<qint16>(std::lround(huValue)));
        }
    }

    // 即使前面都成功了，最后仍然检查一下结果是否合法
    if (!volumeData.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Volume build completed with invalid output data.");
        }
        return std::nullopt;
    }

    volumeData.geometrySummary = QStringLiteral(
                                     "sort=%1; spacingZ source=%2; geometry hints=%3")
                                     .arg(
                                         series.sortStrategySummary.isEmpty()
                                             ? QStringLiteral("unknown")
                                             : series.sortStrategySummary,
                                         volumeData.spacingZSource.isEmpty()
                                             ? QStringLiteral("unknown")
                                             : volumeData.spacingZSource,
                                         series.hasGeometryHints ? QStringLiteral("available") : QStringLiteral("incomplete"));

    if (warnings != nullptr) {
        *warnings = localWarnings;
    }

    if (buildSummary != nullptr) {
        *buildSummary = QStringLiteral(
                            "%1 x %2 x %3, spacing=(%4, %5, %6), WW/WL=(%7, %8), rescale=(%9, %10)")
                            .arg(volumeData.width)
                            .arg(volumeData.height)
                            .arg(volumeData.depth)
                            .arg(volumeData.spacingX, 0, 'f', 3)
                            .arg(volumeData.spacingY, 0, 'f', 3)
                            .arg(volumeData.spacingZ, 0, 'f', 3)
                            .arg(volumeData.windowWidth, 0, 'f', 3)
                            .arg(volumeData.windowCenter, 0, 'f', 3)
                            .arg(volumeData.rescaleSlope, 0, 'f', 3)
                            .arg(volumeData.rescaleIntercept, 0, 'f', 3);
    }

    return volumeData;
}
