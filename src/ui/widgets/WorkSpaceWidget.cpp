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
