#include "services/controller/ImportController.h"

#include <QFileDialog>
#include <QtConcurrent/QtConcurrentRun>

#include <spdlog/spdlog.h>

#include "core/worker/dicom/DicomSeriesScanner.h"
#include "core/worker/volume/VolumeBuilder.h"
#include "ui/dialogs/SeriesSelectionDialog.h"

namespace
{

QString buildScanFailureMessage(const QString &directoryPath)
{
    return QStringLiteral("No readable CT DICOM series were found in the selected folder:\n%1").arg(directoryPath);
}

} // namespace

ImportController::ImportController(QObject *parent)
    : QObject(parent)
{
    connect(&mScanWatcher, &QFutureWatcher<SeriesScanResult>::finished, this, &ImportController::handleScanFinished);
    connect(&mVolumeWatcher, &QFutureWatcher<VolumeBuildResult>::finished, this, &ImportController::handleVolumeBuildFinished);
}

void ImportController::startImportFromFolder(QWidget *dialogParent)
{
    if (mBusy) {
        return;
    }

    const QString directoryPath = QFileDialog::getExistingDirectory(
        dialogParent,
        QStringLiteral("Open DICOM Folder"));
    if (directoryPath.isEmpty()) {
        emit importCancelled();
        return;
    }

    mBusy = true;
    mDialogParent = dialogParent;
    mCurrentDirectoryPath = directoryPath;
    mPendingScanResult = {};
    mPendingSelectedSeries = {};
    emit importStarted();

    mScanWatcher.setFuture(QtConcurrent::run([directoryPath]() {
        DicomSeriesScanner scanner;
        SeriesScanResult scanResult;
        scanResult.seriesList = scanner.scanDirectory(directoryPath);

        for (const DicomSeries &series : std::as_const(scanResult.seriesList)) {
            scanResult.acceptedSliceCount += series.slices.size();
        }

        return scanResult;
    }));
}

bool ImportController::isBusy() const
{
    return mBusy;
}

void ImportController::handleScanFinished()
{
    mPendingScanResult = mScanWatcher.result();
    if (mPendingScanResult.seriesList.isEmpty()) {
        spdlog::warn("No readable CT series found in folder: {}", mCurrentDirectoryPath.toStdString());
        emit importFailed(buildScanFailureMessage(mCurrentDirectoryPath));
        resetState();
        return;
    }

    int selectedIndex = 0;
    if (mPendingScanResult.seriesList.size() > 1) {
        SeriesSelectionDialog dialog(mPendingScanResult.seriesList, mDialogParent);
        if (dialog.exec() != QDialog::Accepted) {
            spdlog::info("Series selection dialog cancelled");
            emit importCancelled();
            resetState();
            return;
        }

        selectedIndex = dialog.selectedSeriesIndex();
        if (selectedIndex < 0 || selectedIndex >= mPendingScanResult.seriesList.size()) {
            spdlog::warn("Series selection dialog returned invalid index: {}", selectedIndex);
            emit importFailed(QStringLiteral("Series selection returned an invalid result."));
            resetState();
            return;
        }
    }

    startVolumeBuild(mPendingScanResult.seriesList.at(selectedIndex));
}

void ImportController::handleVolumeBuildFinished()
{
    const VolumeBuildResult buildResult = mVolumeWatcher.result();
    if (!buildResult.success) {
        spdlog::warn("Volume build failed: {}", buildResult.errorMessage.toStdString());
        emit importFailed(
            buildResult.errorMessage.isEmpty()
                ? QStringLiteral("Failed to build volume data from the selected series.")
                : buildResult.errorMessage);
        resetState();
        return;
    }

    ImportResult result;
    result.selectedSeries = mPendingSelectedSeries;
    result.volumeData = buildResult.volumeData;
    result.scanResult = mPendingScanResult;
    emit importSucceeded(result);
    resetState();
}

void ImportController::startVolumeBuild(const DicomSeries &selectedSeries)
{
    mPendingSelectedSeries = selectedSeries;

    mVolumeWatcher.setFuture(QtConcurrent::run([selectedSeries]() {
        VolumeBuildResult result;
        VolumeBuilder builder;
        const auto volumeData = builder.build(selectedSeries, &result.errorMessage);
        if (!volumeData.has_value()) {
            return result;
        }

        result.success = true;
        result.volumeData = *volumeData;
        return result;
    }));
}

void ImportController::resetState()
{
    mBusy = false;
    mDialogParent = nullptr;
    mCurrentDirectoryPath.clear();
    mPendingScanResult = {};
    mPendingSelectedSeries = {};
}
