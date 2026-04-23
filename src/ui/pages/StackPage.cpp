#include "StackPage.h"

#include <algorithm>

#include <QVBoxLayout>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"
#include "services/state/ViewerSession.h"
#include "ui/widgets/SliceViewWidget.h"

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

StackPage::~StackPage()
{

}

void StackPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    mSliceViewWidget = new SliceViewWidget(this);
    mSliceViewWidget->setMinimumSize(640, 480);
    rootLayout->addWidget(mSliceViewWidget);

    connect(mSliceViewWidget, &SliceViewWidget::sliceScrollRequested, this, &StackPage::handleSliceScrollRequested);
    connect(mSliceViewWidget, &SliceViewWidget::displayParametersChanged, this, &StackPage::displayParametersChanged);
}

void StackPage::setViewerSession(ViewerSession *viewerSession)
{
    if (mViewerSession == viewerSession) {
        return;
    }

    if (mViewerSession != nullptr) {
        disconnect(mViewerSession, nullptr, this, nullptr);
    }

    mViewerSession = viewerSession;
    if (mViewerSession != nullptr) {
        connect(mViewerSession, &ViewerSession::sessionChanged, this, &StackPage::refreshFromSession);
        connect(mViewerSession, &ViewerSession::sessionCleared, this, &StackPage::clearDisplay);
    }

    refreshFromSession();
}

void StackPage::setCurrentSliceIndex(int sliceIndex)
{
    if (mViewerSession == nullptr) {
        return;
    }

    const VolumeData *volumeData = mViewerSession->currentVolumeData();
    if (volumeData == nullptr || !volumeData->isValid()) {
        return;
    }

    const int clampedSliceIndex = std::clamp(sliceIndex, 0, volumeData->depth - 1);
    if (mCurrentSliceIndex == clampedSliceIndex) {
        return;
    }

    mCurrentSliceIndex = clampedSliceIndex;
    updateDisplayedSlice();
}

void StackPage::setToolMode(StackToolMode mode)
{
    mToolMode = mode;
    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->setToolMode(mode);
    }
}

void StackPage::toggleInvert()
{
    mInvertEnabled = !mInvertEnabled;
    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->setInvertEnabled(mInvertEnabled);
    }
}

void StackPage::toggleFlipHorizontal()
{
    mFlipHorizontalEnabled = !mFlipHorizontalEnabled;
    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->setFlipHorizontalEnabled(mFlipHorizontalEnabled);
    }
}

void StackPage::toggleFlipVertical()
{
    mFlipVerticalEnabled = !mFlipVerticalEnabled;
    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->setFlipVerticalEnabled(mFlipVerticalEnabled);
    }
}

void StackPage::resetView()
{
    mInvertEnabled         = false;
    mFlipHorizontalEnabled = false;
    mFlipVerticalEnabled   = false;

    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->resetViewState();
    }
}

void StackPage::refreshFromSession()    /// 导入成功初始化切片
{
    if (mSliceViewWidget == nullptr || mViewerSession == nullptr) {
        return;
    }

    const DicomSeries *currentSeries = mViewerSession->currentSeries();
    const VolumeData  *volumeData    = mViewerSession->currentVolumeData();
    if (currentSeries == nullptr || volumeData == nullptr || !volumeData->isValid()) {
        clearDisplay();
        return;
    }

    /** @note 默认选择中间的切片展示，比较符合用户习惯 */
    mCurrentSliceIndex = volumeData->depth / 2;
    updateDisplayedSlice();
}

void StackPage::updateDisplayedSlice()
{
    if (mSliceViewWidget == nullptr || mViewerSession == nullptr) {
        return;
    }

    const DicomSeries *currentSeries = mViewerSession->currentSeries();
    const VolumeData  *volumeData    = mViewerSession->currentVolumeData();
    if (currentSeries == nullptr || volumeData == nullptr || !volumeData->isValid()) {
        clearDisplay();
        return;
    }

    if (mCurrentSliceIndex < 0) {
        mCurrentSliceIndex = 0;
    }

    mCurrentSliceIndex = std::clamp(mCurrentSliceIndex, 0, volumeData->depth - 1);
    mSliceViewWidget->showAxialSlice(*currentSeries, *volumeData, mCurrentSliceIndex);
    emit currentSliceChanged(mCurrentSliceIndex); // 通知一下缩略图滚动同步
}

void StackPage::handleSliceScrollRequested(int steps)   /// 滚轮切换切片
{
    if (steps == 0 || mViewerSession == nullptr) {
        return;
    }

    const VolumeData *volumeData = mViewerSession->currentVolumeData();
    if (volumeData == nullptr || !volumeData->isValid()) {
        return;
    }

    if (mCurrentSliceIndex < 0) {
        mCurrentSliceIndex = volumeData->depth / 2;
    }

    mCurrentSliceIndex += steps;
    mCurrentSliceIndex = std::clamp(mCurrentSliceIndex, 0, volumeData->depth - 1);
    updateDisplayedSlice();
}

void StackPage::clearDisplay()
{
    mCurrentSliceIndex = -1;
    emit currentSliceChanged(-1);
    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->clearDisplay();
    }
}
