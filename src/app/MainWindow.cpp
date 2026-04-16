#include "app/MainWindow.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QWidget>

#include <spdlog/spdlog.h>

#include "core/worker/dicom/DicomSeriesScanner.h"
#include "core/worker/volume/VolumeBuilder.h"
#include "ui/dialogs/SeriesSelectionDialog.h"
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

    connect(mTitleBar, &TitleBarWidget::openFolderRequested, this, &MainWindow::handleOpenFolderRequested);
}

void MainWindow::handleOpenFolderRequested()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Open DICOM Folder"));
    if (directoryPath.isEmpty()) {
        return;
    }

    DicomSeriesScanner scanner;
    const QVector<DicomSeries> seriesList = scanner.scanDirectory(directoryPath);
    if (seriesList.isEmpty()) {
        QMessageBox::warning(
            this,
            QStringLiteral("No CT Series Found"),
            QStringLiteral("No readable CT DICOM series were found in the selected folder."));
        spdlog::warn("No readable CT series found in folder: {}", directoryPath.toStdString());
        return;
    }

    if (seriesList.size() == 1) {
        setCurrentSeries(seriesList.first());
        return;
    }

    SeriesSelectionDialog dialog(seriesList, this);
    if (dialog.exec() != QDialog::Accepted) {
        spdlog::info("Series selection dialog cancelled");
        return;
    }

    const int selectedIndex = dialog.selectedSeriesIndex();
    if (selectedIndex < 0 || selectedIndex >= seriesList.size()) {
        spdlog::warn("Series selection dialog returned invalid index: {}", selectedIndex);
        return;
    }

    setCurrentSeries(seriesList.at(selectedIndex));
}

void MainWindow::setCurrentSeries(const DicomSeries &series)
{
    mCurrentSeries = series;
    mCurrentVolumeData.reset();

    const QString description = series.seriesDescription.isEmpty()
        ? QStringLiteral("(No Series Description)")
        : series.seriesDescription;

    spdlog::info(
        "Selected CT series: {} | {} slices | {}",
        description.toStdString(),
        series.slices.size(),
        series.pathSummary.toStdString());

    if (!buildVolumeForCurrentSeries()) {
        mCurrentSeries.reset();
    }
}

bool MainWindow::buildVolumeForCurrentSeries()
{
    if (!mCurrentSeries.has_value()) {
        return false;
    }

    VolumeBuilder builder;
    QString errorMessage;
    const auto volumeData = builder.build(*mCurrentSeries, &errorMessage);
    if (!volumeData.has_value()) {
        QMessageBox::warning(
            this,
            QStringLiteral("Volume Build Failed"),
            errorMessage.isEmpty() ? QStringLiteral("Failed to build volume data from the selected series.") : errorMessage);
        spdlog::warn("Volume build failed: {}", errorMessage.toStdString());
        return false;
    }

    mCurrentVolumeData = *volumeData;
    spdlog::info(
        "Built volume data: {} x {} x {} | voxels={} | spacing=({}, {}, {})",
        mCurrentVolumeData->width,
        mCurrentVolumeData->height,
        mCurrentVolumeData->depth,
        mCurrentVolumeData->voxels.size(),
        mCurrentVolumeData->spacingX,
        mCurrentVolumeData->spacingY,
        mCurrentVolumeData->spacingZ);
    return true;
}
