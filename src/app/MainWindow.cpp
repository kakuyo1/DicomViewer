#include "app/MainWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/toolbars/StackToolBar.h"
#include "ui/toolbars/ViewModeBar.h"
#include "ui/widgets/WorkSpaceWidget.h"
#include "ui/widgets/TitleBarWidget.h"
#include "ui/thumbnailpanel/ThumbnailPanel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
}

void MainWindow::setupUi()
{
    resize(1440, 810);
    setMinimumSize(960, 640);
    setWindowFlag(Qt::FramelessWindowHint, true);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("mainWindowRoot"));

    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    mTitleBar = new TitleBarWidget(central);
    mTitleBar->setBarHeight(40);

    auto *toolBarContainer = new QWidget(central);
    auto *toolBarLayout = new QHBoxLayout(toolBarContainer);
    toolBarLayout->setContentsMargins(16, 12, 16, 12);
    toolBarLayout->setSpacing(0);

    mStackToolBar = new StackToolBar(toolBarContainer);
    toolBarLayout->addStretch();
    toolBarLayout->addWidget(mStackToolBar);
    toolBarLayout->addStretch();

    auto *contentContainer = new QWidget(central);
    auto *contentLayout = new QHBoxLayout(contentContainer);
    contentLayout->setContentsMargins(16, 0, 16, 16);
    contentLayout->setSpacing(16);

    auto *leftContainer = new QWidget(contentContainer);
    auto *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    mViewModeBar = new ViewModeBar(leftContainer);
    leftLayout->addStretch();
    leftLayout->addWidget(mViewModeBar, 0, Qt::AlignHCenter);
    leftLayout->addStretch();

    mWorkSpaceWidget = new WorkSpaceWidget(contentContainer);
    mThumbnailPanel = new ThumbnailPanel(contentContainer);

    contentLayout->addWidget(leftContainer);
    contentLayout->addWidget(mWorkSpaceWidget, 1);
    contentLayout->addWidget(mThumbnailPanel);

    rootLayout->addWidget(mTitleBar);
    rootLayout->addWidget(toolBarContainer);
    rootLayout->addWidget(contentContainer, 1);

    setCentralWidget(central);
}
