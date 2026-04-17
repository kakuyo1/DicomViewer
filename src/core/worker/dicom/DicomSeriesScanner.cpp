#include "core/worker/dicom/DicomSeriesScanner.h"

#include <algorithm>
#include <cmath>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QStringList>

#include "core/worker/dicom/DicomTagReader.h"

namespace
{

/** @note
 *  在比较 DICOM 几何信息时，不能直接用 == 判断浮点数，因为：
 *      - 浮点数有精度误差
 *      - 不同设备生成的方向向量可能有很小偏差
 */
constexpr double kGeometryTolerance = 1e-4;

QString buildPathSummary(const QVector<DicomSliceInfo> &slices)
{
    if (slices.isEmpty()) {
        return {};
    }

    QStringList commonParts = QFileInfo(slices.front().filePath).absolutePath().split('/', Qt::SkipEmptyParts);

    for (int i = 1; i < slices.size() && !commonParts.isEmpty(); ++i) {
        const QStringList currentParts = QFileInfo(slices[i].filePath).absolutePath().split('/', Qt::SkipEmptyParts);
        int matchedCount = 0;
        while (matchedCount < commonParts.size() && matchedCount < currentParts.size()
               && commonParts[matchedCount] == currentParts[matchedCount]) {
            ++matchedCount;
        }
        commonParts = commonParts.mid(0, matchedCount);
    }

    if (commonParts.isEmpty()) {
        return QFileInfo(slices.front().filePath).absolutePath();
    }

    return QStringLiteral("/") + commonParts.join('/');
}

double dot(const DicomVector3 &lhs, const DicomVector3 &rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/** @note 逐分量比较两个向量是否“足够接近” */
bool vectorsClose(const DicomVector3 &lhs, const DicomVector3 &rhs, double tolerance)
{
    return std::abs(lhs.x - rhs.x) <= tolerance
        && std::abs(lhs.y - rhs.y) <= tolerance
        && std::abs(lhs.z - rhs.z) <= tolerance;
}

/** @note 定义“切片排序规则（回退排序）”,它在不能使用几何排序时作为备用方案。 */
bool sliceLessThan(const DicomSliceInfo &lhs, const DicomSliceInfo &rhs)
{
    if (lhs.hasInstanceNumber && rhs.hasInstanceNumber && lhs.instanceNumber != rhs.instanceNumber) {
        return lhs.instanceNumber < rhs.instanceNumber;
    }

    if (lhs.hasInstanceNumber != rhs.hasInstanceNumber) {
        return lhs.hasInstanceNumber;
    }

    return lhs.filePath < rhs.filePath;
}

bool seriesLessThan(const DicomSeries &lhs, const DicomSeries &rhs)
{
    const QString lhsDescription = lhs.seriesDescription.isEmpty() ? lhs.seriesInstanceUid : lhs.seriesDescription;
    const QString rhsDescription = rhs.seriesDescription.isEmpty() ? rhs.seriesInstanceUid : rhs.seriesDescription;
    return lhsDescription.toLower() < rhsDescription.toLower();
}

bool canUseGeometrySorting(const DicomSeries &series, QString *reason)
{
    if (series.slices.isEmpty()) {
        if (reason != nullptr) {
            *reason = QStringLiteral("Series has no slices.");
        }
        return false;
    }

    const DicomSliceInfo &firstSlice = series.slices.front();
    if (!firstSlice.hasImagePositionPatient || !firstSlice.hasImageOrientationPatient || !firstSlice.hasSliceNormal) {
        if (reason != nullptr) {
            *reason = QStringLiteral("Missing ImagePositionPatient/ImageOrientationPatient on the first slice.");
        }
        return false;
    }

    for (const DicomSliceInfo &slice : series.slices) {
        if (!slice.hasImagePositionPatient || !slice.hasImageOrientationPatient || !slice.hasSliceNormal) {
            if (reason != nullptr) {
                *reason = QStringLiteral("Some slices are missing parsable ImagePositionPatient/ImageOrientationPatient.");
            }
            return false;
        }
        /** @note 第三条判断是检测法向量方向一致，两个单位向量点积：
          *             - 接近  1 ：同方向
          *             - 接近 -1 ：反方向
          *             - 接近  0 ：垂直
          *             - 这里比较宽容，使用abs表示同/反方向都表示一致
          */
        if (!vectorsClose(slice.rowDirection, firstSlice.rowDirection, kGeometryTolerance)
            || !vectorsClose(slice.columnDirection, firstSlice.columnDirection, kGeometryTolerance)
            || std::abs(dot(slice.sliceNormal, firstSlice.sliceNormal)) < 1.0 - kGeometryTolerance) {
            if (reason != nullptr) {
                *reason = QStringLiteral("Slice orientations are inconsistent inside the series.");
            }
            return false;
        }
    }
    // 历经多次检查，终于可以用几何排序了！
    return true;
}

bool anySliceHasInstanceNumber(const DicomSeries &series)
{
    for (const DicomSliceInfo &slice : series.slices) {
        if (slice.hasInstanceNumber) {
            return true;
        }
    }

    return false;
}

void sortSeriesSlices(DicomSeries *series, QStringList *warnings)
{
    if (series == nullptr || series->slices.isEmpty()) {
        return;
    }

    QString geometryFailureReason;
    if (canUseGeometrySorting(*series, &geometryFailureReason)) {
        // 1.几何排序
        /** @note 每张切片都有一个三维位置 ImagePositionPatient。
          * 如果把它投影到法向量 normal 上：
          *     - projection = IPP⋅normal
          * 就得到该切片沿“层叠方向”的标量位置。
          * 然后按这个值从小到大排序，就能得到切片顺序。
          */
        const DicomVector3 normal = series->slices.front().sliceNormal;
        std::sort(series->slices.begin(), series->slices.end(), [&normal](const DicomSliceInfo &lhs, const DicomSliceInfo &rhs) {
            return dot(lhs.imagePositionPatient, normal) < dot(rhs.imagePositionPatient, normal);
        });
        series->hasGeometryHints = true;
        series->sortStrategySummary = QStringLiteral("Geometry(IPP+IOP projection)");
        return;
    }

    // 2.回退到instanceNumber排序
    std::sort(series->slices.begin(), series->slices.end(), sliceLessThan);
    series->hasGeometryHints = false;
    series->sortStrategySummary = anySliceHasInstanceNumber(*series)
        ? QStringLiteral("InstanceNumber fallback")
        : QStringLiteral("File path fallback");

    if (warnings != nullptr && !geometryFailureReason.isEmpty()) {
        const QString description = series->seriesDescription.isEmpty()
            ? series->seriesInstanceUid
            : series->seriesDescription;
        warnings->push_back(
            QStringLiteral("Series '%1' fell back from geometry sorting: %2")
                .arg(description, geometryFailureReason));
    }
}

void finalizeWarnings(SeriesScanResult *scanResult)
{
    if (scanResult == nullptr) {
        return;
    }

    if (scanResult->skippedUnreadableFileCount > 0) {
        scanResult->warnings.push_back(
            QStringLiteral("Skipped %1 unreadable files.").arg(scanResult->skippedUnreadableFileCount));
    }
    if (scanResult->skippedMissingSeriesUidCount > 0) {
        scanResult->warnings.push_back(
            QStringLiteral("Skipped %1 DICOM files missing SeriesInstanceUID.").arg(scanResult->skippedMissingSeriesUidCount));
    }
    if (scanResult->skippedNonCtCount > 0) {
        scanResult->warnings.push_back(
            QStringLiteral("Skipped %1 non-CT DICOM files.").arg(scanResult->skippedNonCtCount));
    }
}

} // namespace

SeriesScanResult DicomSeriesScanner::scanDirectory(const QString &directoryPath) const
{
    SeriesScanResult scanResult;
    scanResult.directoryPath = directoryPath;

    QVector<DicomSeries> &seriesList = scanResult.seriesList;
    const QDir rootDir(directoryPath);
    if (!rootDir.exists()) {
        scanResult.warnings.push_back(QStringLiteral("Selected directory does not exist."));
        return scanResult;
    }

    DicomTagReader tagReader;
    QHash<QString, int> seriesIndexByUid;

    QDirIterator it(
        directoryPath,
        QDir::Files | QDir::NoSymLinks | QDir::Readable,
        QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString filePath = it.next();
        ++scanResult.scannedFileCount;

        const DicomReadResult readResult = tagReader.readSliceInfo(filePath);
        if (!readResult.readableDicom) {
            ++scanResult.skippedUnreadableFileCount;
            continue;
        }

        ++scanResult.readableDicomCount;
        if (!readResult.success) {
            if (readResult.missingSeriesInstanceUid) {
                ++scanResult.skippedMissingSeriesUidCount;
            } else {
                ++scanResult.skippedUnreadableFileCount;
            }
            continue;
        }

        const DicomSliceInfo &sliceInfo = readResult.sliceInfo;

        /** @note 这个项目目前只支持CT */
        if (sliceInfo.modality.compare(QStringLiteral("CT"), Qt::CaseInsensitive) != 0) {
            ++scanResult.skippedNonCtCount;
            continue;
        }

        ++scanResult.acceptedSliceCount;

        const QString &seriesUid = sliceInfo.seriesInstanceUid;
        if (!seriesIndexByUid.contains(seriesUid)) { // 尚未出现过该series，先注册到Hash
            DicomSeries series;
            series.seriesInstanceUid = seriesUid;
            series.seriesDescription = sliceInfo.seriesDescription;
            series.modality          = sliceInfo.modality;
            seriesList.push_back(series);
            seriesIndexByUid.insert(seriesUid, seriesList.size() - 1);
        }

        // 该series已经出现过，利用Hash快速查找
        DicomSeries &series = seriesList[seriesIndexByUid.value(seriesUid)];
        if (series.seriesDescription.isEmpty()) {
            series.seriesDescription = sliceInfo.seriesDescription;
        }
        if (series.modality.isEmpty()) {
            series.modality = sliceInfo.modality;
        }
        series.slices.push_back(sliceInfo);
    }

    for (DicomSeries &series : seriesList) {
        sortSeriesSlices(&series, &scanResult.warnings);
        /** @note 找出所有切片的公共父目录 */
        series.pathSummary = buildPathSummary(series.slices);
    }

    std::sort(seriesList.begin(), seriesList.end(), seriesLessThan);
    scanResult.seriesCount = seriesList.size();
    finalizeWarnings(&scanResult);
    return scanResult;
}
