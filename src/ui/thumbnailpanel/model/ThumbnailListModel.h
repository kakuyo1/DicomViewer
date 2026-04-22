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

    int itemCount() const;
    int generation() const;
    ThumbnailState thumbnailStateAt(int row) const;
    QString filePathAt(int row) const;

    void setItems(const QVector<ThumbnailItemData> &items);
    void clear();
    void markThumbnailLoading(int row);
    void setThumbnailReady(int row, const QPixmap &pixmap);
    void setThumbnailFailed(int row);

private:
    bool isValidRow(int row) const;

private:
    QVector<ThumbnailItemData> mItems;
    int mGeneration = 0;                // generation机制：短时间内导入多个series的情况下，丢弃旧数据
};
