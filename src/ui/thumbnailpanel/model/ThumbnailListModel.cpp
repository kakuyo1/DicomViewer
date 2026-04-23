#include "ThumbnailListModel.h"

#include <algorithm>

#include <QColor>
#include <QPixmap>

namespace
{

QPixmap createPlaceholderPixmap()
{
    QPixmap placeholderPixmap(160, 112);
    placeholderPixmap.fill(QColor(48, 52, 60));
    return placeholderPixmap;
}

}

ThumbnailListModel::ThumbnailListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ThumbnailListModel::~ThumbnailListModel()
{
}

int ThumbnailListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return mItems.size();
}

QVariant ThumbnailListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= mItems.size()) {
        return {};
    }

    const ThumbnailItemData &item = mItems.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return QStringLiteral("Instance %1").arg(item.instanceNumber);
    case InstanceNumberRole:
        return item.instanceNumber;
    case SliceIndexRole:
        return item.sliceIndex;
    case ThumbnailStateRole:
        return static_cast<int>(item.state);
    case ThumbnailPixmapRole:
        return item.thumbnailPixmap;
    case FilePathRole:
        return item.filePath;
    default:
        return {};
    }
}

QHash<int, QByteArray> ThumbnailListModel::roleNames() const
{
    return {
        {InstanceNumberRole,  "instanceNumber"},
        {SliceIndexRole,      "sliceIndex"},
        {ThumbnailStateRole,  "thumbnailState"},
        {ThumbnailPixmapRole, "thumbnailPixmap"},
        {FilePathRole,        "filePath"},
    };
}

int ThumbnailListModel::itemCount() const
{
    return mItems.size();
}

int ThumbnailListModel::generation() const
{
    return mGeneration;
}

ThumbnailState ThumbnailListModel::thumbnailStateAt(int row) const
{
    if (!isValidRow(row)) {
        return ThumbnailState::Failed;
    }

    return mItems.at(row).state;
}

QString ThumbnailListModel::filePathAt(int row) const
{
    if (!isValidRow(row)) {
        return {};
    }

    return mItems.at(row).filePath;
}

void ThumbnailListModel::setItems(const QVector<ThumbnailItemData> &items)
{
    beginResetModel(); // 告诉所有附加的视图：模型即将失效，不要再依赖当前任何数据
    ++mGeneration;
    mItems = items;
    endResetModel();   // 告诉所有视图：模型已经重置完成，数据已更新
}

void ThumbnailListModel::clear()
{
    setItems({});
}

void ThumbnailListModel::invalidateAllThumbnails()
{
    if (mItems.isEmpty()) {
        ++mGeneration;
        return;
    }

    ++mGeneration;
    for (ThumbnailItemData &item : mItems) {
        item.state           = ThumbnailState::NotRequested;
        item.thumbnailPixmap = createPlaceholderPixmap();
    }

    const QModelIndex firstIndex = index(0, 0);
    const QModelIndex lastIndex  = index(mItems.size() - 1, 0);
    emit dataChanged(firstIndex, lastIndex, {ThumbnailStateRole, ThumbnailPixmapRole});
}

void ThumbnailListModel::invalidateThumbnailRange(int firstRow, int lastRow)
{
    if (mItems.isEmpty()) {
        return;
    }

    firstRow = std::max(0, firstRow);
    lastRow  = std::min(lastRow, static_cast<int>(mItems.size()) - 1);
    if (firstRow > lastRow) {
        return;
    }

    ++mGeneration;
    for (int row = firstRow; row <= lastRow; ++row) {
        ThumbnailItemData &item = mItems[row];
        item.state              = ThumbnailState::NotRequested;
        item.thumbnailPixmap    = createPlaceholderPixmap();
    }

    const QModelIndex firstIndex = index(firstRow, 0);
    const QModelIndex lastIndex  = index(lastRow, 0);
    emit dataChanged(firstIndex, lastIndex, {ThumbnailStateRole, ThumbnailPixmapRole});
}

void ThumbnailListModel::markThumbnailLoading(int row)
{
    if (!isValidRow(row)) {
        return;
    }

    ThumbnailItemData &item = mItems[row];
    if (item.state == ThumbnailState::Loading) {
        return;
    }

    item.state = ThumbnailState::Loading;
    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {ThumbnailStateRole});     // 通知视图重绘
}

void ThumbnailListModel::setThumbnailReady(int row, const QPixmap &pixmap)
{
    if (!isValidRow(row)) {
        return;
    }

    ThumbnailItemData &item = mItems[row];
    item.state              = ThumbnailState::Ready;
    item.thumbnailPixmap    = pixmap;

    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {ThumbnailStateRole, ThumbnailPixmapRole});    // 通知视图重绘
}

void ThumbnailListModel::setThumbnailFailed(int row)
{
    if (!isValidRow(row)) {
        return;
    }

    ThumbnailItemData &item = mItems[row];
    item.state = ThumbnailState::Failed;

    const QModelIndex modelIndex = index(row, 0);
    emit dataChanged(modelIndex, modelIndex, {ThumbnailStateRole});     // 通知视图重绘
}

bool ThumbnailListModel::isValidRow(int row) const
{
    return row >= 0 && row < mItems.size();
}
