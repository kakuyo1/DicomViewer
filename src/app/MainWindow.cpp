#include "app/MainWindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

#include <spdlog/spdlog.h>

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
    resize(1280, 800);
    setMinimumSize(960, 640);
    setWindowFlag(Qt::FramelessWindowHint, true);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("mainWindowRoot"));

    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    mTitleBar = new TitleBarWidget(central);
    mTitleBar->setBarHeight(40);

    auto *contentPlaceholder = new QWidget(central);
    contentPlaceholder->setObjectName(QStringLiteral("mainContentPlaceholder"));

    auto *contentLayout = new QHBoxLayout(contentPlaceholder);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    auto *contentLabel = new QLabel(QStringLiteral("Main workspace placeholder"), contentPlaceholder);
    contentLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(contentLabel);

    rootLayout->addWidget(mTitleBar);
    rootLayout->addWidget(contentPlaceholder, 1);

    central->setStyleSheet(QStringLiteral(R"(
        #mainWindowRoot {
            background-color: #10151c;
        }
        #mainContentPlaceholder {
            background-color: #0d1117;
        }
        #mainContentPlaceholder QLabel {
            color: #7e8795;
            font-size: 14px;
        }
    )"));

    setCentralWidget(central);

    connect(mTitleBar, &TitleBarWidget::openFolderRequested, this, []() {
        spdlog::info("Title bar file button clicked");
    });

    spdlog::info("Frameless main window initialized");
}
