#include "WorkSpaceWidget.h"

#include "services/state/ViewerSession.h"
#include "ui/pages/StackPage.h"
#include "ui/pages/MPRPage.h"
#include "ui/pages/VRPage.h"

WorkSpaceWidget::WorkSpaceWidget(QWidget *parent)
    : QStackedWidget(parent)
{
    setupUi();
}

WorkSpaceWidget::~WorkSpaceWidget()
{
}

void WorkSpaceWidget::setupUi()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mStackPage = new StackPage(this);
    connect(mStackPage, &StackPage::currentSliceChanged, this, &WorkSpaceWidget::currentStackSliceChanged);
    connect(mStackPage, &StackPage::displayParametersChanged, this, &WorkSpaceWidget::stackDisplayParametersChanged);
    addWidget(mStackPage);

    mMPRPage = new MPRPage(this);
    addWidget(mMPRPage);

    mVRPage = new VRPage(this);
    addWidget(mVRPage);

    setCurrentWidget(mStackPage);
}

void WorkSpaceWidget::setViewerSession(ViewerSession *viewerSession)
{
    mViewerSession = viewerSession;
    if (mStackPage != nullptr) {
        mStackPage->setViewerSession(mViewerSession);
    }
    if (mMPRPage != nullptr) {
        mMPRPage->setViewerSession(mViewerSession);
    }
    if (mVRPage != nullptr) {
        mVRPage->setViewerSession(mViewerSession);
    }
    if (mMPRPage != nullptr) {
        mMPRPage->setRefreshEnabled(mCurrentViewMode == ViewMode::MPR);
    }
}

void WorkSpaceWidget::setViewMode(ViewMode mode)
{
    if (mCurrentViewMode == mode) {
        return;
    }

    QWidget *targetPage = nullptr;
    if (mode == ViewMode::Stack) {
        targetPage = mStackPage;
    } else if (mode == ViewMode::MPR) {
        targetPage = mMPRPage;
    } else if (mode == ViewMode::VR) {
        targetPage = mVRPage;
    }

    if (targetPage == nullptr) {
        return;
    }

    mCurrentViewMode = mode;
    setCurrentWidget(targetPage);

    // 延迟渲染 MPR 页面
    // - 切换到 MPR 时启用 MPR 刷新。
    // - 非 MPR 模式下禁用 MPR 自动刷新。
    if (mMPRPage != nullptr) {
        mMPRPage->setRefreshEnabled(mode == ViewMode::MPR);
    }
    emit currentViewModeChanged(mode);
}

ViewMode WorkSpaceWidget::currentViewMode() const
{
    return mCurrentViewMode;
}

void WorkSpaceWidget::setToolMode(StackToolMode mode)
{
    if (mCurrentViewMode == ViewMode::Stack) {
        setStackToolMode(mode);
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->setToolMode(mode);
    }
}

void WorkSpaceWidget::triggerInvert()
{
    if (mCurrentViewMode == ViewMode::Stack) {
        triggerStackInvert();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->triggerInvert();
    }
}

void WorkSpaceWidget::triggerFlipHorizontal()
{
    if (mCurrentViewMode == ViewMode::Stack) {
        triggerStackFlipHorizontal();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->triggerFlipHorizontal();
    }
}

void WorkSpaceWidget::triggerFlipVertical()
{
    if (mCurrentViewMode == ViewMode::Stack) {
        triggerStackFlipVertical();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->triggerFlipVertical();
    }
}

void WorkSpaceWidget::resetCurrentView()
{
    if (mCurrentViewMode == ViewMode::Stack) {
        resetStackView();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->resetView();
        return;
    }
    if (mCurrentViewMode == ViewMode::VR && mVRPage != nullptr) {
        mVRPage->resetView();
    }
}

void WorkSpaceWidget::setCurrentStackSliceIndex(int sliceIndex)
{
    if (mStackPage != nullptr) {
        mStackPage->setCurrentSliceIndex(sliceIndex);
    }
}

void WorkSpaceWidget::setStackToolMode(StackToolMode mode)
{
    if (mStackPage != nullptr) {
        mStackPage->setToolMode(mode);
    }
}

void WorkSpaceWidget::triggerStackInvert()
{
    if (mStackPage != nullptr) {
        mStackPage->toggleInvert();
    }
}

void WorkSpaceWidget::triggerStackFlipHorizontal()
{
    if (mStackPage != nullptr) {
        mStackPage->toggleFlipHorizontal();
    }
}

void WorkSpaceWidget::triggerStackFlipVertical()
{
    if (mStackPage != nullptr) {
        mStackPage->toggleFlipVertical();
    }
}

void WorkSpaceWidget::resetStackView()
{
    if (mStackPage != nullptr) {
        mStackPage->resetView();
    }
}
