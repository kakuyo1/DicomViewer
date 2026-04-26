#pragma once

#include <QPointF>
#include <QString>
#include <QWidget>

#include "core/model/volume/SliceOrientation.h"
#include "ui/toolbars/SliceToolMode.h"

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
    int  sliceIndex     = -1;
    bool invert         = false;
    bool flipHorizontal = false;
    bool flipVertical   = false;
};

class MPRPage : public QWidget
{
    Q_OBJECT

public:
    explicit MPRPage(QWidget *parent = nullptr);
    ~MPRPage();

    void setViewerSession(ViewerSession *viewerSession);
    void setRefreshEnabled(bool enabled);

    void setToolMode(SliceToolMode mode);
    SliceToolMode toolMode() const;
    void triggerInvert();
    void triggerFlipHorizontal();
    void triggerFlipVertical();
    void resetView();

private:
    void             setupUi();
    void             refreshFromSession();
    void             clearDisplay();
    QWidget         *createViewPane(const QString &title, const QString &objectName, SliceViewWidget **viewWidget);
    void             handleSessionChanged();
    void             resetSliceStates();
    void             updateAllViews();
    void             updateView(MPRViewType viewType);
    void             handleSliceScrollRequested(MPRViewType viewType, int steps);
    void             handleCrosshairPointChanged(MPRViewType viewType, const QPointF &imagePoint);
    void             updateCrosshairForAllViews();
    int              imageMmToVoxelIndex(double valueMm, double spacing, int count) const;
    void             setActiveView(MPRViewType viewType);
    SliceViewWidget *viewForType(MPRViewType viewType) const;
    MPRSliceState   *stateForType(MPRViewType viewType);
    SliceOrientation orientationForType(MPRViewType viewType) const;
    int              sliceCountForType(MPRViewType viewType) const;

private:
    ViewerSession *mViewerSession = nullptr;
    SliceToolMode  mToolMode      = SliceToolMode::Pan;
    MPRViewType    mActiveView    = MPRViewType::Axial;

    // - 隐藏状态下收到 sessionChanged 只标记 mRefreshPending = true，不执行 refreshFromSession()，不渲染三窗格。
    // - 切到 MPR View 时，如果有 pending session，再执行一次刷新。
    bool mRefreshEnabled = false;
    bool mRefreshPending = true;

    SliceViewWidget *mSagittalView = nullptr;
    SliceViewWidget *mCoronalView  = nullptr;
    SliceViewWidget *mAxialView    = nullptr;

    MPRSliceState mSagittalState;
    MPRSliceState mCoronalState;
    MPRSliceState mAxialState;
};
