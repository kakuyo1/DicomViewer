#pragma once

#include <QVTKOpenGLNativeWidget.h>

#include <vtkSmartPointer.h>

#include "ui/toolbars/StackToolMode.h"

class VolumeData;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkRenderer;
class vtkImageData;
class QWheelEvent;

class SliceViewWidget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT

public:
    explicit SliceViewWidget(QWidget *parent = nullptr);
    ~SliceViewWidget();

    void showAxialSlice(const VolumeData &volumeData, int sliceIndex);
    void clearDisplay();

    void setToolMode(StackToolMode mode);
    void setInvertEnabled(bool enabled);
    void setFlipHorizontalEnabled(bool enabled);
    void setFlipVerticalEnabled(bool enabled);
    void resetViewState();

signals:
    void sliceScrollRequested(int steps);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupVtkPipeline();
    void ensureImageDataAllocated(int width, int height, double spacingX, double spacingY);

private:
    const VolumeData *mCurrentVolumeData = nullptr;
    int mCurrentSliceIndex               = -1;
    StackToolMode mToolMode              = StackToolMode::Pan;
    bool mInvertEnabled                  = false;
    bool mFlipHorizontalEnabled          = false;
    bool mFlipVerticalEnabled            = false;

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenderWindow;
    vtkSmartPointer<vtkRenderer>                  mRenderer;
    vtkSmartPointer<vtkImageActor>                mImageActor;
    vtkSmartPointer<vtkImageData>                 mImageData;
};
