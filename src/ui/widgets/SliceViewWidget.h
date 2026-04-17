#pragma once

#include <QVTKOpenGLNativeWidget.h>

#include <vtkSmartPointer.h>

class VolumeData;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkRenderer;

class SliceViewWidget : public QVTKOpenGLNativeWidget
{
    Q_OBJECT

public:
    explicit SliceViewWidget(QWidget *parent = nullptr);
    ~SliceViewWidget();

    void showAxialSlice(const VolumeData &volumeData, int sliceIndex);
    void clearDisplay();

private:
    void setupVtkPipeline();

private:
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenderWindow;
    vtkSmartPointer<vtkRenderer>                  mRenderer;
    vtkSmartPointer<vtkImageActor>                mImageActor;
};
