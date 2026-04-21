#include "SliceViewWidget.h"

#include <algorithm>
#include <cmath>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleUser.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

#include "core/model/volume/VolumeData.h"

namespace
{

constexpr double kMinimumInteractiveWindowWidth = 20.0;
constexpr double kMinimumZoomFactor             = 0.1;
constexpr double kMaximumZoomFactor             = 20.0;

/** @note 空操作 interactor style，用来清空原生VTK Interactor的键鼠操作，不能直接disable interactor,因为它包含VTK的事件循环，会直接黑屏！ */
class StackNoOpInteractorStyle : public vtkInteractorStyleUser
{
public:
    static StackNoOpInteractorStyle *New();
    vtkTypeMacro(StackNoOpInteractorStyle, vtkInteractorStyleUser);

    void OnLeftButtonDown() override {}
    void OnLeftButtonUp() override {}
    void OnMiddleButtonDown() override {}
    void OnMiddleButtonUp() override {}
    void OnRightButtonDown() override {}
    void OnRightButtonUp() override {}
    void OnMouseMove() override {}
    void OnMouseWheelForward() override {}
    void OnMouseWheelBackward() override {}
    void OnChar() override {}
    void OnKeyPress() override {}
    void OnKeyRelease() override {}
};

vtkStandardNewMacro(StackNoOpInteractorStyle);

/** @note 把原始的 16 位灰度值/HU（qint16）映射成显示用的 8 位灰度值（unsigned char，范围 0~255）*/
unsigned char mapWindowLevel(qint16 value, double windowCenter, double windowWidth, qint16 sliceMin, qint16 sliceMax)
{
    double lower = 0.0;
    double upper = 0.0;

    if (windowWidth > 1.0) {
        lower = windowCenter - windowWidth * 0.5;
        upper = windowCenter + windowWidth * 0.5;
    } else { // 退化->切片最小值作为黑色,切片最大值作为白色
        lower = static_cast<double>(sliceMin);
        upper = static_cast<double>(sliceMax);
    }

    // 避免除零或错误范围
    if (upper <= lower) {
        return 0;
    }

    const double normalized = (static_cast<double>(value) - lower) / (upper - lower); // [0,1]
    const double clamped    = std::clamp(normalized, 0.0, 1.0);
    return static_cast<unsigned char>(clamped * 255.0); // [0, 255]
}

bool isPaddingPixel(qint16 value, const VolumeData &volumeData)
{
    if (!volumeData.hasPixelPaddingValue) {
        return false;
    }

    if (!volumeData.hasPixelPaddingRangeLimit) {
        return value == volumeData.pixelPaddingValue;
    }

    const qint16 paddingMin = std::min(volumeData.pixelPaddingValue, volumeData.pixelPaddingRangeLimit);
    const qint16 paddingMax = std::max(volumeData.pixelPaddingValue, volumeData.pixelPaddingRangeLimit);
    return value >= paddingMin && value <= paddingMax;
}

} // namespace

SliceViewWidget::SliceViewWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent)
{
    setupVtkPipeline();
}

SliceViewWidget::~SliceViewWidget()
{

}

void SliceViewWidget::showAxialSlice(const VolumeData &volumeData, int sliceIndex)
{
    if (!volumeData.isValid() || sliceIndex < 0 || sliceIndex >= volumeData.depth) {
        clearDisplay();
        return;
    }

    const bool volumeChanged = (mCurrentVolumeData != &volumeData);
    mCurrentVolumeData = &volumeData;
    mCurrentSliceIndex = sliceIndex;

    if (volumeChanged || !mWindowLevelInitialized) {
        resetWindowLevelToDefault();
    }

    if (volumeChanged) {
        mPanOffset              = QPointF(0.0, 0.0);
        mZoomFactor             = 1.0;
        mCameraStateInitialized = false;
    }

    renderCurrentSlice();
}

void SliceViewWidget::renderCurrentSlice()
{
    if (mCurrentVolumeData == nullptr || !mCurrentVolumeData->isValid() || mCurrentSliceIndex < 0
        || mCurrentSliceIndex >= mCurrentVolumeData->depth) {
        clearDisplay();
        return;
    }

    const VolumeData &volumeData = *mCurrentVolumeData;

    const int width           = volumeData.width;
    const int height          = volumeData.height;
    const int slicePixelCount = width * height;
    const int sliceOffset     = mCurrentSliceIndex * slicePixelCount;
    if (sliceOffset < 0 || sliceOffset + slicePixelCount > volumeData.voxels.size()) {
        clearDisplay();
        return;
    }

    // 计算切片在体数据中的偏移位置
    const auto sliceBegin = volumeData.voxels.begin() + sliceOffset;
    const auto sliceEnd   = sliceBegin + slicePixelCount;

    // 找到最小灰度值和最大灰度值 -> 用于mapWindowLevel的回退
    qint16 sliceMin = 0;
    qint16 sliceMax = 0;
    bool hasNonPaddingPixel = false;
    // 排除填充像素影响窗宽窗位
    for (auto it = sliceBegin; it != sliceEnd; ++it) {
        if (isPaddingPixel(*it, volumeData)) {
            continue;
        }

        if (!hasNonPaddingPixel) {
            sliceMin = *it;
            sliceMax = *it;
            hasNonPaddingPixel = true;
            continue;
        }

        sliceMin = std::min(sliceMin, *it);
        sliceMax = std::max(sliceMax, *it);
    }

    if (!hasNonPaddingPixel) {
        sliceMin = 0;
        sliceMax = 0;
    }

    const bool geometryChanged = ensureImageDataAllocated(width, height, volumeData.spacingX, volumeData.spacingY);
    if (geometryChanged) {
        mCameraStateInitialized = false;
    }

    // 填充图片像素
    unsigned char *buffer = static_cast<unsigned char *>(mImageData->GetScalarPointer(0, 0, 0));
    for (int y = 0; y < height; ++y) {
        const int sampleY    = mFlipVerticalEnabled ? (height - 1 - y) : y;
        const qint16 *srcRow = volumeData.voxels.data() + sliceOffset + sampleY * width;
        /**
         *  @note DICOM坐标系原点在左上角
         *        VTK坐标系原点在左下角
         *        翻转Y轴确保图像方向正确（y -> height -1 -y）
         */
        unsigned char *dstRow = buffer + (height - 1 - y) * width;

        for (int x = 0; x < width; ++x) {
            const int sampleX       = mFlipHorizontalEnabled ? (width - 1 - x) : x;
            const qint16 pixelValue = srcRow[sampleX];
            unsigned char grayValue = 0;
            // 只有非填充像素才进行窗宽窗位映射
            if (!isPaddingPixel(pixelValue, volumeData)) {
                grayValue = mapWindowLevel(
                    pixelValue,
                    mCurrentWindowCenter,
                    mCurrentWindowWidth,
                    sliceMin,
                    sliceMax);
                if (mInvertEnabled) {
                    grayValue = static_cast<unsigned char>(255 - grayValue);
                }
            }
            dstRow[x] = grayValue;
        }
    }

    mImageData->Modified();
    mImageActor->SetVisibility(true);

    if (mRenderer != nullptr) {
        vtkCamera *camera = mRenderer->GetActiveCamera();
        if (camera != nullptr) {
            camera->ParallelProjectionOn();
            if (!mCameraStateInitialized) {
                mRenderer->ResetCamera();
                camera->GetFocalPoint(mBaseCameraFocalPoint);
                camera->GetPosition(mBaseCameraPosition);
                mBaseParallelScale      = camera->GetParallelScale();
                mCameraStateInitialized = true;
            }
            applyViewStateToCamera();
        }
    }
    mRenderWindow->Render();
}

void SliceViewWidget::clearDisplay()
{
    mCurrentVolumeData = nullptr;
    mCurrentSliceIndex = -1;
    mMouseDragActive   = false;

    if (mImageActor != nullptr) {
        mImageActor->SetVisibility(false);
    }
    if (mRenderWindow != nullptr) {
        mRenderWindow->Render();
    }
}

void SliceViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event == nullptr) {
        return;
    }

    if (event->button() == Qt::LeftButton && mToolMode == StackToolMode::WindowLevel) {
        mMouseDragActive       = true;
        mMouseDragStartPos     = event->pos();
        mDragStartWindowCenter = mCurrentWindowCenter;
        mDragStartWindowWidth  = std::max(kMinimumInteractiveWindowWidth, mCurrentWindowWidth);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && mToolMode == StackToolMode::Pan) {
        mMouseDragActive    = true;
        mMouseDragStartPos  = event->pos();
        mDragStartPanOffset = mPanOffset;
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && mToolMode == StackToolMode::Zoom) {
        mMouseDragActive     = true;
        mMouseDragStartPos   = event->pos();
        mDragStartZoomFactor = mZoomFactor;
        event->accept();
        return;
    }

    event->accept();
}

void SliceViewWidget::keyPressEvent(QKeyEvent *event)
{
    if (event != nullptr) {
        event->accept();
    }
}

void SliceViewWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event != nullptr) {
        event->accept();
    }
}

void SliceViewWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (event == nullptr) {
        return;
    }

    if (mMouseDragActive && mToolMode == StackToolMode::WindowLevel) {
        const QPoint delta = event->pos() - mMouseDragStartPos;

        constexpr double kWindowWidthSensitivity  = 1.0;
        constexpr double kWindowCenterSensitivity = 1.0;

        mCurrentWindowWidth     = std::max(kMinimumInteractiveWindowWidth, mDragStartWindowWidth + delta.x() * kWindowWidthSensitivity);
        mCurrentWindowCenter    = mDragStartWindowCenter - delta.y() * kWindowCenterSensitivity;
        mWindowLevelInitialized = true;

        renderCurrentSlice();
        event->accept();
        return;
    }

    if (mMouseDragActive && mToolMode == StackToolMode::Pan) {
        const QSize widgetSize = size();
        if (widgetSize.width() > 0 && widgetSize.height() > 0 && mCameraStateInitialized) {
            const QPoint delta = event->pos() - mMouseDragStartPos;
            // 像素位移delta  ->  世界坐标位移
            /** @note  ParallelScale = 可视世界高度的一半  -> 可视世界总高度 = 2 * ParallelScale */
            const double viewHeightWorld = 2.0 * (mBaseParallelScale / std::max(kMinimumZoomFactor, mZoomFactor));
            /** @note 屏幕窗口的宽高比会影响“同样的高度对应多少宽度” */
            const double viewWidthWorld = viewHeightWorld * static_cast<double>(widgetSize.width()) / static_cast<double>(widgetSize.height());
            /** @note viewWorld / widgetSize  ->  每个像素对应多少个世界单位, 并且注意VTK 与Qt 的y轴相反 */
            mPanOffset.setX(mDragStartPanOffset.x() - delta.x() * (viewWidthWorld / static_cast<double>(widgetSize.width())));
            mPanOffset.setY(mDragStartPanOffset.y() + delta.y() * (viewHeightWorld / static_cast<double>(widgetSize.height())));
            applyViewStateToCamera();
            if (mRenderWindow != nullptr) {
                mRenderWindow->Render();
            }
        }
        event->accept();
        return;
    }

    if (mMouseDragActive && mToolMode == StackToolMode::Zoom) {
        const QPoint delta = event->pos() - mMouseDragStartPos;
        const double zoomMultiplier = std::pow(1.01, -delta.y());   /** @note 注意Qt中向下为y正方向 */
        mZoomFactor = std::clamp(mDragStartZoomFactor * zoomMultiplier, kMinimumZoomFactor, kMaximumZoomFactor);
        applyViewStateToCamera();
        if (mRenderWindow != nullptr) {
            mRenderWindow->Render();
        }
        event->accept();
        return;
    }

    event->accept();
}

void SliceViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event == nullptr) {
        return;
    }

    if (event->button() == Qt::LeftButton && mMouseDragActive) {
        mMouseDragActive = false;
        event->accept();
        return;
    }

    event->accept();
}

void SliceViewWidget::wheelEvent(QWheelEvent *event)
{
    if (event == nullptr) {
        return;
    }

    const int deltaY = event->angleDelta().y();
    if (deltaY == 0) {
        event->ignore();
        return;
    }

    int steps = deltaY / 120;
    if (steps == 0) {
        steps = deltaY > 0 ? 1 : -1;
    }

    emit sliceScrollRequested(-steps);
    event->accept();
}

void SliceViewWidget::setToolMode(StackToolMode mode)
{
    mToolMode = mode;
    mMouseDragActive = false;
}

void SliceViewWidget::setInvertEnabled(bool enabled)
{
    mInvertEnabled = enabled;
    renderCurrentSlice();
}

void SliceViewWidget::setFlipHorizontalEnabled(bool enabled)
{
    mFlipHorizontalEnabled = enabled;
    renderCurrentSlice();
}

void SliceViewWidget::setFlipVerticalEnabled(bool enabled)
{
    mFlipVerticalEnabled = enabled;
    renderCurrentSlice();
}

void SliceViewWidget::resetViewState()
{
    mInvertEnabled          = false;
    mFlipHorizontalEnabled  = false;
    mFlipVerticalEnabled    = false;
    mMouseDragActive        = false;
    mPanOffset              = QPointF(0.0, 0.0);
    mZoomFactor             = 1.0;
    mCameraStateInitialized = false;
    resetWindowLevelToDefault();
    renderCurrentSlice();
}

void SliceViewWidget::setupVtkPipeline()
{
    mRenderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    mRenderer     = vtkSmartPointer<vtkRenderer>::New();
    mImageActor   = vtkSmartPointer<vtkImageActor>::New();
    mImageData    = vtkSmartPointer<vtkImageData>::New();

    mRenderer->SetBackground(0.0, 0.0, 0.0);
    mRenderer->AddActor(mImageActor);
    mImageActor->SetInputData(mImageData);
    mImageActor->SetVisibility(false);

    mRenderWindow->AddRenderer(mRenderer);
    setRenderWindow(mRenderWindow);
    installNoOpInteractorStyle();
}

void SliceViewWidget::installNoOpInteractorStyle()
{
    if (mRenderWindow == nullptr) {
        return;
    }

    vtkRenderWindowInteractor *interactor = mRenderWindow->GetInteractor();
    if (interactor == nullptr) {
        return;
    }

    vtkSmartPointer<StackNoOpInteractorStyle> style = vtkSmartPointer<StackNoOpInteractorStyle>::New();
    interactor->SetInteractorStyle(style);
}

bool SliceViewWidget::ensureImageDataAllocated(int width, int height, double spacingX, double spacingY)
{
    int dims[3] = {0, 0, 0};
    double oldSpacing[3] = {1.0, 1.0, 1.0};

    if (mImageData) {
        mImageData->GetDimensions(dims);
        mImageData->GetSpacing(oldSpacing);
    }

    bool needReallocate = false;

    if (!mImageData       ||
        dims[0] != width  ||
        dims[1] != height ||
        dims[2] != 1      ||
        mImageData->GetScalarType() != VTK_UNSIGNED_CHAR) {
        needReallocate = true;
    }

    if (needReallocate) {
        mImageData->Initialize();
        mImageData->SetDimensions(width, height, 1);
        mImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
    }

    const bool spacingChanged = (oldSpacing[0] != spacingX || oldSpacing[1] != spacingY);
    if (spacingChanged) {
        mImageData->SetSpacing(spacingX, spacingY, 1.0);
    }

    return needReallocate || spacingChanged;
}

void SliceViewWidget::applyViewStateToCamera()
{
    if (mRenderer == nullptr || !mCameraStateInitialized) {
        return;
    }

    vtkCamera *camera = mRenderer->GetActiveCamera();
    if (camera == nullptr) {
        return;
    }

    const double zoomFactor = std::clamp(mZoomFactor, kMinimumZoomFactor, kMaximumZoomFactor);
    camera->SetFocalPoint(
        mBaseCameraFocalPoint[0] + mPanOffset.x(),
        mBaseCameraFocalPoint[1] + mPanOffset.y(),
        mBaseCameraFocalPoint[2]);
    camera->SetPosition(
        mBaseCameraPosition[0] + mPanOffset.x(),
        mBaseCameraPosition[1] + mPanOffset.y(),
        mBaseCameraPosition[2]);
    camera->SetParallelScale(mBaseParallelScale / std::max(kMinimumZoomFactor, mZoomFactor));
    camera->ParallelProjectionOn();         // 防止ResetCamera又改回透视投影
    mRenderer->ResetCameraClippingRange();  // 自动调整裁剪范围，使得所有物体都能显示在视图中
}

void SliceViewWidget::resetWindowLevelToDefault()
{
    if (mCurrentVolumeData == nullptr) {
        mCurrentWindowCenter    = 0.0;
        mCurrentWindowWidth     = 0.0;
        mWindowLevelInitialized = false;
        return;
    }

    mCurrentWindowCenter    = mCurrentVolumeData->windowCenter;
    mCurrentWindowWidth     = mCurrentVolumeData->windowWidth;
    mWindowLevelInitialized = true;
}
