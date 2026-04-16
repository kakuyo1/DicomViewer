#include "app/MainWindow.h"

#include <QHBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

#include <spdlog/spdlog.h>

#include "services/controller/ImportController.h"
#include "services/model/ImportResult.h"
#include "ui/toolbars/StackToolBar.h"
#include "ui/toolbars/ViewModeBar.h"
#include "ui/widgets/WorkSpaceWidget.h"
#include "ui/widgets/TitleBarWidget.h"
#include "ui/thumbnailpanel/ThumbnailPanel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    mImportController = new ImportController(this);
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

    connect(mTitleBar, &TitleBarWidget::openFolderRequested, this, &MainWindow::handleOpenFolderRequested);
    connect(mImportController, &ImportController::importStarted, this, &MainWindow::handleImportStarted);
    connect(mImportController, &ImportController::importCancelled, this, &MainWindow::handleImportCancelled);
    connect(mImportController, &ImportController::importFailed, this, &MainWindow::handleImportFailed);
    connect(mImportController, &ImportController::importSucceeded, this, &MainWindow::handleImportSucceeded);
}

void MainWindow::handleOpenFolderRequested()
{
    if (mImportController->isBusy()) {
        return;
    }

    mImportController->startImportFromFolder(this);
}

void MainWindow::handleImportStarted()
{
    setImportBusy(true);
}

void MainWindow::handleImportCancelled()
{
    setImportBusy(false);
}

void MainWindow::handleImportFailed(const QString &message)
{
    setImportBusy(false);
    if (!message.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Import Failed"), message);
    }
}

void MainWindow::handleImportSucceeded(const ImportResult &result)
{
    setImportBusy(false);

    mCurrentSeries = result.selectedSeries;
    mCurrentVolumeData = result.volumeData;

    const QString description = mCurrentSeries->seriesDescription.isEmpty()
        ? QStringLiteral("(No Series Description)")
        : mCurrentSeries->seriesDescription;

    spdlog::info(
        "Selected CT series: {} | {} slices | {}",
        description.toStdString(),
        mCurrentSeries->slices.size(),
        mCurrentSeries->pathSummary.toStdString());

    spdlog::info(
        "Built volume data: {} x {} x {} | voxels={} | spacing=({}, {}, {})",
        mCurrentVolumeData->width,
        mCurrentVolumeData->height,
        mCurrentVolumeData->depth,
        mCurrentVolumeData->voxels.size(),
        mCurrentVolumeData->spacingX,
        mCurrentVolumeData->spacingY,
        mCurrentVolumeData->spacingZ);
}

void MainWindow::setImportBusy(bool busy)
{
    mImportInProgress = busy;
    if (busy) {
        QApplication::setOverrideCursor(Qt::BusyCursor);
    } else {
        QApplication::restoreOverrideCursor();
    }
}
