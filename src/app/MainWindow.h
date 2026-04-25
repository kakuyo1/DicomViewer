#pragma once

#include <QMainWindow>
#include <QString>

#include "services/model/ImportResult.h"
#include "ui/model/ViewMode.h"

class ImportController;
class ViewerSession;
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
    void setupConnects();
    void handleOpenFolderRequested();
    void handleImportStarted();
    void handleImportCancelled();
    void handleImportFailed(const QString &message);
    void handleImportSucceeded(const ImportResult &result);
    void handleViewModeChanged(ViewMode mode);
    void setImportBusy(bool busy);
    void printImportSucceededMsg(const ImportResult &result);

private:
    ImportController * mImportController = nullptr;
    ViewerSession    * mViewerSession    = nullptr;
    TitleBarWidget   * mTitleBar         = nullptr;
    StackToolBar     * mStackToolBar     = nullptr;
    ViewModeBar      * mViewModeBar      = nullptr;
    WorkSpaceWidget  * mWorkSpaceWidget  = nullptr;
    ThumbnailPanel   * mThumbnailPanel   = nullptr;

    bool mImportInProgress = false;
};
