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

constexpr int kPreloadRowCount = 4;

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
    setModel(mThumbnailListModel);
    setItemDelegate(mThumbnailItemDelegate);

    connect(this, &QListView::clicked, this, &ThumbnailPanel::handleItemClicked);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() {
        requestVisibleThumbnails();
    });
    connect(mThumbnailLoader, &ThumbnailLoader::thumbnailLoaded, this, &ThumbnailPanel::handleThumbnailLoaded);
    connect(mThumbnailLoader, &ThumbnailLoader::thumbnailFailed, this, &ThumbnailPanel::handleThumbnailFailed);
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

void ThumbnailPanel::refreshFromSession()
{
    if (mThumbnailListModel == nullptr || mViewerSession == nullptr) {
        clearItems();
        return;
    }

    const DicomSeries *currentSeries = mViewerSession->currentSeries();
    if (currentSeries == nullptr || currentSeries->slices.isEmpty()) {
        clearItems();
        return;
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
    if (mThumbnailListModel == nullptr || mThumbnailLoader == nullptr) {
        return;
    }

    const int itemCount = mThumbnailListModel->itemCount();
    if (itemCount <= 0) {
        return;
    }

    // 取视口中间的 x 坐标（x 不重要（随便取一列））
    const int sampleX = std::max(0, viewport()->width() / 2);
    // y 才是关键（上下） -> 0 ~ viewportHeight - 1
    const QModelIndex firstVisibleIndex = indexAt(QPoint(sampleX, 0));
    const QModelIndex lastVisibleIndex  = indexAt(QPoint(sampleX, viewport()->height() - 1));

    if (!firstVisibleIndex.isValid() && !lastVisibleIndex.isValid()) {
        requestThumbnailRange(0, std::min(itemCount - 1, kPreloadRowCount * 2));  // 直接加载前 N 个
        return;
    }

    // 如果拿不到 → 默认 0
    const int firstVisibleRow = firstVisibleIndex.isValid() ? firstVisibleIndex.row() : 0;
    // 如果拿不到 → 默认最后一行
    const int lastVisibleRow  = lastVisibleIndex.isValid() ? lastVisibleIndex.row() : (itemCount - 1);
    // 预加载
    const int requestFirstRow = std::max(0, firstVisibleRow - kPreloadRowCount);
    const int requestLastRow  = std::min(itemCount - 1, lastVisibleRow + kPreloadRowCount);

    requestThumbnailRange(requestFirstRow, requestLastRow);
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
        if (state == ThumbnailState::Ready || state == ThumbnailState::Loading) {
            continue;
        }

        const SliceImageBuildInput input = buildSliceImageInput(*currentVolumeData, row);
        if (!input.isValid()) {
            mThumbnailListModel->setThumbnailFailed(row);
            continue;
        }

        // 先把loading文字渲染上去
        mThumbnailListModel->markThumbnailLoading(row);
        // 再渲染缩略图
        mThumbnailLoader->requestThumbnail(row, input, requestGeneration, currentVolumeData->windowCenter, currentVolumeData->windowWidth);
    }
}

void ThumbnailPanel::handleThumbnailLoaded(int row, int generation, const QImage &image)
{
    // 根据 generation 丢掉旧数据
    if (mThumbnailListModel == nullptr || generation != mThumbnailListModel->generation()) {
        return;
    }

    const QPixmap pixmap = QPixmap::fromImage(image);
    if (pixmap.isNull()) {
        mThumbnailListModel->setThumbnailFailed(row);
        return;
    }

    mThumbnailListModel->setThumbnailReady(row, pixmap);
}

void ThumbnailPanel::handleThumbnailFailed(int row, int generation)
{
    if (mThumbnailListModel == nullptr || generation != mThumbnailListModel->generation()) {
        return;
    }

    mThumbnailListModel->setThumbnailFailed(row);
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
