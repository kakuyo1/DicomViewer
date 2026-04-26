#pragma once

#include <QWidget>

#include "ui/toolbars/SliceToolMode.h"

class SliceViewWidget;
class ViewerSession;

class StackPage : public QWidget
{
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

    void          setViewerSession(ViewerSession *viewerSession);
    void          setCurrentSliceIndex(int sliceIndex);
    void          setToolMode(SliceToolMode mode);
    SliceToolMode toolMode() const;
    void          toggleInvert();
    void          toggleFlipHorizontal();
    void          toggleFlipVertical();
    void          resetView();

signals:
    void currentSliceChanged(int sliceIndex);
    void displayParametersChanged(double windowCenter,
                                  double windowWidth,
                                  bool   invert,
                                  bool   flipHorizontal,
                                  bool   flipVertical);

private:
    void setupUi();
    void refreshFromSession();
    void updateDisplayedSlice();
    void handleSliceScrollRequested(int steps);
    void clearDisplay();

private:
    ViewerSession   *mViewerSession     = nullptr;
    SliceViewWidget *mSliceViewWidget   = nullptr;
    int              mCurrentSliceIndex = -1;

    SliceToolMode mToolMode              = SliceToolMode::WindowLevel;
    bool          mInvertEnabled         = false;
    bool          mFlipHorizontalEnabled = false;
    bool          mFlipVerticalEnabled   = false;
};
