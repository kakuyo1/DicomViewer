#include "ThumbnailListModel.h"

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

void ThumbnailListModel::setItems(const QVector<ThumbnailItemData> &items)
{
    beginResetModel(); // 告诉所有附加的视图：模型即将失效，不要再依赖当前任何数据
    mItems = items;
    endResetModel();   // 告诉所有视图：模型已经重置完成，数据已更新
}

void ThumbnailListModel::clear()
{
    setItems({});
}
