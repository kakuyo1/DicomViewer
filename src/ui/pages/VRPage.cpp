#include "VRPage.h"

#include <QLabel>
#include <QStackedLayout>
#include <QVTKOpenGLNativeWidget.h>

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageImport.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

#include "core/model/volume/VolumeData.h"
#include "services/state/ViewerSession.h"

VRPage::VRPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupVtkPipeline();
}

VRPage::~VRPage()
{
}

void VRPage::setupUi()
{
    setObjectName(QStringLiteral("vrPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *rootLayout = new QStackedLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setStackingMode(QStackedLayout::StackAll);

    mVtkWidget = new QVTKOpenGLNativeWidget(this);
    mVtkWidget->setObjectName(QStringLiteral("vrVtkWidget"));

    mHintLabel = new QLabel(QStringLiteral("Import a CT series to view 3D volume rendering"), this);
    mHintLabel->setObjectName(QStringLiteral("placeholderPageHint"));
    mHintLabel->setAlignment(Qt::AlignCenter);
    mHintLabel->setWordWrap(true);
    mHintLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    rootLayout->addWidget(mVtkWidget);
    rootLayout->addWidget(mHintLabel);
}

void VRPage::setupVtkPipeline()
{
    mRenderWindow            = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    mRenderer                = vtkSmartPointer<vtkRenderer>::New();
    mVolumeImporter          = vtkSmartPointer<vtkImageImport>::New();
    mVolumeMapper            = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    mVolume                  = vtkSmartPointer<vtkVolume>::New();
    mVolumeProperty          = vtkSmartPointer<vtkVolumeProperty>::New();
    mColorTransferFunction   = vtkSmartPointer<vtkColorTransferFunction>::New();
    mOpacityTransferFunction = vtkSmartPointer<vtkPiecewiseFunction>::New();

    mRenderer->SetBackground(0.0, 0.0, 0.0);
    mRenderWindow->AddRenderer(mRenderer);

    if (mVtkWidget != nullptr) {
        mVtkWidget->setRenderWindow(mRenderWindow);
    }

    vtkRenderWindowInteractor *interactor = mRenderWindow->GetInteractor();
    if (interactor != nullptr) {
        vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
        interactor->SetInteractorStyle(style);
    }

    if (mVolumeMapper == nullptr) {
        if (mHintLabel != nullptr) {
            mHintLabel->setText(QStringLiteral("VTK GPU volume mapper is unavailable"));
            mHintLabel->show();
            mHintLabel->raise();
        }
        return;
    }

    setupDefaultTransferFunction();
    mVolumeProperty->SetColor(mColorTransferFunction);
    mVolumeProperty->SetScalarOpacity(mOpacityTransferFunction);
    // mVolumeProperty->ShadeOn();
    mVolumeProperty->ShadeOff();
    mVolumeProperty->SetInterpolationTypeToLinear();
    mVolumeProperty->SetAmbient(0.2);
    mVolumeProperty->SetDiffuse(0.7);
    mVolumeProperty->SetSpecular(0.2);

    mVolumeMapper->SetBlendModeToComposite();     // 默认：体渲染混合模式，用于累加沿着射线穿过体数据时的颜色和不透明度
    mVolumeMapper->AutoAdjustSampleDistancesOn(); // 根据相机距离动态调整光线行进时的采样步长，在保持图像质量的同时优化性能
    if (mVolumeImporter != nullptr) {
        mVolumeMapper->SetInputConnection(mVolumeImporter->GetOutputPort());
    }

    mVolume->SetMapper(mVolumeMapper);
    mVolume->SetProperty(mVolumeProperty);
}

void VRPage::setViewerSession(ViewerSession *viewerSession)
{
    if (mViewerSession == viewerSession) { // 重复
        return;
    }

    if (mViewerSession != nullptr) { // 已有
        disconnect(mViewerSession, nullptr, this, nullptr);
    }

    mViewerSession = viewerSession;
    if (mViewerSession != nullptr) {
        connect(mViewerSession, &ViewerSession::sessionChanged, this, &VRPage::handleSessionChanged);
        connect(mViewerSession, &ViewerSession::sessionCleared, this, &VRPage::clearDisplay);
    }

    handleSessionChanged();
}

void VRPage::setRefreshEnabled(bool enabled)
{
    if (mRefreshEnabled == enabled) {
        return;
    }

    mRefreshEnabled = enabled;
    if (mRefreshEnabled && mRefreshPending) { // 条件：1.在VR Page 2.有新的 volume 加载完毕
        refreshFromSession();
    }
}

void VRPage::resetView()
{
    if (!mHasVolume || mRenderer == nullptr || mRenderWindow == nullptr) {
        refreshFromSession();
        return;
    }

    mRenderer->ResetCamera();
    mRenderer->ResetCameraClippingRange();
    mRenderWindow->Render();
}

void VRPage::handleSessionChanged()
{
    mRefreshPending = true; // 有待刷新的 volume
    if (!mRefreshEnabled) { // 如果没有在 VR Page，disable 后延迟刷新
        clearDisplay();     // 隐藏时收到新 session，清掉旧 VR 引用，避免隐藏页面长期持有旧 volume
        return;
    }

    refreshFromSession();
}

void VRPage::refreshFromSession()
{
    mRefreshPending = false; // 表示最新的 volume 已经开始渲染

    const std::shared_ptr<const VolumeData> volumeData = (mViewerSession != nullptr) ? mViewerSession->currentVolumeDataShared() : nullptr;
    if (volumeData == nullptr || !volumeData->isValid()) {
        clearDisplay();
        return;
    }

    updateVolumeData(*volumeData);
    mCurrentVolumeData = volumeData;
    renderVolume();
}

void VRPage::clearDisplay()
{
    mRefreshPending = true;
    mHasVolume      = false;

    if (mRenderer != nullptr && mVolume != nullptr) {
        mRenderer->RemoveVolume(mVolume);
    }
    if (mVolumeImporter != nullptr) { // 断开外部 buffer
        mVolumeImporter->SetImportVoidPointer(static_cast<void *>(nullptr));
        mVolumeImporter->Modified();
    }
    mCurrentVolumeData.reset(); // 释放
    if (mHintLabel != nullptr) {
        mHintLabel->setText(QStringLiteral("Import a CT series to view 3D volume rendering"));
        mHintLabel->show();
        mHintLabel->raise();
    }
    if (mRenderWindow != nullptr) {
        mRenderWindow->Render();
    }
}

void VRPage::updateVolumeData(const VolumeData &volumeData)
{
    if (mVolumeImporter == nullptr || mVolumeMapper == nullptr || !volumeData.isValid()) {
        clearDisplay();
        return;
    }

    mVolumeImporter->SetDataScalarTypeToShort();
    mVolumeImporter->SetNumberOfScalarComponents(1);
    mVolumeImporter->SetDataSpacing(volumeData.spacingX, volumeData.spacingY, volumeData.spacingZ);
    mVolumeImporter->SetDataOrigin(0.0, 0.0, 0.0);
    mVolumeImporter->SetWholeExtent(0, volumeData.width - 1, 0, volumeData.height - 1, 0, volumeData.depth - 1);
    mVolumeImporter->SetDataExtentToWholeExtent();

    mVolumeImporter->SetImportVoidPointer(const_cast<qint16 *>(volumeData.voxels.constData()));
    mVolumeImporter->Modified();
    mVolumeMapper->SetInputConnection(mVolumeImporter->GetOutputPort());
}

void VRPage::setupDefaultTransferFunction()
{
    if (mColorTransferFunction == nullptr || mOpacityTransferFunction == nullptr) {
        return;
    }

    mColorTransferFunction->RemoveAllPoints();
    mColorTransferFunction->AddRGBPoint(-1000.0, 0.0, 0.0, 0.0);
    mColorTransferFunction->AddRGBPoint(-300.0, 0.12, 0.12, 0.12);
    mColorTransferFunction->AddRGBPoint(300.0, 0.75, 0.72, 0.68);
    mColorTransferFunction->AddRGBPoint(1000.0, 1.0, 0.95, 0.85);
    mColorTransferFunction->AddRGBPoint(2000.0, 1.0, 1.0, 1.0);

    mOpacityTransferFunction->RemoveAllPoints();
    mOpacityTransferFunction->AddPoint(-1000.0, 0.00);
    mOpacityTransferFunction->AddPoint(-300.0, 0.00);
    mOpacityTransferFunction->AddPoint(100.0, 0.02);
    mOpacityTransferFunction->AddPoint(300.0, 0.08);
    mOpacityTransferFunction->AddPoint(1000.0, 0.35);
    mOpacityTransferFunction->AddPoint(2000.0, 0.60);
}

void VRPage::renderVolume()
{
    if (mRenderer == nullptr || mRenderWindow == nullptr || mVolume == nullptr || mVolumeMapper == nullptr) {
        clearDisplay();
        return;
    }

    if (!mHasVolume) {
        mRenderer->AddVolume(mVolume);
    }
    mHasVolume = true;

    if (mHintLabel != nullptr) {
        mHintLabel->hide();
    }

    mRenderer->ResetCamera();
    vtkCamera *camera = mRenderer->GetActiveCamera();
    if (camera != nullptr) {
        camera->Azimuth(35.0);
        camera->Elevation(20.0);
        camera->Dolly(1.1);
    }
    mRenderer->ResetCameraClippingRange();
    mRenderWindow->Render();
}
