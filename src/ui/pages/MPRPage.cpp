#include "MPRPage.h"

#include <algorithm>
#include <cmath>

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "common/Util.h"
#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"
#include "services/state/ViewerSession.h"
#include "ui/widgets/SliceViewWidget.h"

MPRPage::MPRPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

MPRPage::~MPRPage()
{
}

void MPRPage::setupUi()
{
    setObjectName(QStringLiteral("mprPage"));
    setAttribute(Qt::WA_StyledBackground, true); // 该Widget的style依赖QSS

    auto *rootLayout = new QGridLayout(this);
    rootLayout->setContentsMargins(8, 8, 8, 8);
    rootLayout->setSpacing(8);

    QWidget *sagittalPane = createViewPane(QStringLiteral("Sagittal"), QStringLiteral("mprSagittalPane"), &mSagittalView);
    QWidget *coronalPane  = createViewPane(QStringLiteral("Coronal"), QStringLiteral("mprCoronalPane"), &mCoronalView);
    QWidget *axialPane    = createViewPane(QStringLiteral("Axial"), QStringLiteral("mprAxialPane"), &mAxialView);

    rootLayout->addWidget(sagittalPane, 0, 0);
    rootLayout->addWidget(coronalPane, 1, 0);
    rootLayout->addWidget(axialPane, 0, 1, 2, 1);
    rootLayout->setColumnStretch(0, 10); // 宽度(小:大) 1 : 1.5
    rootLayout->setColumnStretch(1, 15);
    rootLayout->setRowStretch(0, 1); // 高度(小:大) 1 : 2
    rootLayout->setRowStretch(1, 1);

    // 激活单个窗口
    connect(mSagittalView, &SliceViewWidget::activated, this, [this]() {
        setActiveView(MPRViewType::Sagittal);
    });
    connect(mCoronalView, &SliceViewWidget::activated, this, [this]() {
        setActiveView(MPRViewType::Coronal);
    });
    connect(mAxialView, &SliceViewWidget::activated, this, [this]() {
        setActiveView(MPRViewType::Axial);
    });

    // 滚轮切片
    connect(mSagittalView, &SliceViewWidget::sliceScrollRequested, this, [this](int steps) {
        handleSliceScrollRequested(MPRViewType::Sagittal, steps);
    });
    connect(mCoronalView, &SliceViewWidget::sliceScrollRequested, this, [this](int steps) {
        handleSliceScrollRequested(MPRViewType::Coronal, steps);
    });
    connect(mAxialView, &SliceViewWidget::sliceScrollRequested, this, [this](int steps) {
        handleSliceScrollRequested(MPRViewType::Axial, steps);
    });

    // WW/WL 同步
    connect(mSagittalView, &SliceViewWidget::windowLevelEdited, this, [this](double windowCenter, double windowWidth) {
        handleWindowLevelEdited(MPRViewType::Sagittal, windowCenter, windowWidth);
    });
    connect(mCoronalView, &SliceViewWidget::windowLevelEdited, this, [this](double windowCenter, double windowWidth) {
        handleWindowLevelEdited(MPRViewType::Coronal, windowCenter, windowWidth);
    });
    connect(mAxialView, &SliceViewWidget::windowLevelEdited, this, [this](double windowCenter, double windowWidth) {
        handleWindowLevelEdited(MPRViewType::Axial, windowCenter, windowWidth);
    });

    // Crosshair 功能下鼠标按下
    connect(mSagittalView, &SliceViewWidget::imagePointPressed, this, [this](const QPointF &imagePoint) {
        handleCrosshairPointChanged(MPRViewType::Sagittal, imagePoint);
    });

    connect(mCoronalView, &SliceViewWidget::imagePointPressed, this, [this](const QPointF &imagePoint) {
        handleCrosshairPointChanged(MPRViewType::Coronal, imagePoint);
    });
    connect(mAxialView, &SliceViewWidget::imagePointPressed, this, [this](const QPointF &imagePoint) {
        handleCrosshairPointChanged(MPRViewType::Axial, imagePoint);
    });

    // Crosshair 功能下鼠标拖动
    connect(mSagittalView, &SliceViewWidget::imagePointDragged, this, [this](const QPointF &imagePoint) {
        handleCrosshairPointChanged(MPRViewType::Sagittal, imagePoint);
    });
    connect(mCoronalView, &SliceViewWidget::imagePointDragged, this, [this](const QPointF &imagePoint) {
        handleCrosshairPointChanged(MPRViewType::Coronal, imagePoint);
    });
    connect(mAxialView, &SliceViewWidget::imagePointDragged, this, [this](const QPointF &imagePoint) {
        handleCrosshairPointChanged(MPRViewType::Axial, imagePoint);
    });

    setActiveView(MPRViewType::Axial);
}

QWidget *MPRPage::createViewPane(const QString &title, const QString &objectName, SliceViewWidget **viewWidget)
{
    auto *pane = new QWidget(this);
    pane->setObjectName(objectName);
    pane->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(pane);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    auto *titleLabel = new QLabel(title, pane);
    titleLabel->setObjectName(QStringLiteral("mprPaneTitle"));
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto *sliceView = new SliceViewWidget(pane);
    sliceView->setMinimumSize(220, 180);

    layout->addWidget(titleLabel);
    layout->addWidget(sliceView, 1);

    if (viewWidget != nullptr) {
        *viewWidget = sliceView;
    }

    return pane;
}

void MPRPage::setViewerSession(ViewerSession *viewerSession)
{
    if (mViewerSession == viewerSession) { // 重复
        return;
    }

    if (mViewerSession != nullptr) { // 已有
        disconnect(mViewerSession, nullptr, this, nullptr);
    }

    mViewerSession = viewerSession;
    if (mViewerSession != nullptr) {
        connect(mViewerSession, &ViewerSession::sessionChanged, this, &MPRPage::handleSessionChanged);
        connect(mViewerSession, &ViewerSession::sessionCleared, this, &MPRPage::clearDisplay);
    }

    handleSessionChanged();
}

void MPRPage::setRefreshEnabled(bool enabled)
{
    if (mRefreshEnabled == enabled) {
        return;
    }

    mRefreshEnabled = enabled;
    if (mRefreshEnabled && mRefreshPending) {
        refreshFromSession();
    }
}

void MPRPage::setToolMode(SliceToolMode mode)
{
    mToolMode = mode;
    applyToolModeToViews(mode);
    updateCrosshairForAllViews(); // 只有 crosshair 模式下才出现十字线
}

SliceToolMode MPRPage::toolMode() const
{
    return mToolMode;
}

void MPRPage::triggerInvert()
{
    MPRSliceState *activeState = stateForType(mActiveView);
    if (activeState == nullptr) {
        return;
    }

    const bool enabled    = !activeState->invert;
    mSagittalState.invert = enabled;
    mCoronalState.invert  = enabled;
    mAxialState.invert    = enabled;
    if (mSagittalView != nullptr) {
        mSagittalView->setInvertEnabled(enabled);
    }
    if (mCoronalView != nullptr) {
        mCoronalView->setInvertEnabled(enabled);
    }
    if (mAxialView != nullptr) {
        mAxialView->setInvertEnabled(enabled);
    }
}

void MPRPage::triggerFlipHorizontal()
{
    MPRSliceState *activeState = stateForType(mActiveView);
    if (activeState == nullptr) {
        return;
    }

    const bool enabled            = !activeState->flipHorizontal;
    mSagittalState.flipHorizontal = enabled;
    mCoronalState.flipHorizontal  = enabled;
    mAxialState.flipHorizontal    = enabled;
    if (mSagittalView != nullptr) {
        mSagittalView->setFlipHorizontalEnabled(enabled);
    }
    if (mCoronalView != nullptr) {
        mCoronalView->setFlipHorizontalEnabled(enabled);
    }
    if (mAxialView != nullptr) {
        mAxialView->setFlipHorizontalEnabled(enabled);
    }
    updateCrosshairForAllViews();
    updateOrientationMarkersForAllViews();
}

void MPRPage::triggerFlipVertical()
{
    MPRSliceState *activeState = stateForType(mActiveView);
    if (activeState == nullptr) {
        return;
    }

    const bool enabled          = !activeState->flipVertical;
    mSagittalState.flipVertical = enabled;
    mCoronalState.flipVertical  = enabled;
    mAxialState.flipVertical    = enabled;
    if (mSagittalView != nullptr) {
        mSagittalView->setFlipVerticalEnabled(enabled);
    }
    if (mCoronalView != nullptr) {
        mCoronalView->setFlipVerticalEnabled(enabled);
    }
    if (mAxialView != nullptr) {
        mAxialView->setFlipVerticalEnabled(enabled);
    }
    updateCrosshairForAllViews();
    updateOrientationMarkersForAllViews();
}

void MPRPage::resetView()
{
    resetSliceStates();
    if (mSagittalView != nullptr) {
        mSagittalView->resetViewState(false);
    }
    if (mCoronalView != nullptr) {
        mCoronalView->resetViewState(false);
    }
    if (mAxialView != nullptr) {
        mAxialView->resetViewState(false);
    }
    updateAllViews();
}

void MPRPage::refreshFromSession() // 属于加载完一次series后开始初始化
{
    resetSliceStates();
    if (mSagittalView != nullptr) {
        mSagittalView->resetViewState(false);
    }
    if (mCoronalView != nullptr) {
        mCoronalView->resetViewState(false);
    }
    if (mAxialView != nullptr) {
        mAxialView->resetViewState(false);
    }
    updateAllViews();
}

void MPRPage::clearDisplay()
{
    mRefreshPending = true;

    mSagittalState = MPRSliceState();
    mCoronalState  = MPRSliceState();
    mAxialState    = MPRSliceState();

    if (mSagittalView != nullptr) {
        mSagittalView->clearDisplay();
        mSagittalView->clearCrosshair();
        mSagittalView->clearOrientationMarkers();
    }
    if (mCoronalView != nullptr) {
        mCoronalView->clearDisplay();
        mCoronalView->clearCrosshair();
        mCoronalView->clearOrientationMarkers();
    }
    if (mAxialView != nullptr) {
        mAxialView->clearDisplay();
        mAxialView->clearCrosshair();
        mAxialView->clearOrientationMarkers();
    }
}

void MPRPage::handleSessionChanged()
{
    mRefreshPending = true;
    if (!mRefreshEnabled) { // 如果还没有进入过 MPR ，比如还在Stack,那么就会跳过不渲染 MPR
        return;
    }

    refreshFromSession();
}

void MPRPage::resetSliceStates()
{
    const VolumeData *volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    if (volumeData == nullptr || !volumeData->isValid()) {
        clearDisplay();
        return;
    }

    mSagittalState = MPRSliceState();
    mCoronalState  = MPRSliceState();
    mAxialState    = MPRSliceState();

    mSagittalState.sliceIndex = volumeData->width / 2;
    mCoronalState.sliceIndex  = volumeData->height / 2;
    mAxialState.sliceIndex    = volumeData->depth / 2;
    mActiveView               = MPRViewType::Axial;
}

void MPRPage::updateAllViews()
{
    mRefreshPending = false;

    // 先渲染图
    updateView(MPRViewType::Sagittal);
    updateView(MPRViewType::Coronal);
    updateView(MPRViewType::Axial);
    // 再绘制十字线
    updateCrosshairForAllViews(); // 非 crosshair 按钮立刻隐藏十字线
    updateOrientationMarkersForAllViews();
}

void MPRPage::updateView(MPRViewType viewType)
{
    const DicomSeries *currentSeries = (mViewerSession != nullptr) ? mViewerSession->currentSeries() : nullptr;
    const VolumeData  *volumeData    = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    SliceViewWidget   *view          = viewForType(viewType);
    MPRSliceState     *state         = stateForType(viewType);
    if (currentSeries == nullptr || volumeData == nullptr || !volumeData->isValid() || view == nullptr || state == nullptr) {
        if (view != nullptr) {
            view->clearDisplay();
        }
        return;
    }

    const int sliceCount = sliceCountForType(viewType);
    if (sliceCount <= 0) {
        view->clearDisplay();
        return;
    }

    state->sliceIndex = std::clamp(state->sliceIndex, 0, sliceCount - 1);
    view->showSlice(*currentSeries, *volumeData, orientationForType(viewType), state->sliceIndex);
}

void MPRPage::handleSliceScrollRequested(MPRViewType viewType, int steps)
{
    if (steps == 0) {
        return;
    }

    const int      SliceCount = sliceCountForType(viewType);
    MPRSliceState *state      = stateForType(viewType);

    if (SliceCount > 0) {
        state->sliceIndex = std::clamp(state->sliceIndex + steps, 0, SliceCount - 1);
    }

    updateView(viewType);

    if (mToolMode == SliceToolMode::Crosshair) {
        updateCrosshairForAllViews();
    }
}

void MPRPage::handleWindowLevelEdited(MPRViewType viewType, double windowCenter, double windowWidth)
{
    if (mToolMode != SliceToolMode::WindowLevel) {
        return;
    }

    if (viewType != MPRViewType::Sagittal && mSagittalView != nullptr) {
        mSagittalView->setWindowLevel(windowCenter, windowWidth);
    }
    if (viewType != MPRViewType::Coronal && mCoronalView != nullptr) {
        mCoronalView->setWindowLevel(windowCenter, windowWidth);
    }
    if (viewType != MPRViewType::Axial && mAxialView != nullptr) {
        mAxialView->setWindowLevel(windowCenter, windowWidth);
    }
}

void MPRPage::handleCrosshairPointChanged(MPRViewType viewType, const QPointF &imagePoint)
{
    const VolumeData *volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    if (volumeData == nullptr || !volumeData->isValid()) {
        return;
    }

    // 如果视图被 flip 了，这个 imagePoint 是翻转后的显示坐标。
    // 但 MPR 内部维护的 Crosshair 核心状态是未翻转的 volume 坐标：
    // -- x = mSagittalState.sliceIndex
    // -- y = mCoronalState.sliceIndex
    // -- z = mAxialState.sliceIndex
    // 所以这里要先把 点击点 反解回 未翻转坐标，然后再用它更新三维体素位置。（用当前显示 imagePoint 反解出来的原始平面坐标更新体素）
    // 你不能用反转后的 imagePoint 去更新三维体素位置(x,y,z)! 因为虽然进行了翻转，但是你想要的点在三维体素中还是同一个
    // 记住：更新体素坐标要用原始未翻转的平面坐标，更新十字线则在体素坐标投影后直接翻转。关键是找到那个不变的体素坐标
    const QPointF unflippedPoint = mirrorPointForViewFlips(viewType, imagePoint);

    bool sagittalChanged = false;
    bool coronalChanged  = false;
    bool axialChanged    = false;

    const auto updateSliceIndexIfChanged = [](MPRSliceState &state, int sliceIndex) {
        if (state.sliceIndex == sliceIndex) { // 旧 - 新
            return false;
        }
        state.sliceIndex = sliceIndex;
        return true;
    };

    // 更新三维体素位置，只重建 sliceIndex 实际变化的视图。
    if (viewType == MPRViewType::Axial) {
        const int sagittalSliceIndex = imageMmToVoxelIndex(unflippedPoint.x(), volumeData->spacingX, volumeData->width);
        const int coronalSliceIndex  = imageMmToVoxelIndex(unflippedPoint.y(), volumeData->spacingY, volumeData->height);
        sagittalChanged              = updateSliceIndexIfChanged(mSagittalState, sagittalSliceIndex);
        coronalChanged               = updateSliceIndexIfChanged(mCoronalState, coronalSliceIndex);
    } else if (viewType == MPRViewType::Coronal) {
        const int sagittalSliceIndex = imageMmToVoxelIndex(unflippedPoint.x(), volumeData->spacingX, volumeData->width);
        const int axialSliceIndex    = imageMmToVoxelIndex(unflippedPoint.y(), volumeData->spacingZ, volumeData->depth);
        sagittalChanged              = updateSliceIndexIfChanged(mSagittalState, sagittalSliceIndex);
        axialChanged                 = updateSliceIndexIfChanged(mAxialState, axialSliceIndex);
    } else if (viewType == MPRViewType::Sagittal) {
        const int coronalSliceIndex = imageMmToVoxelIndex(unflippedPoint.x(), volumeData->spacingY, volumeData->height);
        const int axialSliceIndex   = imageMmToVoxelIndex(unflippedPoint.y(), volumeData->spacingZ, volumeData->depth);
        coronalChanged              = updateSliceIndexIfChanged(mCoronalState, coronalSliceIndex);
        axialChanged                = updateSliceIndexIfChanged(mAxialState, axialSliceIndex);
    }

    if (sagittalChanged) {
        updateView(MPRViewType::Sagittal);
    }
    if (coronalChanged) {
        updateView(MPRViewType::Coronal);
    }
    if (axialChanged) {
        updateView(MPRViewType::Axial);
    }
    updateCrosshairForAllViews();
}

void MPRPage::updateCrosshairForAllViews()
{
    const VolumeData *volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;

    if (mToolMode != SliceToolMode::Crosshair || volumeData == nullptr || !volumeData->isValid()) {
        if (mSagittalView != nullptr) {
            mSagittalView->clearCrosshair();
        }
        if (mCoronalView != nullptr) {
            mCoronalView->clearCrosshair();
        }
        if (mAxialView != nullptr) {
            mAxialView->clearCrosshair();
        }
        return;
    }

    // Crosshair 的核心不是画两条线，而是维护一个三维体素坐标：
    // (x, y, z) = (Sagittal index, Coronal index, Axial index)
    // 点击任意视图时，根据该视图的方向把 2D image point 反推到 (x, y, z)
    const int x = std::clamp(mSagittalState.sliceIndex, 0, volumeData->width - 1);
    const int y = std::clamp(mCoronalState.sliceIndex, 0, volumeData->height - 1);
    const int z = std::clamp(mAxialState.sliceIndex, 0, volumeData->depth - 1);

    // 绘制时，再把 `(x, y, z)` 投影回三个视图各自的 image point。
    // 1.如果是 crosshair 状态下点击或者拖动：这里输入 map 的坐标是更新完后的体素坐标，但是这里是更新十字线，需要翻转一次。
    // 2.如果是 单次点击 flipH/V, 则只是体素坐标投影后单纯翻转。
    // 注意：这里的体素坐标是基于未翻转的原始平面坐标得到的！
    if (mSagittalView != nullptr) {
        const QPointF sagittalPoint = mirrorPointForViewFlips(MPRViewType::Sagittal, QPointF(y * volumeData->spacingY, z * volumeData->spacingZ));
        mSagittalView->setCrosshairImagePoint(sagittalPoint);
    }
    if (mCoronalView != nullptr) {
        const QPointF coronalPoint = mirrorPointForViewFlips(MPRViewType::Coronal, QPointF(x * volumeData->spacingX, z * volumeData->spacingZ));
        mCoronalView->setCrosshairImagePoint(coronalPoint);
    }
    if (mAxialView != nullptr) {
        const QPointF axialPoint = mirrorPointForViewFlips(MPRViewType::Axial, QPointF(x * volumeData->spacingX, y * volumeData->spacingY));
        mAxialView->setCrosshairImagePoint(axialPoint);
    }
}

void MPRPage::updateOrientationMarkersForAllViews()
{
    updateOrientationMarkersForView(MPRViewType::Sagittal);
    updateOrientationMarkersForView(MPRViewType::Coronal);
    updateOrientationMarkersForView(MPRViewType::Axial);
}

void MPRPage::updateOrientationMarkersForView(MPRViewType viewType)
{
    const VolumeData *volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    SliceViewWidget  *view       = viewForType(viewType);
    MPRSliceState    *state      = stateForType(viewType);
    if (view == nullptr) {
        return;
    }
    if (volumeData == nullptr || !volumeData->isValid() || !volumeData->hasPatientOrientation || state == nullptr) {
        view->clearOrientationMarkers();
        return;
    }

    DicomVector3 screenRight;
    DicomVector3 screenDown;
    if (viewType == MPRViewType::Axial) {
        screenRight = volumeData->volumeXDirection;
        screenDown  = volumeData->volumeYDirection;
    } else if (viewType == MPRViewType::Coronal) {
        screenRight = volumeData->volumeXDirection;
        screenDown  = volumeData->volumeZDirection;
    } else if (viewType == MPRViewType::Sagittal) {
        screenRight = volumeData->volumeYDirection;
        screenDown  = volumeData->volumeZDirection;
    }

    if (state->flipHorizontal) {
        screenRight = util::reversedDirection(screenRight);
    }
    if (state->flipVertical) {
        screenDown = util::reversedDirection(screenDown);
    }

    view->setOrientationMarkers(
        util::patientDirectionLabel(util::reversedDirection(screenRight)),
        util::patientDirectionLabel(screenRight),
        util::patientDirectionLabel(util::reversedDirection(screenDown)),
        util::patientDirectionLabel(screenDown));
}

// 从 image 坐标转体素 index 时，使用 `std::lround`，再 clamp 到合法范围。
// e.g.
// - spacing = 0.7mm
// - 旧位置 = 7.0mm
// - 旧 index = lround(7.0 / 0.7) = 10
//
// - 鼠标移动 0.2mm
// - 新位置 = 7.2mm
// - 新 index = lround(7.2 / 0.7) = lround(10.2857) = 10
// 所以：
// state.sliceIndex == sliceIndex成立，updateSliceIndexIfChanged(...)
// 返回 false，对应的 updateView(...) 不会执行。
int MPRPage::imageMmToVoxelIndex(double valueMm, double spacing, int count) const
{
    if (spacing <= 0.0 || count <= 0) {
        return 0;
    }

    const int index = static_cast<int>(std::lround(valueMm / spacing));
    return std::clamp(index, 0, count - 1);
}

QPointF MPRPage::mirrorPointForViewFlips(MPRViewType viewType, const QPointF &imagePoint)
{
    const VolumeData *volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    MPRSliceState    *state      = stateForType(viewType);
    if (volumeData == nullptr || !volumeData->isValid() || state == nullptr) {
        return imagePoint;
    }

    double maxX = 0.0;
    double maxY = 0.0;
    if (viewType == MPRViewType::Axial) {
        maxX = static_cast<double>(volumeData->width - 1) * volumeData->spacingX;
        maxY = static_cast<double>(volumeData->height - 1) * volumeData->spacingY;
    } else if (viewType == MPRViewType::Coronal) {
        maxX = static_cast<double>(volumeData->width - 1) * volumeData->spacingX;
        maxY = static_cast<double>(volumeData->depth - 1) * volumeData->spacingZ;
    } else if (viewType == MPRViewType::Sagittal) {
        maxX = static_cast<double>(volumeData->height - 1) * volumeData->spacingY;
        maxY = static_cast<double>(volumeData->depth - 1) * volumeData->spacingZ;
    }

    QPointF mappedPoint = imagePoint;
    if (state->flipHorizontal) {
        mappedPoint.setX(maxX - mappedPoint.x());
    }
    if (state->flipVertical) {
        mappedPoint.setY(maxY - mappedPoint.y());
    }
    return mappedPoint;
}

void MPRPage::setActiveView(MPRViewType viewType)
{
    mActiveView                 = viewType;
    SliceViewWidget *activeView = viewForType(mActiveView);
    if (activeView != nullptr) {
        activeView->setToolMode(mToolMode);
    }
}

SliceViewWidget *MPRPage::viewForType(MPRViewType viewType) const
{
    if (viewType == MPRViewType::Sagittal) {
        return mSagittalView;
    }
    if (viewType == MPRViewType::Coronal) {
        return mCoronalView;
    }
    if (viewType == MPRViewType::Axial) {
        return mAxialView;
    }
    return nullptr;
}

MPRSliceState *MPRPage::stateForType(MPRViewType viewType)
{
    if (viewType == MPRViewType::Sagittal) {
        return &mSagittalState;
    }
    if (viewType == MPRViewType::Coronal) {
        return &mCoronalState;
    }
    if (viewType == MPRViewType::Axial) {
        return &mAxialState;
    }
    return nullptr;
}

SliceOrientation MPRPage::orientationForType(MPRViewType viewType) const
{
    if (viewType == MPRViewType::Sagittal) {
        return SliceOrientation::Sagittal;
    }
    if (viewType == MPRViewType::Coronal) {
        return SliceOrientation::Coronal;
    }
    return SliceOrientation::Axial;
}

int MPRPage::sliceCountForType(MPRViewType viewType) const
{
    const VolumeData *volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    if (volumeData == nullptr || !volumeData->isValid()) {
        return 0;
    }

    if (viewType == MPRViewType::Sagittal) {
        return volumeData->width;
    }
    if (viewType == MPRViewType::Coronal) {
        return volumeData->height;
    }
    if (viewType == MPRViewType::Axial) {
        return volumeData->depth;
    }
    return 0;
}

void MPRPage::applyToolModeToViews(SliceToolMode mode)
{
    if (mSagittalView != nullptr) {
        mSagittalView->setToolMode(mode);
    }
    if (mCoronalView != nullptr) {
        mCoronalView->setToolMode(mode);
    }
    if (mAxialView != nullptr) {
        mAxialView->setToolMode(mode);
    }
}
