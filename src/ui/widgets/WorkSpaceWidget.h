#pragma once

#include <QStackedWidget>

#include "ui/model/ViewMode.h"
#include "ui/toolbars/SliceToolMode.h"

class StackPage;
class MPRPage;
class VRPage;
class ViewerSession;

class WorkSpaceWidget : public QStackedWidget
{
    Q_OBJECT

public:
    explicit WorkSpaceWidget(QWidget *parent = nullptr);
    ~WorkSpaceWidget();

    void     setViewerSession(ViewerSession *viewerSession);
    void     setViewMode(ViewMode mode);
    ViewMode currentViewMode() const;

    void setToolMode(SliceToolMode mode);
    SliceToolMode currentToolMode() const;
    void triggerInvert();
    void triggerFlipHorizontal();
    void triggerFlipVertical();
    void resetCurrentView();

    void setCurrentStackSliceIndex(int sliceIndex);

signals:
    void currentViewModeChanged(ViewMode mode);
    void currentStackSliceChanged(int sliceIndex);
    void stackDisplayParametersChanged(double windowCenter,
                                       double windowWidth,
                                       bool   invert,
                                       bool   flipHorizontal,
                                       bool   flipVertical);

private:
    void setupUi();

private:
    ViewerSession *mViewerSession   = nullptr;
    StackPage     *mStackPage       = nullptr;
    MPRPage       *mMPRPage         = nullptr;
    VRPage        *mVRPage          = nullptr;
    ViewMode       mCurrentViewMode = ViewMode::Stack;
};
