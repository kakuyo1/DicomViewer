#pragma once

#include <QListView>

class ThumbnailItemDelegate;
class ThumbnailListModel;
class QModelIndex;
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

private:
    ViewerSession *mViewerSession                  = nullptr;
    ThumbnailListModel *mThumbnailListModel        = nullptr;
    ThumbnailItemDelegate *mThumbnailItemDelegate  = nullptr;
    int mPendingSliceIndex                         = -1;        // 防止workSpace来切片更换信号时，model还没加载完
};
