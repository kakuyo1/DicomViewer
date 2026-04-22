#include "core/worker/SliceImageBuilder.h"

#include <algorithm>

namespace
{

template<typename PixelAccessor> // 像素读取函数
QImage buildSliceImageInternal(int width, int height, const PixelAccessor &pixelAccessor, bool hasPixelPaddingValue, qint16 pixelPaddingValue, bool hasPixelPaddingRangeLimit, qint16 pixelPaddingRangeLimit, const SliceImageBuildOptions &options)
{
    if (width <= 0 || height <= 0) {
        return {};
    }

    // 1.为 mapWindowLevel 的退化策略计算好 sliceMin 和 sliceMax
    qint16 sliceMin = 0;
    qint16 sliceMax = 0;
    bool hasNonPaddingPixel = false;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const qint16 pixelValue = pixelAccessor(x, y);
            // 跳过 padding 像素
            if (isPaddingPixel(pixelValue, hasPixelPaddingValue, pixelPaddingValue, hasPixelPaddingRangeLimit, pixelPaddingRangeLimit)) {
                continue;
            }

            // 初始化 min/max
            if (!hasNonPaddingPixel) {
                sliceMin = pixelValue;
                sliceMax = pixelValue;
                hasNonPaddingPixel = true;
                continue;
            }

            // 更新 min/max
            sliceMin = std::min(sliceMin, pixelValue);
            sliceMax = std::max(sliceMax, pixelValue);
        }
    }

    // 整张图都是 padding
    if (!hasNonPaddingPixel) {
        sliceMin = 0;
        sliceMax = 0;
    }

    // 2.创建输出图像
    QImage image(width, height, QImage::Format_Grayscale8);
    if (image.isNull()) {
        return {};
    }

    for (int y = 0; y < height; ++y) {
        uchar *scanLine = image.scanLine(y);    // 指向第 y 行的内存
        /**
         *  @note DICOM坐标系原点在左上角， 与 Qt 一致
         */
        const int sampleY = options.flipVertical ? (height - 1 - y) : y;
        for (int x = 0; x < width; ++x) {
            const int sampleX = options.flipHorizontal ? (width - 1 - x) : x;
            const qint16 pixelValue = pixelAccessor(sampleX, sampleY);
            unsigned char grayValue = 0;
            // 只有非填充像素才进行窗宽窗位映射
            if (!isPaddingPixel(pixelValue, hasPixelPaddingValue, pixelPaddingValue, hasPixelPaddingRangeLimit, pixelPaddingRangeLimit)) {
                grayValue = mapWindowLevel(pixelValue, options.windowCenter, options.windowWidth, sliceMin, sliceMax);
                if (options.invert) {
                    grayValue = static_cast<unsigned char>(255 - grayValue);
                }
            }
            scanLine[x] = grayValue;
        }
    }

    if (!options.outputSize.isValid()) {
        return image;
    }

    return image.scaled(options.outputSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

} // namespace

bool isPaddingPixel(qint16 value, bool hasPixelPaddingValue, qint16 pixelPaddingValue, bool hasPixelPaddingRangeLimit, qint16 pixelPaddingRangeLimit)
{
    if (!hasPixelPaddingValue) {
        return false;
    }

    if (!hasPixelPaddingRangeLimit) {
        return value == pixelPaddingValue;
    }

    // e.g. pixelPaddingValue = 0, pixelPaddingRangeLimit = 50 => HU [0, 50] is not wantted.
    const qint16 paddingMin = std::min(pixelPaddingValue, pixelPaddingRangeLimit);
    const qint16 paddingMax = std::max(pixelPaddingValue, pixelPaddingRangeLimit);
    return value >= paddingMin && value <= paddingMax;
}

/** @note 把原始的 16 位灰度值/HU（qint16）映射成显示用的 8 位灰度值（unsigned char，范围 0~255）*/
unsigned char mapWindowLevel(qint16 value, double windowCenter, double windowWidth, qint16 sliceMin, qint16 sliceMax)
{
    double lower = 0.0;
    double upper = 0.0;

    if (windowWidth > 1.0) { // 标准方式
        lower = windowCenter - windowWidth * 0.5;
        upper = windowCenter + windowWidth * 0.5;
    } else { // 退化->切片最小值作为黑色, 切片最大值作为白色
        lower = static_cast<double>(sliceMin);
        upper = static_cast<double>(sliceMax);
    }

    if (upper <= lower) {
        return 0;
    }

    const double normalized = (static_cast<double>(value) - lower) / (upper - lower); // [0, 1]
    const double clamped = std::clamp(normalized, 0.0, 1.0);
    return static_cast<unsigned char>(clamped * 255.0); // [0, 255]
}

SliceImageBuildInput buildSliceImageInput(const VolumeData &volumeData, int sliceIndex)
{
    SliceImageBuildInput input;
    if (!volumeData.isValid() || sliceIndex < 0 || sliceIndex >= volumeData.depth) {
        return input;
    }

    const int slicePixelCount = volumeData.width * volumeData.height;
    const int sliceOffset = sliceIndex * slicePixelCount;
    if (sliceOffset < 0 || sliceOffset + slicePixelCount > volumeData.voxels.size()) {
        return input;
    }

    input.width = volumeData.width;
    input.height = volumeData.height;
    input.hasPixelPaddingValue = volumeData.hasPixelPaddingValue;
    input.pixelPaddingValue = volumeData.pixelPaddingValue;
    input.hasPixelPaddingRangeLimit = volumeData.hasPixelPaddingRangeLimit;
    input.pixelPaddingRangeLimit = volumeData.pixelPaddingRangeLimit;
    // 拿到单片图片像素
    input.pixels = QVector<qint16>(volumeData.voxels.begin() + sliceOffset, volumeData.voxels.begin() + sliceOffset + slicePixelCount);
    return input;
}

/// for SliceViewWidget ，直接读 VolumeData，不拷贝，线程不安全
QImage buildSliceImage(const VolumeData &volumeData, int sliceIndex, const SliceImageBuildOptions &options)
{
    if (!volumeData.isValid() || sliceIndex < 0 || sliceIndex >= volumeData.depth) {
        return {};
    }

    const int width = volumeData.width;
    const int height = volumeData.height;
    const int slicePixelCount = width * height;
    const int sliceOffset = sliceIndex * slicePixelCount;
    if (sliceOffset < 0 || sliceOffset + slicePixelCount > volumeData.voxels.size()) {
        return {};
    }

    return buildSliceImageInternal(
        width,
        height,
        [&volumeData, sliceOffset, width](int x, int y) {
            return volumeData.voxels[sliceOffset + y * width + x];
        },
        volumeData.hasPixelPaddingValue,
        volumeData.pixelPaddingValue,
        volumeData.hasPixelPaddingRangeLimit,
        volumeData.pixelPaddingRangeLimit,
        options);
}

/// for thumbnail，拷贝一份VolumeData, 线程安全
QImage buildSliceImage(const SliceImageBuildInput &input, const SliceImageBuildOptions &options)
{
    if (!input.isValid()) {
        return {};
    }

    return buildSliceImageInternal(
        input.width,
        input.height,
        [&input](int x, int y) {
            return input.pixels[y * input.width + x];
        },
        input.hasPixelPaddingValue,
        input.pixelPaddingValue,
        input.hasPixelPaddingRangeLimit,
        input.pixelPaddingRangeLimit,
        options);
}
