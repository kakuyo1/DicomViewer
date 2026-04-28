#pragma once

#include <vtkSmartPointer.h>

#include <memory>

#include <QWidget>

class QLabel;
class QVTKOpenGLNativeWidget;
class ViewerSession;
class vtkColorTransferFunction;
class vtkGenericOpenGLRenderWindow;
class vtkGPUVolumeRayCastMapper;
class vtkImageImport;
class vtkPiecewiseFunction;
class vtkRenderer;
class vtkVolume;
class vtkVolumeProperty;

struct VolumeData;

class VRPage : public QWidget
{
    Q_OBJECT

public:
    explicit VRPage(QWidget *parent = nullptr);
    ~VRPage();

    void setViewerSession(ViewerSession *viewerSession);
    void setRefreshEnabled(bool enabled);
    void resetView();

private:
    void setupUi();
    void setupVtkPipeline();
    void handleSessionChanged();
    void refreshFromSession();
    void clearDisplay();
    void updateVolumeData(const VolumeData &volumeData);
    void setupDefaultTransferFunction();
    void renderVolume();

private:
    ViewerSession *mViewerSession = nullptr;
    QLabel        *mHintLabel     = nullptr;

    bool mRefreshEnabled = false;
    bool mRefreshPending = true;

    QVTKOpenGLNativeWidget *mVtkWidget = nullptr;

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenderWindow;
    vtkSmartPointer<vtkRenderer>                  mRenderer;
    vtkSmartPointer<vtkImageImport>               mVolumeImporter;
    vtkSmartPointer<vtkGPUVolumeRayCastMapper>    mVolumeMapper;
    vtkSmartPointer<vtkVolume>                    mVolume;
    vtkSmartPointer<vtkVolumeProperty>            mVolumeProperty;
    vtkSmartPointer<vtkColorTransferFunction>     mColorTransferFunction;
    vtkSmartPointer<vtkPiecewiseFunction>         mOpacityTransferFunction;

    std::shared_ptr<const VolumeData> mCurrentVolumeData;
    bool mHasVolume = false;
};
