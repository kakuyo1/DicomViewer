#pragma once

#include <QListView>

class ThumbnailItemDelegate;
class ThumbnailLoader;
class ThumbnailListModel;
class QImage;
class QModelIndex;
class QResizeEvent;
class QShowEvent;
class QTimer;
class ViewerSession;

class ThumbnailPanel : public QListView
{
    Q_OBJECT

public:
    explicit ThumbnailPanel(QWidget *parent = nullptr);
    ~ThumbnailPanel();

    void setViewerSession(ViewerSession *viewerSession);
    void setCurrentSliceIndex(int sliceIndex);
    void setDisplayParameters(double windowCenter,
                              double windowWidth,
                              bool invert,
                              bool flipHorizontal,
                              bool flipVertical);

signals:
    void sliceActivated(int sliceIndex);

private:
    void setupUi();
    void refreshFromSession();
    void clearItems();
    void handleItemClicked(const QModelIndex &index);
    void requestVisibleThumbnails();
    void requestThumbnailRange(int firstRow, int lastRow);
    bool calculateVisibleRowRange(int *firstRow, int *lastRow, bool includePreload) const;
    void refreshVisibleThumbnails(bool includePreload, bool invalidateRange);
    void applyPendingWindowLevelToThumbnails();
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
    QTimer *mWindowLevelSyncTimer                  = nullptr;
    int mPendingSliceIndex                         = -1;        // 防止workSpace来切片更换信号时，model还没加载完

    // 下面的参数用于跟进主视图图像参数
    double mWindowCenter                           = 0.0;       // 当前生效
    double mWindowWidth                            = 0.0;
    bool mInvertEnabled                            = false;
    bool mFlipHorizontalEnabled                    = false;
    bool mFlipVerticalEnabled                      = false;
    double mPendingWindowCenter                    = 0.0;       // 用户正在拖
    double mPendingWindowWidth                     = 0.0;
};
