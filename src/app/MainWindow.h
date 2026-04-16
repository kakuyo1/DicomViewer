#pragma once

#include <QMainWindow>
#include <QString>

#include <optional>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"
#include "services/model/ImportResult.h"

class ImportController;
class TitleBarWidget;
class StackToolBar;
class ViewModeBar;
class WorkSpaceWidget;
class ThumbnailPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void handleOpenFolderRequested();
    void handleImportStarted();
    void handleImportCancelled();
    void handleImportFailed(const QString &message);
    void handleImportSucceeded(const ImportResult &result);
    void setImportBusy(bool busy);

private:
    ImportController *mImportController = nullptr;
    TitleBarWidget * mTitleBar        = nullptr;
    StackToolBar   * mStackToolBar    = nullptr;
    ViewModeBar    * mViewModeBar     = nullptr;
    WorkSpaceWidget* mWorkSpaceWidget = nullptr;
    ThumbnailPanel * mThumbnailPanel  = nullptr;
    std::optional<DicomSeries> mCurrentSeries;
    std::optional<VolumeData> mCurrentVolumeData;
    bool mImportInProgress = false;
};
