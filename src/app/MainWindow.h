#pragma once

#include <QMainWindow>
#include <QString>

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

private:
    TitleBarWidget * mTitleBar        = nullptr;
    StackToolBar   * mStackToolBar    = nullptr;
    ViewModeBar    * mViewModeBar     = nullptr;
    WorkSpaceWidget* mWorkSpaceWidget = nullptr;
    ThumbnailPanel * mThumbnailPanel  = nullptr;
};
