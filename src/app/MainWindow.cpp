#include "app/MainWindow.h"

#include <QHBoxLayout>
#include <QApplication>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

#include <spdlog/spdlog.h>

#include "services/controller/ImportController.h"
#include "services/model/ImportResult.h"
#include "services/state/ViewerSession.h"
#include "ui/toolbars/StackToolBar.h"
#include "ui/toolbars/ViewModeBar.h"
#include "ui/widgets/WorkSpaceWidget.h"
#include "ui/widgets/TitleBarWidget.h"
#include "ui/thumbnailpanel/ThumbnailPanel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    mImportController = new ImportController(this);
    mViewerSession    = new ViewerSession(this);

    setupUi();
    setupConnects();
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
    mWorkSpaceWidget->setViewerSession(mViewerSession);
    mThumbnailPanel = new ThumbnailPanel(contentContainer);

    contentLayout->addWidget(leftContainer);
    contentLayout->addWidget(mWorkSpaceWidget, 1);
    contentLayout->addWidget(mThumbnailPanel);

    rootLayout->addWidget(mTitleBar);
    rootLayout->addWidget(toolBarContainer);
    rootLayout->addWidget(contentContainer, 1);

    setCentralWidget(central);
}

void MainWindow::setupConnects()
{
    connect(mTitleBar, &TitleBarWidget::openFolderRequested,       this, &MainWindow::handleOpenFolderRequested);

    connect(mImportController, &ImportController::importStarted,   this, &MainWindow::handleImportStarted);
    connect(mImportController, &ImportController::importCancelled, this, &MainWindow::handleImportCancelled);
    connect(mImportController, &ImportController::importFailed,    this, &MainWindow::handleImportFailed);
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

    mViewerSession->setImportResult(result);

    const DicomSeries *currentSeries     = mViewerSession->currentSeries();
    const VolumeData  *currentVolumeData = mViewerSession->currentVolumeData();
    if (currentSeries == nullptr || currentVolumeData == nullptr) {
        spdlog::warn("ViewerSession did not retain a valid series/volume after import success");
        return;
    }

    const QString description = currentSeries->seriesDescription.isEmpty()
        ? QStringLiteral("(No Series Description)")
        : currentSeries->seriesDescription;

    spdlog::info(
        "Selected CT series: {} | {} slices | {}",
        description.toStdString(),
        currentSeries->slices.size(),
        currentSeries->pathSummary.toStdString());

    spdlog::info(
        "Scan summary: dir={} scanned={} readable={} accepted={} skipped_non_ct={} skipped_unreadable={} skipped_missing_uid={} series={} strategy={} geometry_hints={}",
        result.scanResult.directoryPath.toStdString(),
        result.scanResult.scannedFileCount,
        result.scanResult.readableDicomCount,
        result.scanResult.acceptedSliceCount,
        result.scanResult.skippedNonCtCount,
        result.scanResult.skippedUnreadableFileCount,
        result.scanResult.skippedMissingSeriesUidCount,
        result.scanResult.seriesCount,
        currentSeries->sortStrategySummary.toStdString(),
        currentSeries->hasGeometryHints);

    spdlog::info(
        "Build summary: {} | geometry={}",
        result.volumeBuildResult.buildSummary.toStdString(),
        currentVolumeData->geometrySummary.toStdString());

    for (const QString &warning : result.volumeBuildResult.warnings) {
        spdlog::warn("Import warning: {}", warning.toStdString());
    }
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
