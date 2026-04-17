#include "core/worker/dicom/DicomTagReader.h"

#include <array>
#include <cmath>

#include <QStringList>

#include <dcmtk/dcmdata/dctk.h>

namespace
{

QString readStringValue(DcmDataset *dataset, const DcmTagKey &tagKey)
{
    OFString value;
    /** @note 这里用的是FAG的Array系列，String版默认只读一个值，导致IPP/IOP等多值Tag读取出错 */
    if (dataset->findAndGetOFStringArray(tagKey, value).good()) {
        return QString::fromUtf8(value.c_str()).trimmed();
    }

    return {};
}

bool readFloatValue(DcmDataset *dataset, const DcmTagKey &tagKey, double *outValue)
{
    if (outValue == nullptr) {
        return false;
    }

    Float64 value = 0.0;
    if (dataset->findAndGetFloat64(tagKey, value).good()) {
        *outValue = value;
        return true;
    }

    const QString stringValue = readStringValue(dataset, tagKey);
    if (stringValue.isEmpty()) {
        return false;
    }

    bool ok = false;
    const double parsedValue = stringValue.toDouble(&ok);
    if (!ok) {
        return false;
    }

    *outValue = parsedValue;
    return true;
}

double dot(const DicomVector3 &lhs, const DicomVector3 &rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

DicomVector3 cross(const DicomVector3 &lhs, const DicomVector3 &rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x,
    };
}

/** @note 计算向量长度 */
double norm(const DicomVector3 &vector)
{
    return std::sqrt(dot(vector, vector));
}

bool normalizeInPlace(DicomVector3 *vector)
{
    if (vector == nullptr) {
        return false;
    }

    const double length = norm(*vector);
    if (length <= 1e-8) {
        return false;
    }

    vector->x /= length;
    vector->y /= length;
    vector->z /= length;
    return true;
}

/** @note 把 DICOM 标签的字符串形式（例如 x\y\z）解析成三维向量 */
bool parseVector3(const QString &rawValue, DicomVector3 *outVector)
{
    if (outVector == nullptr || rawValue.isEmpty()) {
        return false;
    }

    const QStringList parts = rawValue.split('\\', Qt::SkipEmptyParts);
    if (parts.size() != 3) {
        return false;
    }

    bool okX = false;
    bool okY = false;
    bool okZ = false;
    const double x = parts[0].trimmed().toDouble(&okX);
    const double y = parts[1].trimmed().toDouble(&okY);
    const double z = parts[2].trimmed().toDouble(&okZ);
    if (!okX || !okY || !okZ) {
        return false;
    }

    *outVector = DicomVector3{x, y, z};
    return true;
}

/** @note 解析 DICOM 的图像方向标签（6 个浮点数） */
bool parseOrientation(const QString &rawValue, DicomVector3 *rowDirection, DicomVector3 *columnDirection)
{
    if (rowDirection == nullptr || columnDirection == nullptr || rawValue.isEmpty()) {
        return false;
    }

    const QStringList parts = rawValue.split('\\', Qt::SkipEmptyParts);
    if (parts.size() != 6) {
        return false;
    }

    std::array<double, 6> values = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    for (int i = 0; i < 6; ++i) {
        bool ok = false;
        values[static_cast<std::size_t>(i)] = parts[i].trimmed().toDouble(&ok);
        if (!ok) {
            return false;
        }
    }

    *rowDirection    = DicomVector3{values[0], values[1], values[2]};
    *columnDirection = DicomVector3{values[3], values[4], values[5]};
    return normalizeInPlace(rowDirection) && normalizeInPlace(columnDirection);
}

} // namespace

DicomReadResult DicomTagReader::readSliceInfo(const QString &filePath) const
{
    DicomReadResult result;

    DcmFileFormat fileFormat;
    if (!fileFormat.loadFile(filePath.toUtf8().constData()).good()) {
        result.errorMessage = QStringLiteral("Failed to load file.");
        return result;
    }

    result.readableDicom = true;

    DcmDataset *dataset = fileFormat.getDataset();
    if (dataset == nullptr) {
        result.errorMessage = QStringLiteral("Failed to access DICOM dataset.");
        return result;
    }

    // Get Tags
    DicomSliceInfo sliceInfo;
    sliceInfo.filePath                   = filePath;
    sliceInfo.seriesInstanceUid          = readStringValue(dataset, DCM_SeriesInstanceUID);
    sliceInfo.sopInstanceUid             = readStringValue(dataset, DCM_SOPInstanceUID);
    sliceInfo.seriesDescription          = readStringValue(dataset, DCM_SeriesDescription);
    sliceInfo.modality                   = readStringValue(dataset, DCM_Modality).toUpper();

    sliceInfo.hasImagePositionPatient    = parseVector3(readStringValue(dataset, DCM_ImagePositionPatient), &sliceInfo.imagePositionPatient);
    sliceInfo.hasImageOrientationPatient = parseOrientation(
        readStringValue(dataset, DCM_ImageOrientationPatient),
        &sliceInfo.rowDirection,
        &sliceInfo.columnDirection);

    if (sliceInfo.hasImageOrientationPatient) {
        sliceInfo.sliceNormal    = cross(sliceInfo.rowDirection, sliceInfo.columnDirection);
        sliceInfo.hasSliceNormal = normalizeInPlace(&sliceInfo.sliceNormal);
    }

    if (sliceInfo.seriesInstanceUid.isEmpty()) {
        result.missingSeriesInstanceUid = true;
        result.errorMessage = QStringLiteral("Missing SeriesInstanceUID.");
        return result;
    }

    Sint32 instanceNumber = 0;
    if (dataset->findAndGetSint32(DCM_InstanceNumber, instanceNumber).good()) {
        sliceInfo.instanceNumber    = static_cast<int>(instanceNumber);
        sliceInfo.hasInstanceNumber = true;
    }

    sliceInfo.hasSliceThickness = readFloatValue(dataset, DCM_SliceThickness, &sliceInfo.sliceThickness);
    sliceInfo.hasWindowCenter   = readFloatValue(dataset, DCM_WindowCenter  , &sliceInfo.windowCenter);
    sliceInfo.hasWindowWidth    = readFloatValue(dataset, DCM_WindowWidth   , &sliceInfo.windowWidth);

    result.success   = true;
    result.sliceInfo = sliceInfo;
    return result;
}
