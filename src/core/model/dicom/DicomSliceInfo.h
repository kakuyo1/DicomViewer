#pragma once

#include <QString>

struct DicomSliceInfo
{
    QString filePath;
    QString seriesInstanceUid;
    QString sopInstanceUid;
    QString seriesDescription;
    QString modality;
    int instanceNumber = 0;
    bool hasInstanceNumber = false;
};
