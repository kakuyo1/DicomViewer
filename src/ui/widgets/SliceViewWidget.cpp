#include "SliceViewWidget.h"

#include <cmath>

#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>

#include <vtkCamera.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleUser.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"
#include "core/worker/SliceImageBuilder.h"
#include "common/Util.h"
#include "ui/model/OverlayInfo.h"
#include "ui/widgets/ImageOverlayWidget.h"

namespace
{

constexpr double kMinimumInteractiveWindowWidth = 20.0;
constexpr double kMinimumZoomFactor             = 0.1;
constexpr double kMaximumZoomFactor             = 20.0;
constexpr double kMeasurementTickSpacingMm      = 20.0;

/** @note 空操作 interactor style，用来清空原生VTK Interactor的键鼠操作，不能直接disable interactor,因为它包含VTK的事件循环，会直接黑屏！ */
class StackNoOpInteractorStyle : public vtkInteractorStyleUser
{
public:
    static StackNoOpInteractorStyle *New();
    vtkTypeMacro(StackNoOpInteractorStyle, vtkInteractorStyleUser);

    void OnLeftButtonDown() override
    {}
    void OnLeftButtonUp() override
    {}
    void OnMiddleButtonDown() override
    {}
    void OnMiddleButtonUp() override
    {}
    void OnRightButtonDown() override
    {}
    void OnRightButtonUp() override
    {}
    void OnMouseMove() override
    {}
    void OnMouseWheelForward() override
    {}
    void OnMouseWheelBackward() override
    {}
    void OnChar() override
    {}
    void OnKeyPress() override
    {}
    void OnKeyRelease() override
    {}
};

vtkStandardNewMacro(StackNoOpInteractorStyle);

} // namespace

SliceViewWidget::SliceViewWidget(QWidget *parent)
    : QVTKOpenGLNativeWidget(parent)
{
    setupVtkPipeline();

    mImageOverlayWidget = new ImageOverlayWidget(this);
    mImageOverlayWidget->setGeometry(rect());
    mImageOverlayWidget->show();

    mMeasurementLabel = new QLabel(this);
    mMeasurementLabel->hide();
    mMeasurementLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    mMeasurementLabel->setStyleSheet(QStringLiteral(
        "color: rgb(255, 255, 0);"
        "background: transparent;"
        "font-size: 13px;"
        "font-family: Sans Serif;"));
}

SliceViewWidget::~SliceViewWidget()
{
}

void SliceViewWidget::showAxialSlice(const DicomSeries &series, const VolumeData &volumeData, int sliceIndex)
{
    showSlice(series, volumeData, SliceOrientation::Axial, sliceIndex);
}

void SliceViewWidget::showSlice(const DicomSeries &series, const VolumeData &volumeData, SliceOrientation orientation, int sliceIndex)
{
    if (!volumeData.isValid() || sliceIndex < 0 || sliceIndex >= util::sliceCountForOrientation(volumeData, orientation)) {
        clearDisplay();
        return;
    }

    mCurrentSeries                = &series;
    const bool seriesChanged      = (mCurrentSeriesInstanceUid != series.seriesInstanceUid);
    const bool volumeChanged      = seriesChanged || (mCurrentVolumeData != &volumeData);
    const bool orientationChanged = (mCurrentOrientation != orientation);
    const bool sliceChanged       = orientationChanged || (mCurrentSliceIndex != sliceIndex);
    mCurrentSeriesInstanceUid     = series.seriesInstanceUid;
    mCurrentVolumeData            = &volumeData;
    mCurrentOrientation           = orientation;
    mCurrentSliceIndex            = sliceIndex;

    if (volumeChanged || !mWindowLevelInitialized) {
        resetWindowLevelToDefault();
        emitDisplayParametersChanged(); // 更新缩略图
    }

    if (volumeChanged || orientationChanged) {
        mPanOffset              = QPointF(0.0, 0.0);
        mZoomFactor             = 1.0;
        mCameraStateInitialized = false;
    }
    if (volumeChanged || sliceChanged) {
        clearMeasurement();
    }

    renderCurrentSlice();
}

void SliceViewWidget::renderCurrentSlice()
{
    if (mCurrentVolumeData == nullptr || !mCurrentVolumeData->isValid() || mCurrentSliceIndex < 0 || mCurrentSliceIndex >= util::sliceCountForOrientation(*mCurrentVolumeData, mCurrentOrientation)) {
        clearDisplay();
        return;
    }

    const VolumeData &volumeData = *mCurrentVolumeData;

    // 确定 slice 方位的四件套：
    const util::SlicePlaneGeometry geometry = util::sliceGeometryForOrientation(volumeData, mCurrentOrientation);
    const int width                         = geometry.width;
    const int height                        = geometry.height;
    const double spacingX                   = geometry.spacingX;
    const double spacingY                   = geometry.spacingY;
    if (width <= 0 || height <= 0) {
        clearDisplay();
        return;
    }

    const bool geometryChanged = ensureImageDataAllocated(width, height, spacingX, spacingY);
    if (geometryChanged) {
        mCameraStateInitialized = false;
    }

    mCurrentImageWidth    = width;
    mCurrentImageHeight   = height;
    mCurrentImageSpacingX = spacingX;
    mCurrentImageSpacingY = spacingY;

    SliceImageBuildOptions buildOptions;
    buildOptions.windowCenter   = mCurrentWindowCenter;
    buildOptions.windowWidth    = mCurrentWindowWidth;
    buildOptions.invert         = mInvertEnabled;
    buildOptions.flipHorizontal = mFlipHorizontalEnabled;
    buildOptions.flipVertical   = mFlipVerticalEnabled;

    const QImage image = buildSliceImage(volumeData, mCurrentOrientation, mCurrentSliceIndex, buildOptions);
    if (image.isNull()) {
        clearDisplay();
        return;
    }

    // 填充图片像素
    unsigned char *buffer = static_cast<unsigned char *>(mImageData->GetScalarPointer(0, 0, 0));
    for (int y = 0; y < height; ++y) {
        const uchar *srcRow = image.constScanLine(y);
        /**
         *  @note DICOM坐标系原点在左上角
         *        VTK坐标系原点在左下角
         *        翻转Y轴确保图像方向正确（y -> height -1 -y）
         */
        unsigned char *dstRow = buffer + (height - 1 - y) * width;
        std::copy_n(srcRow, width, dstRow);
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
    updateOverlayInfo();
    mRenderWindow->Render();
}

void SliceViewWidget::clearDisplay()
{
    mCurrentSeries = nullptr;
    mCurrentSeriesInstanceUid.clear();
    mCurrentVolumeData  = nullptr;
    mCurrentSliceIndex  = -1;
    mCurrentImageWidth  = 0;
    mCurrentImageHeight = 0;
    mMouseDragActive    = false;
    clearMeasurement();
    if (mImageOverlayWidget != nullptr) {
        mImageOverlayWidget->clearOverlay();
    }

    if (mImageActor != nullptr) {
        mImageActor->SetVisibility(false);
    }
    if (mRenderWindow != nullptr) {
        mRenderWindow->Render();
    }
}

void SliceViewWidget::resizeEvent(QResizeEvent *event)
{
    QVTKOpenGLNativeWidget::resizeEvent(event);

    if (mImageOverlayWidget != nullptr) {
        mImageOverlayWidget->setGeometry(rect());
    }
}

void SliceViewWidget::paintEvent(QPaintEvent *event)
{
    QVTKOpenGLNativeWidget::paintEvent(event);

    if (!mMeasurementVisible || mCurrentVolumeData == nullptr || !mCurrentVolumeData->isValid()) {
        if (mMeasurementLabel != nullptr) {
            mMeasurementLabel->hide();
        }
        return;
    }

    /** @note 最终绘制时要用像素坐标 */
    const QPointF startPoint = imagePointToWidgetPoint(mMeasurementLine.p1());
    const QPointF endPoint   = imagePointToWidgetPoint(mMeasurementLine.p2());
    if (!std::isfinite(startPoint.x()) || !std::isfinite(startPoint.y()) || !std::isfinite(endPoint.x()) || !std::isfinite(endPoint.y())) {
        return;
    }

    const double distanceMm = std::hypot(mMeasurementLine.dx(), mMeasurementLine.dy());
    const QLineF screenLine(startPoint, endPoint);
    const double lineLength = screenLine.length();

    // 画主测量线
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    QPen linePen(QColor(255, 215, 0));
    linePen.setWidth(2);
    linePen.setCosmetic(true);
    painter.setPen(linePen);
    painter.drawLine(startPoint, endPoint);

    if (lineLength > 1e-3) {
        // 画两端刻度线
        const QPointF    direction = (endPoint - startPoint) / lineLength;
        const QPointF    normal(-direction.y(), direction.x());
        constexpr double kEndTickHalfLength = 7.0;
        painter.drawLine(startPoint - normal * kEndTickHalfLength, startPoint + normal * kEndTickHalfLength);
        painter.drawLine(endPoint - normal * kEndTickHalfLength, endPoint + normal * kEndTickHalfLength);

        // 画中间刻度
        constexpr double kInnerTickHalfLength = 4.0;
        if (distanceMm >= kMeasurementTickSpacingMm) {
            const int tickCount = static_cast<int>(std::floor(distanceMm / kMeasurementTickSpacingMm));
            for (int tickIndex = 1; tickIndex < tickCount; ++tickIndex) {
                const double  distanceRatio = (tickIndex * kMeasurementTickSpacingMm) / distanceMm;
                const QPointF tickCenter    = startPoint + (endPoint - startPoint) * distanceRatio;
                painter.drawLine(tickCenter - normal * kInnerTickHalfLength, tickCenter);
            }
        }
    }

    updateMeasurementLabel(startPoint, endPoint, distanceMm);
}

void SliceViewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event == nullptr) {
        return;
    }

    emit activated();

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

    if (event->button() == Qt::LeftButton && mToolMode == StackToolMode::Measure) {
        QPointF imagePoint;
        if (widgetPointToImagePoint(event->pos(), &imagePoint)) {
            mMeasurementLine     = QLineF(imagePoint, imagePoint);
            mMeasurementVisible  = true;
            mMeasurementDragging = true;
            update();
        }
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

        emitDisplayParametersChanged();
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
        const QPoint delta          = event->pos() - mMouseDragStartPos;
        const double zoomMultiplier = std::pow(1.01, -delta.y()); /** @note 注意Qt中向下为y正方向 */
        mZoomFactor                 = std::clamp(mDragStartZoomFactor * zoomMultiplier, kMinimumZoomFactor, kMaximumZoomFactor);
        applyViewStateToCamera();
        updateOverlayInfo();
        if (mRenderWindow != nullptr) {
            mRenderWindow->Render();
        }
        event->accept();
        return;
    }

    if (mMeasurementDragging && mToolMode == StackToolMode::Measure) {
        QPointF imagePoint;
        if (widgetPointToImagePoint(event->pos(), &imagePoint)) {
            mMeasurementLine.setP2(imagePoint);
            update();
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

    if (event->button() == Qt::LeftButton && mMeasurementDragging) {
        mMeasurementDragging = false;
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
    if (mToolMode == StackToolMode::Measure && mode != StackToolMode::Measure) {
        clearMeasurement();
    }
    mToolMode            = mode;
    mMouseDragActive     = false;
    mMeasurementDragging = false;
}

void SliceViewWidget::setInvertEnabled(bool enabled)
{
    mInvertEnabled = enabled;
    emitDisplayParametersChanged();
    renderCurrentSlice();
}

void SliceViewWidget::setFlipHorizontalEnabled(bool enabled)
{
    mFlipHorizontalEnabled = enabled;
    emitDisplayParametersChanged();
    clearMeasurement();
    renderCurrentSlice();
}

void SliceViewWidget::setFlipVerticalEnabled(bool enabled)
{
    mFlipVerticalEnabled = enabled;
    emitDisplayParametersChanged();
    clearMeasurement();
    renderCurrentSlice();
}

void SliceViewWidget::resetViewState(bool renderImmediately)
{
    mInvertEnabled          = false;
    mFlipHorizontalEnabled  = false;
    mFlipVerticalEnabled    = false;
    mMouseDragActive        = false;
    mPanOffset              = QPointF(0.0, 0.0);
    mZoomFactor             = 1.0;
    mCameraStateInitialized = false;
    clearMeasurement();
    resetWindowLevelToDefault();
    emitDisplayParametersChanged();
    if (renderImmediately) {
        renderCurrentSlice();
    }
}

void SliceViewWidget::emitDisplayParametersChanged()
{
    emit displayParametersChanged(
        mCurrentWindowCenter,
        mCurrentWindowWidth,
        mInvertEnabled,
        mFlipHorizontalEnabled,
        mFlipVerticalEnabled);
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
    int    dims[3]       = {0, 0, 0};
    double oldSpacing[3] = {1.0, 1.0, 1.0};

    if (mImageData) {
        mImageData->GetDimensions(dims);
        mImageData->GetSpacing(oldSpacing);
    }

    bool needReallocate = false;

    if (!mImageData ||
        dims[0] != width ||
        dims[1] != height ||
        dims[2] != 1 ||
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
    camera->ParallelProjectionOn();        // 防止ResetCamera又改回透视投影
    mRenderer->ResetCameraClippingRange(); // 自动调整裁剪范围，使得所有物体都能显示在视图中
}

void SliceViewWidget::clearMeasurement()
{
    mMeasurementVisible  = false;
    mMeasurementDragging = false;
    mMeasurementLine     = QLineF();
    if (mMeasurementLabel != nullptr) {
        mMeasurementLabel->hide();
    }
    update();
}

void SliceViewWidget::updateMeasurementLabel(const QPointF &startPoint, const QPointF &endPoint, double distanceMm)
{
    if (mMeasurementLabel == nullptr) {
        return;
    }

    mMeasurementLabel->setText(QString::number(distanceMm, 'f', 2) + QStringLiteral(" mm"));
    mMeasurementLabel->adjustSize();

    const QPointF midPoint = 0.5 * (startPoint + endPoint);
    // 偏右上一点
    const int labelX = static_cast<int>(std::lround(midPoint.x() + 8.0));
    const int labelY = static_cast<int>(std::lround(midPoint.y() - mMeasurementLabel->height() - 4.0));
    mMeasurementLabel->move(labelX, labelY);
    mMeasurementLabel->show();
    mMeasurementLabel->raise();
}

void SliceViewWidget::updateOverlayInfo()
{
    if (mImageOverlayWidget == nullptr) {
        return;
    }

    if (mCurrentSeries == nullptr || mCurrentVolumeData == nullptr || !mCurrentVolumeData->isValid() || mCurrentSeries->slices.isEmpty() || mCurrentSliceIndex < 0 || mCurrentSliceIndex >= util::sliceCountForOrientation(*mCurrentVolumeData, mCurrentOrientation)) {
        mImageOverlayWidget->clearOverlay();
        return;
    }

    const int             axialSliceIndex = (mCurrentOrientation == SliceOrientation::Axial && mCurrentSliceIndex < mCurrentSeries->slices.size())
                                                ? mCurrentSliceIndex
                                                : std::clamp(mCurrentVolumeData->depth / 2, 0, static_cast<int>(mCurrentSeries->slices.size()) - 1);
    const DicomSliceInfo &sliceInfo       = mCurrentSeries->slices.at(axialSliceIndex);
    OverlayInfo           overlayInfo;

    if (!sliceInfo.patientName.isEmpty()) {
        overlayInfo.topLeftLines.push_back(sliceInfo.patientName);
    } // 匿名化后为[Anonymized^^]
    if (!sliceInfo.patientId.isEmpty()) {
        overlayInfo.topLeftLines.push_back(QStringLiteral("ID: %1").arg(sliceInfo.patientId));
    } // 匿名化后为[0]
    if (!sliceInfo.patientBirthDate.isEmpty()) {
        overlayInfo.topLeftLines.push_back(sliceInfo.patientBirthDate);
    } // 匿名化后可能不显示
    const QString sexAgeText = util::buildSexAgeText(sliceInfo);
    if (!sexAgeText.isEmpty()) {
        overlayInfo.topLeftLines.push_back(sexAgeText);
    }

    if (!sliceInfo.studyDate.isEmpty()) {
        overlayInfo.topRightLines.push_back(util::formatDicomDate(sliceInfo.studyDate));
    }
    if (!sliceInfo.studyTime.isEmpty()) {
        overlayInfo.topRightLines.push_back(util::formatDicomTime(sliceInfo.studyTime));
    }
    if (!mCurrentVolumeData->modality.isEmpty() && mCurrentVolumeData->width && mCurrentVolumeData->height) {
        overlayInfo.topRightLines.push_back(QStringLiteral("%1 [%2x%3]")
                                                .arg(mCurrentVolumeData->modality)
                                                .arg(mCurrentVolumeData->width)
                                                .arg(mCurrentVolumeData->height));
    }
    if (sliceInfo.hasKvp || sliceInfo.hasTubeCurrentMa) {
        QStringList technicalParts;
        if (sliceInfo.hasKvp) {
            technicalParts.push_back(QStringLiteral("%1 kVp").arg(sliceInfo.kvp, 0, 'f', 0));
        }
        if (sliceInfo.hasTubeCurrentMa) {
            technicalParts.push_back(QStringLiteral("%1 mA").arg(sliceInfo.tubeCurrentMa, 0, 'f', 0));
        }
        overlayInfo.topRightLines.push_back(technicalParts.join(QStringLiteral(" / ")));
    }

    QString seriesText;
    if (!mCurrentSeries->seriesDescription.isEmpty()) {
        seriesText = mCurrentSeries->seriesDescription;
    } else if (sliceInfo.hasSeriesNumber) {
        seriesText = QStringLiteral("Series %1").arg(sliceInfo.seriesNumber);
    }
    if (!seriesText.isEmpty()) {
        overlayInfo.bottomLeftLines.push_back(seriesText);
    }
    const int orientationSliceCount = util::sliceCountForOrientation(*mCurrentVolumeData, mCurrentOrientation);
    if (mCurrentOrientation == SliceOrientation::Axial && sliceInfo.hasInstanceNumber) {
        overlayInfo.bottomLeftLines.push_back(QStringLiteral("Image %1 / %2").arg(sliceInfo.instanceNumber).arg(mCurrentVolumeData->depth));
    } else {
        overlayInfo.bottomLeftLines.push_back(QStringLiteral("%1 %2 / %3")
                                                  .arg(util::orientationText(mCurrentOrientation))
                                                  .arg(mCurrentSliceIndex + 1)
                                                  .arg(orientationSliceCount));
    }
    const QString slicePositionText = (mCurrentOrientation == SliceOrientation::Axial)
                                          ? util::formatSlicePositionText(sliceInfo)
                                          : QString();
    if (!slicePositionText.isEmpty()) {
        overlayInfo.bottomLeftLines.push_back(slicePositionText);
    }

    overlayInfo.bottomRightLines.push_back(
        QStringLiteral("WW/WL: %1 / %2").arg(mCurrentWindowWidth, 0, 'f', 1).arg(mCurrentWindowCenter, 0, 'f', 1));
    if (sliceInfo.hasSliceThickness) {
        overlayInfo.bottomRightLines.push_back(QStringLiteral("Thickness: %1 mm").arg(sliceInfo.sliceThickness, 0, 'f', 2));
    }
    overlayInfo.bottomRightLines.push_back(QStringLiteral("Zoom: %1x").arg(mZoomFactor, 0, 'f', 2));

    mImageOverlayWidget->setOverlayInfo(overlayInfo);
}

/**
 *  @note 将 Qt 屏幕像素坐标转换为 VTK 图像的世界坐标
 *
 *  vtkImageData::SetSpacing(spacingX, spacingY, 1.0)
 *    ↓
 *  图像在 VTK 世界中的尺寸单位变成 spacing 对应的物理长度
 *    ↓
 *  camera / actor / renderer 都工作在这个世界坐标系里
 *    ↓
 *  widgetPointToImagePoint() 算出来的是世界坐标
 *    ↓
 *  因此 worldX / worldY 的单位也就是 spacing 对应的物理单位（通常 mm）
 */
bool SliceViewWidget::widgetPointToImagePoint(const QPoint &widgetPoint, QPointF *imagePoint) const
{
    if (imagePoint == nullptr || mCurrentVolumeData == nullptr || !mCurrentVolumeData->isValid() || mRenderer == nullptr) {
        return false;
    }

    vtkCamera  *camera     = mRenderer->GetActiveCamera();
    const QSize widgetSize = size();
    if (camera == nullptr || widgetSize.width() <= 0 || widgetSize.height() <= 0) {
        return false;
    }

    const double spacingX = mCurrentImageSpacingX;
    const double spacingY = mCurrentImageSpacingY;
    const double maxX     = std::max(0.0, static_cast<double>(mCurrentImageWidth - 1) * spacingX);
    const double maxY     = std::max(0.0, static_cast<double>(mCurrentImageHeight - 1) * spacingY);

    const double viewHeightWorld = 2.0 * camera->GetParallelScale();
    const double viewWidthWorld  = viewHeightWorld * static_cast<double>(widgetSize.width()) / static_cast<double>(widgetSize.height());

    double focalPoint[3] = {0.0, 0.0, 0.0};
    camera->GetFocalPoint(focalPoint);

    /**
     *  @note focalPoint[0] - 0.5 * viewWidthWorld   -> 世界范围的左边界
     *        focalPoint[1] + 0.5 * viewHeightWorld  -> 世界范围的上边界
     */
    const double worldX = focalPoint[0] - 0.5 * viewWidthWorld + static_cast<double>(widgetPoint.x()) * viewWidthWorld / static_cast<double>(widgetSize.width());
    const double worldY = focalPoint[1] + 0.5 * viewHeightWorld - static_cast<double>(widgetPoint.y()) * viewHeightWorld / static_cast<double>(widgetSize.height());
    if (worldX < 0.0 || worldX > maxX || worldY < 0.0 || worldY > maxY) {
        return false;
    }

    *imagePoint = QPointF(worldX, worldY);
    return true;
}

QPointF SliceViewWidget::imagePointToWidgetPoint(const QPointF &imagePoint) const
{
    if (mRenderer == nullptr) {
        return QPointF();
    }

    vtkCamera  *camera     = mRenderer->GetActiveCamera();
    const QSize widgetSize = size();
    if (camera == nullptr || widgetSize.width() <= 0 || widgetSize.height() <= 0) {
        return QPointF();
    }

    const double viewHeightWorld = 2.0 * camera->GetParallelScale();
    const double viewWidthWorld  = viewHeightWorld * static_cast<double>(widgetSize.width()) / static_cast<double>(widgetSize.height());

    double focalPoint[3] = {0.0, 0.0, 0.0};
    camera->GetFocalPoint(focalPoint);

    const double widgetX = (imagePoint.x() - (focalPoint[0] - 0.5 * viewWidthWorld)) * static_cast<double>(widgetSize.width()) / viewWidthWorld;
    const double widgetY = ((focalPoint[1] + 0.5 * viewHeightWorld) - imagePoint.y()) * static_cast<double>(widgetSize.height()) / viewHeightWorld;
    return QPointF(widgetX, widgetY);
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
