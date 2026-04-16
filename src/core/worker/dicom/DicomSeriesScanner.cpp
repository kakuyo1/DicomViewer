#include "core/worker/dicom/DicomSeriesScanner.h"

#include <algorithm>

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStringList>

#include "core/worker/dicom/DicomTagReader.h"

#include <spdlog/spdlog.h>

namespace
{

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

} // namespace

QVector<DicomSeries> DicomSeriesScanner::scanDirectory(const QString &directoryPath) const
{
    QVector<DicomSeries> seriesList;
    const QDir rootDir(directoryPath);
    if (!rootDir.exists()) {
        return seriesList;
    }

    DicomTagReader tagReader;
    QHash<QString, int> seriesIndexByUid;

    QDirIterator it(
        directoryPath,
        QDir::Files | QDir::NoSymLinks | QDir::Readable,
        QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString filePath = it.next();
        const auto sliceInfo = tagReader.readSliceInfo(filePath);
        if (!sliceInfo.has_value()) {
            continue;
        }

        /** @note 这个项目目前只支持CT */
        if (sliceInfo->modality.compare(QStringLiteral("CT"), Qt::CaseInsensitive) != 0) {
            continue;
        }

        const QString &seriesUid = sliceInfo->seriesInstanceUid;
        if (!seriesIndexByUid.contains(seriesUid)) {
            DicomSeries series;
            series.seriesInstanceUid = seriesUid;
            series.seriesDescription = sliceInfo->seriesDescription;
            series.modality          = sliceInfo->modality;
            seriesList.push_back(series);
            seriesIndexByUid.insert(seriesUid, seriesList.size() - 1);
        }

        DicomSeries &series = seriesList[seriesIndexByUid.value(seriesUid)];
        if (series.seriesDescription.isEmpty()) {
            series.seriesDescription = sliceInfo->seriesDescription;
        }
        if (series.modality.isEmpty()) {
            series.modality = sliceInfo->modality;
        }
        series.slices.push_back(*sliceInfo);
    }

    for (DicomSeries &series : seriesList) {
        /** @note 优先按InstanceNumber排序, 如果没有，按文件路径排序 */
        std::sort(series.slices.begin(), series.slices.end(), sliceLessThan);
        /** @note 找出所有切片的公共父目录 */
        series.pathSummary = buildPathSummary(series.slices);
    }

    std::sort(seriesList.begin(), seriesList.end(), seriesLessThan);
    return seriesList;
}
