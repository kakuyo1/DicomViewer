#pragma once

#include <QStackedWidget>

#include "../pages/StackPage.h"
#include "ui/toolbars/StackToolMode.h"

class ViewerSession;

class WorkSpaceWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WorkSpaceWidget(QWidget *parent = nullptr);
    ~WorkSpaceWidget();

    void setViewerSession(ViewerSession *viewerSession);
    void setStackToolMode(StackToolMode mode);
    void triggerStackInvert();
    void triggerStackFlipHorizontal();
    void triggerStackFlipVertical();
    void resetStackView();

private:
    void setupUi();

private:
    ViewerSession *mViewerSession = nullptr;
    StackPage     *mStackPage     = nullptr;
};
