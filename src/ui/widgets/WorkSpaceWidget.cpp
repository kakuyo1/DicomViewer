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
    if (mVRPage != nullptr) {
        mVRPage->setRefreshEnabled(mCurrentViewMode == ViewMode::VR);
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

    // 延迟渲染 MPR / VR 页面，避免导入后在隐藏页面做重渲染。
    if (mMPRPage != nullptr) {
        mMPRPage->setRefreshEnabled(mode == ViewMode::MPR);
    }
    if (mVRPage != nullptr) {
        mVRPage->setRefreshEnabled(mode == ViewMode::VR);
    }
    emit currentViewModeChanged(mode);
}

ViewMode WorkSpaceWidget::currentViewMode() const
{
    return mCurrentViewMode;
}

void WorkSpaceWidget::setToolMode(SliceToolMode mode)
{
    if (mCurrentViewMode == ViewMode::Stack && mStackPage != nullptr) {
        mStackPage->setToolMode(mode);
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->setToolMode(mode);
    }
}

SliceToolMode WorkSpaceWidget::currentToolMode() const
{
    if (mCurrentViewMode == ViewMode::Stack && mStackPage != nullptr) {
        return mStackPage->toolMode();
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        return mMPRPage->toolMode();
    }
    return SliceToolMode::Pan;
}

void WorkSpaceWidget::triggerInvert()
{
    if (mCurrentViewMode == ViewMode::Stack && mStackPage != nullptr) {
        mStackPage->toggleInvert();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->triggerInvert();
    }
}

void WorkSpaceWidget::triggerFlipHorizontal()
{
    if (mCurrentViewMode == ViewMode::Stack && mStackPage != nullptr) {
        mStackPage->toggleFlipHorizontal();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->triggerFlipHorizontal();
    }
}

void WorkSpaceWidget::triggerFlipVertical()
{
    if (mCurrentViewMode == ViewMode::Stack && mStackPage != nullptr) {
        mStackPage->toggleFlipVertical();
        return;
    }
    if (mCurrentViewMode == ViewMode::MPR && mMPRPage != nullptr) {
        mMPRPage->triggerFlipVertical();
    }
}

void WorkSpaceWidget::resetCurrentView()
{
    if (mCurrentViewMode == ViewMode::Stack && mStackPage != nullptr) {
        mStackPage->resetView();
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
