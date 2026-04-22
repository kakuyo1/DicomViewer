#pragma once

#include <QImage>
#include <QSize>
#include <QVector>

#include "core/model/volume/VolumeData.h"

struct SliceImageBuildInput
{
    int width                      = 0;
    int height                     = 0;
    QVector<qint16> pixels;
    bool hasPixelPaddingValue      = false;
    qint16 pixelPaddingValue       = 0;
    bool hasPixelPaddingRangeLimit = false;
    qint16 pixelPaddingRangeLimit  = 0;

    bool isValid() const
    {
        return width > 0 && height > 0 && pixels.size() == width * height;
    }
};

struct SliceImageBuildOptions
{
    double windowCenter      = 0.0;
    double windowWidth       = 0.0;
    bool invert              = false;
    bool flipHorizontal      = false;
    bool flipVertical        = false;
    QSize outputSize;
};

bool isPaddingPixel(qint16 value,
                    bool hasPixelPaddingValue,
                    qint16 pixelPaddingValue,
                    bool hasPixelPaddingRangeLimit,
                    qint16 pixelPaddingRangeLimit);

unsigned char mapWindowLevel(qint16 value,
                             double windowCenter,
                             double windowWidth,
                             qint16 sliceMin,
                             qint16 sliceMax);

SliceImageBuildInput buildSliceImageInput(const VolumeData &volumeData,
                                          int sliceIndex);

QImage buildSliceImage(const VolumeData &volumeData,
                       int sliceIndex,
                       const SliceImageBuildOptions &options);

QImage buildSliceImage(const SliceImageBuildInput &input,
                       const SliceImageBuildOptions &options);
