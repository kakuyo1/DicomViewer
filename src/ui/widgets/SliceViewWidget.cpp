#include "SliceViewWidget.h"

#include <algorithm>

#include <QWheelEvent>

#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>

#include "core/model/volume/VolumeData.h"

namespace
{

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
    const double clamped = std::clamp(normalized, 0.0, 1.0);
    return static_cast<unsigned char>(clamped * 255.0); // [0, 255]
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

    const int width           = volumeData.width;
    const int height          = volumeData.height;
    const int slicePixelCount = width * height;
    const int sliceOffset     = sliceIndex * slicePixelCount;
    if (sliceOffset < 0 || sliceOffset + slicePixelCount > volumeData.voxels.size()) {
        clearDisplay();
        return;
    }

    // 计算切片在体数据中的偏移位置
    const auto sliceBegin = volumeData.voxels.begin() + sliceOffset;
    const auto sliceEnd   = sliceBegin + slicePixelCount;

    // 找到最小灰度值和最大灰度值 -> 用于mapWindowLevel的回退
    const auto [sliceMinIt, sliceMaxIt] = std::minmax_element(sliceBegin, sliceEnd);
    const qint16 sliceMin = (sliceMinIt != sliceEnd) ? *sliceMinIt : 0;
    const qint16 sliceMax = (sliceMaxIt != sliceEnd) ? *sliceMaxIt : 0;

    ensureImageDataAllocated(width, height, volumeData.spacingX, volumeData.spacingY);

    // 填充图片像素
    unsigned char *buffer = static_cast<unsigned char *>(mImageData->GetScalarPointer(0, 0, 0));
    for (int y = 0; y < height; ++y) {
        const qint16 *srcRow = volumeData.voxels.data() + sliceOffset + y * width;
        /**
         *  @note DICOM坐标系原点在左上角
         *        VTK坐标系原点在左下角
         *        翻转Y轴确保图像方向正确（y -> height -1 -y）
         */
        unsigned char *dstRow = buffer + (height - 1 - y) * width;

        for (int x = 0; x < width; ++x) {
            dstRow[x] = mapWindowLevel(
                srcRow[x],
                volumeData.windowCenter,
                volumeData.windowWidth,
                sliceMin,
                sliceMax);
        }
    }

    mImageData->Modified();
    mImageActor->SetVisibility(true);
    mRenderer->ResetCamera();
    /** @note 开启平行投影，避免透视变形 */
    mRenderer->GetActiveCamera()->ParallelProjectionOn();
    mRenderWindow->Render();
}

void SliceViewWidget::clearDisplay()
{
    if (mImageActor != nullptr) {
        mImageActor->SetVisibility(false);
    }
    if (mRenderWindow != nullptr) {
        mRenderWindow->Render();
    }
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
}

void SliceViewWidget::ensureImageDataAllocated(int width, int height, double spacingX, double spacingY)
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

    if (oldSpacing[0] != spacingX || oldSpacing[1] != spacingY) {
        mImageData->SetSpacing(spacingX, spacingY, 1.0);
    }
}
