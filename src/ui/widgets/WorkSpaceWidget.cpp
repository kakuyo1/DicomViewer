#include "WorkSpaceWidget.h"

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
