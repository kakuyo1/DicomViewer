#pragma once

#include <QAbstractListModel>

#include "ui/thumbnailpanel/model/ThumbnailItemData.h"

class ThumbnailListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ThumbnailRoles
    {
        InstanceNumberRole = Qt::UserRole + 1,
        SliceIndexRole,
        ThumbnailStateRole,
        ThumbnailPixmapRole,
        FilePathRole,
    };

public:
    explicit ThumbnailListModel(QObject *parent = nullptr);
    ~ThumbnailListModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QVector<ThumbnailItemData> &items);
    void clear();

private:
    QVector<ThumbnailItemData> mItems;
};
