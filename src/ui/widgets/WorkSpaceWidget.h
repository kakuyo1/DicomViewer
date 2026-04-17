#pragma once

#include <QStackedWidget>

#include "../pages/StackPage.h"

class ViewerSession;

class WorkSpaceWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WorkSpaceWidget(QWidget *parent = nullptr);
    ~WorkSpaceWidget();

    void setViewerSession(ViewerSession *viewerSession);

private:
    void setupUi();

private:
    ViewerSession *mViewerSession = nullptr;
    StackPage     *mStackPage     = nullptr;
};
