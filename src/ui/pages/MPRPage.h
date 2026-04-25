#pragma once

#include <QString>
#include <QWidget>

#include "core/model/volume/SliceOrientation.h"
#include "ui/toolbars/StackToolMode.h"

class QLabel;
class SliceViewWidget;
class ViewerSession;

enum class MPRViewType
{
    Sagittal,
    Coronal,
    Axial,
};

struct MPRSliceState
{
    int sliceIndex         = -1;
    bool invert            = false;
    bool flipHorizontal    = false;
    bool flipVertical      = false;
};

class MPRPage : public QWidget
{
    Q_OBJECT

public:
    explicit MPRPage(QWidget *parent = nullptr);
    ~MPRPage();

    void setViewerSession(ViewerSession *viewerSession);

    void setToolMode(StackToolMode mode);
    void triggerInvert();
    void triggerFlipHorizontal();
    void triggerFlipVertical();
    void resetView();

private:
    void setupUi();
    void refreshFromSession();
    void clearDisplay();
    QWidget *createViewPane(const QString &title, const QString &objectName, SliceViewWidget **viewWidget);
    void resetSliceStates();
    void updateAllViews();
    void updateView(MPRViewType viewType);
    void handleSliceScrollRequested(MPRViewType viewType, int steps);
    void setActiveView(MPRViewType viewType);
    SliceViewWidget *viewForType(MPRViewType viewType) const;
    MPRSliceState *stateForType(MPRViewType viewType);
    SliceOrientation orientationForType(MPRViewType viewType) const;
    int sliceCountForType(MPRViewType viewType) const;

private:
    ViewerSession *mViewerSession       = nullptr;
    SliceViewWidget *mSagittalView      = nullptr;
    SliceViewWidget *mCoronalView       = nullptr;
    SliceViewWidget *mAxialView         = nullptr;
    MPRViewType mActiveView             = MPRViewType::Axial;
    MPRSliceState mSagittalState;
    MPRSliceState mCoronalState;
    MPRSliceState mAxialState;
    StackToolMode  mToolMode            = StackToolMode::Pan;
};
