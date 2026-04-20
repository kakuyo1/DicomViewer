#pragma once

#include <QtGlobal>
#include <QString>
#include <QVector>

struct VolumeData
{
    QString seriesInstanceUid;
    QString seriesDescription;
    QString modality;

    int width                         = 0;
    int height                        = 0;
    int depth                         = 0;

    double spacingX                   = 1.0;
    double spacingY                   = 1.0;
    double spacingZ                   = 1.0;

    double rescaleSlope               = 1.0;
    double rescaleIntercept           = 0.0;

    double windowCenter               = 0.0;
    double windowWidth                = 0.0;

    bool hasPixelPaddingValue         = false;
    qint16 pixelPaddingValue          = 0;          // 无效像素的基准值
    bool hasPixelPaddingRangeLimit    = false;
    qint16 pixelPaddingRangeLimit     = 0;          // 无效像素范围的另一端

    bool usedSliceThicknessAsSpacingZ = true;
    QString spacingZSource;
    QString geometrySummary;

    QVector<qint16> voxels;

    bool isValid() const
    {
        return width > 0 && height > 0 && depth > 0 && voxels.size() == width * height * depth;
    }
};
