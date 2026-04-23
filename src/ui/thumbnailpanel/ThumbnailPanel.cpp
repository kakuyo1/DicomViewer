#include "ThumbnailPanel.h"

#include <QColor>
#include <QImage>
#include <QItemSelectionModel>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollBar>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QTimer>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"
#include "core/worker/SliceImageBuilder.h"
#include "services/state/ViewerSession.h"
#include "ui/thumbnailpanel/delegate/ThumbnailItemDelegate.h"
#include "ui/thumbnailpanel/loader/ThumbnailLoader.h"
#include "ui/thumbnailpanel/model/ThumbnailItemData.h"
#include "ui/thumbnailpanel/model/ThumbnailListModel.h"

namespace
{

constexpr int kPreloadRowCount        = 4;
constexpr int kWindowLevelSyncDelayMs = 150;

}

ThumbnailPanel::ThumbnailPanel(QWidget *parent)
    : QListView(parent)
{
    setupUi();
}

ThumbnailPanel::~ThumbnailPanel()
{

}

void ThumbnailPanel::setupUi()
{
    setObjectName(QStringLiteral("thumbnailPanel"));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(220);

    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(false);
    setSpacing(8);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);   // 平滑滚动
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeMode(QListView::Adjust);

    mThumbnailListModel    = new ThumbnailListModel(this);
    mThumbnailItemDelegate = new ThumbnailItemDelegate(this);
    mThumbnailLoader       = new ThumbnailLoader(this);

    mWindowLevelSyncTimer  = new QTimer(this);
    mWindowLevelSyncTimer->setSingleShot(true);

    setModel(mThumbnailListModel);
    setItemDelegate(mThumbnailItemDelegate);

    connect(this, &QListView::clicked, this, &ThumbnailPanel::handleItemClicked);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        requestVisibleThumbnails();
    });
    connect(mThumbnailLoader, &ThumbnailLoader::thumbnailLoaded, this, &ThumbnailPanel::handleThumbnailLoaded);
    connect(mThumbnailLoader, &ThumbnailLoader::thumbnailFailed, this, &ThumbnailPanel::handleThumbnailFailed);

    connect(mWindowLevelSyncTimer, &QTimer::timeout, this, &ThumbnailPanel::applyPendingWindowLevelToThumbnails);
}

void ThumbnailPanel::setViewerSession(ViewerSession *viewerSession)
{
    if (mViewerSession == viewerSession) {
        return;
    }

    if (mViewerSession != nullptr) {
        disconnect(mViewerSession, nullptr, this, nullptr);
    }

    mViewerSession = viewerSession;
    if (mViewerSession != nullptr) {
        connect(mViewerSession, &ViewerSession::sessionChanged, this, &ThumbnailPanel::refreshFromSession);
        connect(mViewerSession, &ViewerSession::sessionCleared, this, &ThumbnailPanel::clearItems);
    }

    refreshFromSession();
}

void ThumbnailPanel::setCurrentSliceIndex(int sliceIndex)
{
    mPendingSliceIndex = sliceIndex;

    if (mThumbnailListModel == nullptr || sliceIndex < 0) {
        clearSelection();
        return;
    }

    const QModelIndex modelIndex = mThumbnailListModel->index(sliceIndex, 0);
    if (!modelIndex.isValid()) {
        clearSelection();
        return;
    }

    if (selectionModel() != nullptr) {
        /**
         *  @note - 这里运用 RAII 的思想， 让blocker作用域结束自动释放。
         *        - blocker的作用是为了防止用户点击缩略图，主视图响应，
         *          主视图反过来通知 ThumbnailPanel，
         *          最终造成死循环！
         *        - 毕竟是这里是主视图更新切片让我（缩略图）滚动同步的，那我就临时禁止所有
         *          信号，静默同步。
         */
        const QSignalBlocker blocker(selectionModel());
        selectionModel()->setCurrentIndex(modelIndex, QItemSelectionModel::ClearAndSelect); // 设置选中
    }
    scrollTo(modelIndex, QAbstractItemView::PositionAtCenter);
}

/**
 *  @note
 *  主视图图像状态改变，缩略图如何跟进？
 *      - 对于 invert/flip 这种频率不高的功能 -> 立刻同步
 *      - 对于 WW/WL 这种频率高耗性能的功能    -> debounce 延迟同步
 *  同步的流程为两大关键步骤，采用 DisplayRevision 进行 pixmap 版本管理。
 *      - 主视图图像更新后     -> 立刻更新可视区域内的缩略图，其它区域暂时不管（希望用户不要动滚动条）
 *      - 用户滑动缩略图滚动条  -> 更新最新可视区域内的缩略图
 *  @footnote
 *  现在满足用户滚到新区域时，新的可视区能自动纠正。
 *  采用后台线程小批量提前补齐非可视区 pixmap 暂时不考虑。
 */
void ThumbnailPanel::setDisplayParameters(double windowCenter,
                                          double windowWidth,
                                          bool invert,
                                          bool flipHorizontal,
                                          bool flipVertical)
{
    if (mThumbnailListModel == nullptr || mThumbnailLoader == nullptr) {
        return;
    }

    const bool windowLevelChanged = (mWindowCenter != windowCenter || mWindowWidth != windowWidth);
    const bool orientationChanged = (mInvertEnabled != invert
                                     || mFlipHorizontalEnabled != flipHorizontal
                                     || mFlipVerticalEnabled != flipVertical);
    if (!windowLevelChanged && !orientationChanged) {
        return;
    }

    mPendingWindowCenter = windowCenter;
    mPendingWindowWidth  = windowWidth;

    if (orientationChanged) { // 1.立即刷新的情况（invert/flip）
        mWindowCenter          = windowCenter;
        mWindowWidth           = windowWidth;
        mInvertEnabled         = invert;
        mFlipHorizontalEnabled = flipHorizontal;
        mFlipVerticalEnabled   = flipVertical;
        ++mCurrentDisplayRevision;              // 图像状态变了 -> 更新 pixmap 版本
        mWindowLevelSyncTimer->stop();          // 防止之前的 ww/wl 延迟任务在未来突然触发，把现在的图覆盖掉
        refreshVisibleThumbnails(false, true);  // 只刷新当前可见，强制所有缩略图失效
        return;
    }

    // 防抖：如果用户在短时间内拖个不停，当前timer->start会一直重置，延迟applyPendingWindowLevelToThumbnails的触发
    mWindowLevelSyncTimer->start(kWindowLevelSyncDelayMs); // 2.延迟刷新的情况
}

void ThumbnailPanel::refreshFromSession()
{
    if (mThumbnailListModel == nullptr || mViewerSession == nullptr) {
        clearItems();
        return;
    }

    const DicomSeries *currentSeries = mViewerSession->currentSeries();
    const VolumeData  *currentVolumeData = mViewerSession->currentVolumeData();
    if (currentSeries == nullptr || currentSeries->slices.isEmpty()) {
        clearItems();
        return;
    }

    if (currentVolumeData != nullptr) { // 初始化各成员变量
        mWindowCenter           = currentVolumeData->windowCenter;
        mWindowWidth            = currentVolumeData->windowWidth;
        mInvertEnabled          = false;
        mFlipHorizontalEnabled  = false;
        mFlipVerticalEnabled    = false;
        mPendingWindowCenter    = mWindowCenter;
        mPendingWindowWidth     = mWindowWidth;
        mCurrentDisplayRevision = 0;
    }

    QVector<ThumbnailItemData> items;
    items.reserve(currentSeries->slices.size());

    // 为每一张 slice 构造一个 ThumbnailItemData
    for (int sliceIndex = 0; sliceIndex < currentSeries->slices.size(); ++sliceIndex) {
        const DicomSliceInfo &slice = currentSeries->slices.at(sliceIndex);

        ThumbnailItemData item;
        item.filePath        = slice.filePath;
        item.instanceNumber  = slice.hasInstanceNumber ? slice.instanceNumber : (sliceIndex + 1);
        item.sliceIndex      = sliceIndex;
        item.state           = ThumbnailState::NotRequested;

        QPixmap placeholderPixmap(160, 112);
        placeholderPixmap.fill(QColor(48, 52, 60));
        item.thumbnailPixmap = placeholderPixmap;       // 显示占位图（灰色块）

        items.push_back(item);
    }

    mThumbnailListModel->setItems(items);  // 通知：数据变了，重新绘制！

    // 以防setCurrentSliceIndex时，model还未加载完
    // 第一次导入thumbnail会受到两个信号：sessionChanged -> refreshFromSession（来自viewSession） |
    // currentStackSliceChanged ->setCurrentSliceIndex（来自stackPage）, 如果后者先到则 model 可能还未准备好导致缩略图无法同步主窗口
    if (mPendingSliceIndex >= 0) {
        setCurrentSliceIndex(mPendingSliceIndex);
    }

    QTimer::singleShot(0, this, [this]() {
        requestVisibleThumbnails();
    });
}

void ThumbnailPanel::clearItems()
{
    if (mThumbnailListModel != nullptr) {
        mThumbnailListModel->clear();
    }
    if (mWindowLevelSyncTimer != nullptr) {
        mWindowLevelSyncTimer->stop();
    }
    mPendingSliceIndex = -1;
    clearSelection();
}

void ThumbnailPanel::handleItemClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    emit sliceActivated(index.data(ThumbnailListModel::SliceIndexRole).toInt());
}

void ThumbnailPanel::requestVisibleThumbnails()
{
    refreshVisibleThumbnails(true, false);
}

// includePreload设置false表示只刷新可见区域，kPreloadRowCount作废，适合缩略图同步主视图图像状态
bool ThumbnailPanel::calculateVisibleRowRange(int *firstRow, int *lastRow, bool includePreload) const
{
    if (mThumbnailListModel == nullptr || mThumbnailLoader == nullptr) {
        return false;
    }

    const int itemCount = mThumbnailListModel->itemCount();
    if (itemCount <= 0) {
        return false;
    }

    // 取视口中间的 x 坐标（x 不重要（随便取一列））
    const int sampleX = std::max(0, viewport()->width() / 2);
    // y 才是关键（上下） -> 0 ~ viewportHeight - 1
    const QModelIndex firstVisibleIndex = indexAt(QPoint(sampleX, 0));
    const QModelIndex lastVisibleIndex  = indexAt(QPoint(sampleX, viewport()->height() - 1));

    if (!firstVisibleIndex.isValid() && !lastVisibleIndex.isValid()) {
        *firstRow = 0;
        *lastRow  = includePreload ? std::min(itemCount - 1, kPreloadRowCount * 2) : 0;
        return true;
    }

    // 如果拿不到 → 默认 0
    const int firstVisibleRow = firstVisibleIndex.isValid() ? firstVisibleIndex.row() : 0;
    // 如果拿不到 → 默认最后一行
    const int lastVisibleRow  = lastVisibleIndex.isValid() ? lastVisibleIndex.row() : (itemCount - 1);
    // 预加载
    *firstRow = includePreload ? std::max(0, firstVisibleRow - kPreloadRowCount) : firstVisibleRow;
    *lastRow  = includePreload ? std::min(itemCount - 1, lastVisibleRow + kPreloadRowCount) : lastVisibleRow;

    return true;
}

void ThumbnailPanel::refreshVisibleThumbnails(bool includePreload, bool invalidateRange)
{
    int firstRow = 0;
    int lastRow  = -1;
    if (!calculateVisibleRowRange(&firstRow, &lastRow, includePreload)) {
        return;
    }

    if (invalidateRange && mThumbnailListModel != nullptr) {
        mThumbnailListModel->invalidateThumbnailRange(firstRow, lastRow); // UI 立即变灰（占位图），旧任务全部作废（generation）
    }
    // 重新请求缩略图
    requestThumbnailRange(firstRow, lastRow);
}

void ThumbnailPanel::applyPendingWindowLevelToThumbnails()
{
    // 判断是否真的变了，若没变，直接跳过
    if (mWindowCenter == mPendingWindowCenter && mWindowWidth == mPendingWindowWidth) {
        return;
    }

    mWindowCenter = mPendingWindowCenter;
    mWindowWidth  = mPendingWindowWidth;
    ++mCurrentDisplayRevision;              // 主视图图像状态变了 -> debounce 后更新一下缩略图的 pixmap 版本
    refreshVisibleThumbnails(false, true);  // 只刷新当前可见，强制所有缩略图失效
}

void ThumbnailPanel::requestThumbnailRange(int firstRow, int lastRow)
{
    if (mThumbnailListModel == nullptr || mThumbnailLoader == nullptr) {
        return;
    }

    const VolumeData *currentVolumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    if (currentVolumeData == nullptr || !currentVolumeData->isValid()) {
        return;
    }

    const int requestGeneration = mThumbnailListModel->generation();
    for (int row = firstRow; row <= lastRow; ++row) {
        const ThumbnailState state = mThumbnailListModel->thumbnailStateAt(row);
        const int appliedDisplayRevision = mThumbnailListModel->appliedDisplayRevisionAt(row);
        const int loadingDisplayRevision = mThumbnailListModel->loadingDisplayRevisionAt(row);
        /**
         *  @note 应该渲染的最新 pixmap 的两个条件：状态 Ready + Revision 最新
         *        - `Ready   + appliedDisplayRevision == currentRevision` => 最新
         *        - `Ready   + appliedDisplayRevision != currentRevision` => 有图，但已过期
         *        - `Loading + loadingDisplayRevision == currentRevision` => 正在加载当前版本
         *        - `Loading + loadingDisplayRevision != currentRevision` => 正在跑旧任务，应视为过期
         */
        if (state == ThumbnailState::Ready   && appliedDisplayRevision == mCurrentDisplayRevision) {
            continue;
        }
        if (state == ThumbnailState::Loading && loadingDisplayRevision == mCurrentDisplayRevision) {
            continue;
        }

        // 先把loading文字渲染上去，然后标记该缩略图的 pixmap 版本为最新的 DisplayVersion, 旧图如果存在则继续保留，直到新图完成
        mThumbnailListModel->markThumbnailLoading(row, mCurrentDisplayRevision);

        const SliceImageBuildInput input = buildSliceImageInput(*currentVolumeData, row);
        if (!input.isValid()) {
            mThumbnailListModel->setThumbnailFailed(row, mCurrentDisplayRevision);
            continue;
        }

        // 再渲染缩略图
        mThumbnailLoader->requestThumbnail(row,
                                           input,
                                           requestGeneration,
                                           mCurrentDisplayRevision, // ！标记该缩略图的 pixmap 版本为最新的 DisplayVersion
                                           mWindowCenter,
                                           mWindowWidth,
                                           mInvertEnabled,
                                           mFlipHorizontalEnabled,
                                           mFlipVerticalEnabled);
    }
}

void ThumbnailPanel::handleThumbnailLoaded(int row, int generation, int displayRevision, const QImage &image)
{
    // 根据 generation     丢掉旧数据（面向series）
    // 根据 displayRevison 丢掉旧数据（面向主/缩同步）
    if (mThumbnailListModel == nullptr || generation != mThumbnailListModel->generation() || displayRevision != mCurrentDisplayRevision) {
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(image);
    if (pixmap.isNull()) {
        mThumbnailListModel->setThumbnailFailed(row, displayRevision);
        return;
    }

    mThumbnailListModel->setThumbnailReady(row, pixmap, displayRevision);
}

void ThumbnailPanel::handleThumbnailFailed(int row, int generation, int displayRevision)
{
    if (mThumbnailListModel == nullptr || generation != mThumbnailListModel->generation() || displayRevision != mCurrentDisplayRevision) {
        return;
    }

    mThumbnailListModel->setThumbnailFailed(row, displayRevision);
}

void ThumbnailPanel::resizeEvent(QResizeEvent *event)
{
    QListView::resizeEvent(event);
    requestVisibleThumbnails();
}

void ThumbnailPanel::showEvent(QShowEvent *event)
{
    QListView::showEvent(event);

    QTimer::singleShot(0, this, [this]() {
        requestVisibleThumbnails();
    });
}
