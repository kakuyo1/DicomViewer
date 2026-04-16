#pragma once

#include <QMainWindow>
#include <QString>

#include <optional>

#include "core/model/dicom/DicomSeries.h"

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
    void setCurrentSeries(const DicomSeries &series);

private:
    TitleBarWidget * mTitleBar        = nullptr;
    StackToolBar   * mStackToolBar    = nullptr;
    ViewModeBar    * mViewModeBar     = nullptr;
    WorkSpaceWidget* mWorkSpaceWidget = nullptr;
    ThumbnailPanel * mThumbnailPanel  = nullptr;
    std::optional<DicomSeries> mCurrentSeries;
};
