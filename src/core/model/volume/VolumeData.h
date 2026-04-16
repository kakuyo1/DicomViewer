#pragma once

#include <QVector>
#include <QString>

struct VolumeData
{
    QString seriesInstanceUid;
    QString seriesDescription;
    QString modality;

    int width           = 0;
    int height          = 0;
    int depth           = 0;

    double spacingX     = 1.0;
    double spacingY     = 1.0;
    double spacingZ     = 1.0;

    double windowCenter = 0.0;
    double windowWidth  = 0.0;

    QVector<qint16> voxels;

    bool isValid() const
    {
        return width > 0 && height > 0 && depth > 0 && voxels.size() == width * height * depth;
    }
};
