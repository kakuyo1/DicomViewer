#pragma once

#include <QVTKOpenGLNativeWidget.h>

#include <QLineF>
#include <QPoint>
#include <QPointF>

#include <vtkSmartPointer.h>

#include "ui/toolbars/StackToolMode.h"

class VolumeData;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkRenderer;
class vtkImageData;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPaintEvent;
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
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void setupVtkPipeline();
    void installNoOpInteractorStyle();
    bool ensureImageDataAllocated(int width, int height, double spacingX, double spacingY);
    void applyViewStateToCamera();
    void clearMeasurement();
    bool widgetPointToImagePoint(const QPoint &widgetPoint, QPointF *imagePoint) const;
    QPointF imagePointToWidgetPoint(const QPointF &imagePoint) const;
    void updateMeasurementLabel(const QPointF &startPoint, const QPointF &endPoint, double distanceMm);
    void renderCurrentSlice();
    void resetWindowLevelToDefault();

private:
    const VolumeData *mCurrentVolumeData = nullptr;
    int mCurrentSliceIndex               = -1;

    StackToolMode mToolMode              = StackToolMode::Pan;

    bool mInvertEnabled                  = false;

    bool mFlipHorizontalEnabled          = false;
    bool mFlipVerticalEnabled            = false;

    double mCurrentWindowCenter          = 0.0;
    double mCurrentWindowWidth           = 0.0;
    double mDragStartWindowCenter        = 0.0;
    double mDragStartWindowWidth         = 0.0;
    bool mWindowLevelInitialized         = false;

    bool mMouseDragActive                = false;
    QPoint mMouseDragStartPos;
    QPointF mPanOffset;
    QPointF mDragStartPanOffset;
    double mZoomFactor                   = 1.0;
    double mDragStartZoomFactor          = 1.0;

    bool mCameraStateInitialized         = false;
    double mBaseCameraFocalPoint[3]      = {0.0, 0.0, 0.0};
    double mBaseCameraPosition[3]        = {0.0, 0.0, 1.0};
    double mBaseParallelScale            = 1.0;                 // 平行投影下的 ParallelScale：可视世界高度的一半

    bool mMeasurementVisible             = false;
    bool mMeasurementDragging            = false;
    QLineF mMeasurementLine;                                    // 保存的是图像物理坐标（如果保存的是屏幕坐标，一旦屏幕尺寸变化等，线的位置就乱了）
    QLabel *mMeasurementLabel            = nullptr;             // 用label保存文字，因为不知道为什么直接在paintEvent中绘制，文字会乱码

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> mRenderWindow;
    vtkSmartPointer<vtkRenderer>                  mRenderer;
    vtkSmartPointer<vtkImageActor>                mImageActor;
    vtkSmartPointer<vtkImageData>                 mImageData;
};
