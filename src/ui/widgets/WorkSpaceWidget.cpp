#include "WorkSpaceWidget.h"

#include "services/state/ViewerSession.h"

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
    addWidget(mStackPage);
    setCurrentWidget(mStackPage);
}

void WorkSpaceWidget::setViewerSession(ViewerSession *viewerSession)
{
    mViewerSession = viewerSession;
    if (mStackPage != nullptr) {
        mStackPage->setViewerSession(mViewerSession);
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
