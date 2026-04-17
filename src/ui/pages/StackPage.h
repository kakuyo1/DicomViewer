#pragma once

#include <QWidget>

class SliceViewWidget;
class ViewerSession;

class StackPage : public QWidget
{
    Q_OBJECT

public:
    explicit StackPage(QWidget *parent = nullptr);
    ~StackPage();

    void setViewerSession(ViewerSession *viewerSession);

private:
    void setupUi();
    void refreshFromSession();
    void updateDisplayedSlice();
    void handleSliceScrollRequested(int steps);
    void clearDisplay();

private:
    ViewerSession   *mViewerSession   = nullptr;
    SliceViewWidget *mSliceViewWidget = nullptr;
    int mCurrentSliceIndex = -1;
};
