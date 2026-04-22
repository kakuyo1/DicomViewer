#pragma once

#include <QWidget>

#include "ui/toolbars/StackToolMode.h"

class SliceViewWidget;
class ViewerSession;

class StackPage : public QWidget
{
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

    void setViewerSession(ViewerSession *viewerSession);
    void setCurrentSliceIndex(int sliceIndex);
    void setToolMode(StackToolMode mode);
    void toggleInvert();
    void toggleFlipHorizontal();
    void toggleFlipVertical();
    void resetView();

signals:
    void currentSliceChanged(int sliceIndex);

private:
    void setupUi();
    void refreshFromSession();
    void updateDisplayedSlice();
    void handleSliceScrollRequested(int steps);
    void clearDisplay();

private:
    ViewerSession   *mViewerSession   = nullptr;
    SliceViewWidget *mSliceViewWidget = nullptr;
    int mCurrentSliceIndex            = -1;

    StackToolMode mToolMode           = StackToolMode::Pan;
    bool mInvertEnabled               = false;
    bool mFlipHorizontalEnabled       = false;
    bool mFlipVerticalEnabled         = false;
};
