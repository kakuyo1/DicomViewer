#include "ThumbnailPanel.h"

#include <QColor>
#include <QItemSelectionModel>
#include <QPixmap>
#include <QSignalBlocker>

#include "core/model/dicom/DicomSeries.h"
#include "services/state/ViewerSession.h"
#include "ui/thumbnailpanel/delegate/ThumbnailItemDelegate.h"
#include "ui/thumbnailpanel/model/ThumbnailItemData.h"
#include "ui/thumbnailpanel/model/ThumbnailListModel.h"

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
    setModel(mThumbnailListModel);
    setItemDelegate(mThumbnailItemDelegate);

    connect(this, &QListView::clicked, this, &ThumbnailPanel::handleItemClicked);
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

    for (int sliceIndex = 0; sliceIndex < currentSeries->slices.size(); ++sliceIndex) {
        const DicomSliceInfo &slice = currentSeries->slices.at(sliceIndex);

        ThumbnailItemData item;
        item.filePath        = slice.filePath;
        item.instanceNumber  = slice.hasInstanceNumber ? slice.instanceNumber : (sliceIndex + 1);
        item.sliceIndex      = sliceIndex;
        item.state           = ThumbnailState::NotRequested;

        QPixmap placeholderPixmap(160, 112);
        placeholderPixmap.fill(QColor(48, 52, 60));
        item.thumbnailPixmap = placeholderPixmap;

        items.push_back(item);
    }

    mThumbnailListModel->setItems(items);  // 通知：数据变了，重新绘制！

    // 以防setCurrentSliceIndex时，model还未加载完
    // 第一次导入thumbnail会受到两个信号：sessionChanged -> refreshFromSession（来自viewSession） |
    // currentStackSliceChanged ->setCurrentSliceIndex（来自stackPage）, 如果后者先到则 model 可能还未准备好导致缩略图无法同步主窗口
    if (mPendingSliceIndex >= 0) {
        setCurrentSliceIndex(mPendingSliceIndex);
    }
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
