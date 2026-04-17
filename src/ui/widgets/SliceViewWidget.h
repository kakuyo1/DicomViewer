#pragma once

#include <QVTKOpenGLNativeWidget.h>

#include <vtkSmartPointer.h>

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

signals:
    void sliceScrollRequested(int steps);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupVtkPipeline();
    void ensureImageDataAllocated(int width, int height, double spacingX, double spacingY);

private:
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenderWindow;
    vtkSmartPointer<vtkRenderer>                  mRenderer;
    vtkSmartPointer<vtkImageActor>                mImageActor;
    vtkSmartPointer<vtkImageData>                 mImageData;
};
