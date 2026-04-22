#pragma once

#include <QListView>

class ThumbnailItemDelegate;
class ThumbnailLoader;
class ThumbnailListModel;
class QImage;
class QModelIndex;
class QResizeEvent;
class QShowEvent;
class ViewerSession;

class ThumbnailPanel : public QListView
{
    Q_OBJECT

public:
    explicit ThumbnailPanel(QWidget *parent = nullptr);
    ~ThumbnailPanel();

    void setViewerSession(ViewerSession *viewerSession);
    void setCurrentSliceIndex(int sliceIndex);

signals:
    void sliceActivated(int sliceIndex);

private:
    void setupUi();
    void refreshFromSession();
    void clearItems();
    void handleItemClicked(const QModelIndex &index);
    void requestVisibleThumbnails();
    void requestThumbnailRange(int firstRow, int lastRow);
    void handleThumbnailLoaded(int row, int generation, const QImage &image);
    void handleThumbnailFailed(int row, int generation);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    ViewerSession *mViewerSession                  = nullptr;
    ThumbnailListModel *mThumbnailListModel        = nullptr;
    ThumbnailItemDelegate *mThumbnailItemDelegate  = nullptr;
    ThumbnailLoader *mThumbnailLoader              = nullptr;
    int mPendingSliceIndex                         = -1;        // 防止workSpace来切片更换信号时，model还没加载完
};
