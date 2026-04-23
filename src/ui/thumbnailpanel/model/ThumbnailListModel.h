#pragma once

#include <QAbstractListModel>

#include "ui/thumbnailpanel/model/ThumbnailItemData.h"

/**
 *  @note ThumbnailListModel 里存着 Ready 的 pixmap，
 *  重复滚动不会重算 => model 本身已经在充当缓存（不需要再设计一套缓存机制）
 *  缓存真正可能有用的场景是：切换 series 后再切回来 => 但是本轻量化项目暂不考虑
 */
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
    int appliedDisplayRevisionAt(int row) const;
    int loadingDisplayRevisionAt(int row) const;
    QString filePathAt(int row) const;

    void setItems(const QVector<ThumbnailItemData> &items);
    void clear();

    void invalidateAllThumbnails();
    void invalidateThumbnailRange(int firstRow, int lastRow);

    void markThumbnailLoading(int row, int displayRevision);
    void setThumbnailReady(int row, const QPixmap &pixmap, int displayRevision);
    void setThumbnailFailed(int row, int displayRevision);

private:
    bool isValidRow(int row) const;

private:
    QVector<ThumbnailItemData> mItems;
    int mGeneration = 0;                // generation机制：短时间内导入多个series的情况下，丢弃旧数据
};
