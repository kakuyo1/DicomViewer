#include "MPRPage.h"

#include <algorithm>

#include <QGridLayout>
#include <QLabel>
#include <QVBoxLayout>

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
    rootLayout->setColumnStretch(0, 1);
    rootLayout->setColumnStretch(1, 2);
    rootLayout->setRowStretch(0, 1);
    rootLayout->setRowStretch(1, 1);

    connect(mSagittalView, &SliceViewWidget::activated, this, [this]() {
        setActiveView(MPRViewType::Sagittal);
    });
    connect(mCoronalView, &SliceViewWidget::activated, this, [this]() {
        setActiveView(MPRViewType::Coronal);
    });
    connect(mAxialView, &SliceViewWidget::activated, this, [this]() {
        setActiveView(MPRViewType::Axial);
    });

    connect(mSagittalView, &SliceViewWidget::sliceScrollRequested, this, [this](int steps) {
        handleSliceScrollRequested(MPRViewType::Sagittal, steps);
    });
    connect(mCoronalView, &SliceViewWidget::sliceScrollRequested, this, [this](int steps) {
        handleSliceScrollRequested(MPRViewType::Coronal, steps);
    });
    connect(mAxialView, &SliceViewWidget::sliceScrollRequested, this, [this](int steps) {
        handleSliceScrollRequested(MPRViewType::Axial, steps);
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

void MPRPage::setToolMode(StackToolMode mode)
{
    mToolMode                   = mode;
    SliceViewWidget *activeView = viewForType(mActiveView);
    if (activeView != nullptr) {
        activeView->setToolMode(mode);
    }
}

void MPRPage::triggerInvert()
{
    MPRSliceState   *state = stateForType(mActiveView);
    SliceViewWidget *view  = viewForType(mActiveView);
    if (state == nullptr || view == nullptr) {
        return;
    }

    state->invert = !state->invert;
    view->setInvertEnabled(state->invert);
}

void MPRPage::triggerFlipHorizontal()
{
    MPRSliceState   *state = stateForType(mActiveView);
    SliceViewWidget *view  = viewForType(mActiveView);
    if (state == nullptr || view == nullptr) {
        return;
    }

    state->flipHorizontal = !state->flipHorizontal;
    view->setFlipHorizontalEnabled(state->flipHorizontal);
}

void MPRPage::triggerFlipVertical()
{
    MPRSliceState   *state = stateForType(mActiveView);
    SliceViewWidget *view  = viewForType(mActiveView);
    if (state == nullptr || view == nullptr) {
        return;
    }

    state->flipVertical = !state->flipVertical;
    view->setFlipVerticalEnabled(state->flipVertical);
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
    }
    if (mCoronalView != nullptr) {
        mCoronalView->clearDisplay();
    }
    if (mAxialView != nullptr) {
        mAxialView->clearDisplay();
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

    updateView(MPRViewType::Sagittal);
    updateView(MPRViewType::Coronal);
    updateView(MPRViewType::Axial);
    setToolMode(mToolMode);
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
    MPRSliceState *state = stateForType(viewType);
    if (state == nullptr || steps == 0) {
        return;
    }

    const int sliceCount = sliceCountForType(viewType);
    if (sliceCount <= 0) {
        return;
    }

    state->sliceIndex = std::clamp(state->sliceIndex + steps, 0, sliceCount - 1);
    updateView(viewType);
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
