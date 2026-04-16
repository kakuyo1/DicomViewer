#include "services/controller/ImportController.h"

#include <QFileDialog>

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

std::optional<ImportResult> ImportController::importFromFolder(QWidget *dialogParent, QString *errorMessage) const
{
    const QString directoryPath = QFileDialog::getExistingDirectory(
        dialogParent,
        QStringLiteral("Open DICOM Folder"));
    if (directoryPath.isEmpty()) {
        return std::nullopt;
    }

    DicomSeriesScanner scanner;
    SeriesScanResult scanResult;
    scanResult.seriesList = scanner.scanDirectory(directoryPath);

    for (const DicomSeries &series : std::as_const(scanResult.seriesList)) {
        scanResult.acceptedSliceCount += series.slices.size();
    }

    if (scanResult.seriesList.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = buildScanFailureMessage(directoryPath);
        }
        spdlog::warn("No readable CT series found in folder: {}", directoryPath.toStdString());
        return std::nullopt;
    }

    int selectedIndex = 0;
    if (scanResult.seriesList.size() > 1) {
        SeriesSelectionDialog dialog(scanResult.seriesList, dialogParent);
        if (dialog.exec() != QDialog::Accepted) {
            spdlog::info("Series selection dialog cancelled");
            return std::nullopt;
        }

        selectedIndex = dialog.selectedSeriesIndex();
        if (selectedIndex < 0 || selectedIndex >= scanResult.seriesList.size()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Series selection returned an invalid result.");
            }
            spdlog::warn("Series selection dialog returned invalid index: {}", selectedIndex);
            return std::nullopt;
        }
    }

    const DicomSeries &selectedSeries = scanResult.seriesList.at(selectedIndex);

    VolumeBuilder builder;
    QString volumeError;
    const auto volumeData = builder.build(selectedSeries, &volumeError);
    if (!volumeData.has_value()) {
        if (errorMessage != nullptr) {
            *errorMessage = volumeError.isEmpty()
                ? QStringLiteral("Failed to build volume data from the selected series.")
                : volumeError;
        }
        spdlog::warn("Volume build failed: {}", volumeError.toStdString());
        return std::nullopt;
    }

    ImportResult result;
    result.selectedSeries = selectedSeries;
    result.volumeData = *volumeData;
    result.scanResult = scanResult;
    return result;
}
