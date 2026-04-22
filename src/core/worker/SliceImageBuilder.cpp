#include "core/worker/SliceImageBuilder.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace
{

using WindowLevelLut = std::array<unsigned char, 65536>;

template<typename PixelAccessor> // 像素读取函数
QImage buildSliceImageInternal(int width, int height, const PixelAccessor &pixelAccessor, bool hasPixelPaddingValue, qint16 pixelPaddingValue, bool hasPixelPaddingRangeLimit, qint16 pixelPaddingRangeLimit, const SliceImageBuildOptions &options)
{
    if (width <= 0 || height <= 0) {
        return {};
    }

    // 1.为当前切片构建一次窗宽窗位 LUT，后续每个像素只做数组查表
    const WindowLevelLut lut = [&]() {
        WindowLevelLut table{};

        double lower = 0.0;
        double upper = 0.0;
        if (options.windowWidth > 1.0) { // 标准方式
            lower = options.windowCenter - options.windowWidth * 0.5;
            upper = options.windowCenter + options.windowWidth * 0.5;
        } else { // 退化->使用最小可用窗宽，避免再扫描整张切片
            lower = options.windowCenter - 0.5;
            upper = options.windowCenter + 0.5;
        }

        if (upper <= lower) {
            table.fill(0);
            return table;
        }

        const double denominator = upper - lower;
        for (int i = 0; i < static_cast<int>(table.size()); ++i) {
            /** @note i 从 0 到 65535，覆盖所有16位整数的位模式
                      rawIndex 将 i 视为无符号16位整数的数值 */
            const quint16 rawIndex = static_cast<quint16>(i);
            qint16 value = 0;
            std::memcpy(&value, &rawIndex, sizeof(value)); // unsigned -> signed

            const double normalized = (static_cast<double>(value) - lower) / denominator;
            const double clamped    = std::clamp(normalized, 0.0, 1.0);
            table[rawIndex] = static_cast<unsigned char>(clamped * 255.0);
        }

        return table;
    }();

    const bool hasPadding     = hasPixelPaddingValue;
    const qint16 paddingValue = pixelPaddingValue;
    const qint16 paddingMin   = hasPixelPaddingRangeLimit ? std::min(pixelPaddingValue, pixelPaddingRangeLimit) : 0;
    const qint16 paddingMax   = hasPixelPaddingRangeLimit ? std::max(pixelPaddingValue, pixelPaddingRangeLimit) : 0;

    // 2.创建输出图像
    QImage image(width, height, QImage::Format_Grayscale8);
    if (image.isNull()) {
        return {};
    }

    if (!hasPadding) { /// no padding
        for (int y = 0; y < height; ++y) {
            uchar *scanLine = image.scanLine(y);    // 指向第 y 行的内存
            /**
             *  @note DICOM坐标系原点在左上角， 与 Qt 一致
             */
            const int sampleY = options.flipVertical ? (height - 1 - y) : y;
            for (int x = 0; x < width; ++x) {
                const int sampleX = options.flipHorizontal ? (width - 1 - x) : x;
                // 所有像素都通过LUT映射
                unsigned char grayValue = lut[static_cast<quint16>(pixelAccessor(sampleX, sampleY))];
                if (options.invert) {
                    grayValue = static_cast<unsigned char>(255 - grayValue);
                }
                scanLine[x] = grayValue;
            }
        }
    } else if (!hasPixelPaddingRangeLimit) { /// single padding value
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
                // 只有不等于填充值的像素才映射
                if (pixelValue != paddingValue) {
                    grayValue = lut[static_cast<quint16>(pixelValue)];
                    if (options.invert) {
                        grayValue = static_cast<unsigned char>(255 - grayValue);
                    }
                }
                scanLine[x] = grayValue;
            }
        }
    } else { /// padding range
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
                // 像素值在[paddingMin, paddingMax]范围内视为填充
                if (pixelValue < paddingMin || pixelValue > paddingMax) {
                    grayValue = lut[static_cast<quint16>(pixelValue)];
                    if (options.invert) {
                        grayValue = static_cast<unsigned char>(255 - grayValue);
                    }
                }
                scanLine[x] = grayValue;
            }
        }
    }

    if (!options.outputSize.isValid()) {
        return image;
    }

    return image.scaled(options.outputSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

} // namespace

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
