#pragma once

#include <QString>

struct DicomVector3
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct DicomSliceInfo
{
    QString filePath;
    QString seriesInstanceUid;
    QString sopInstanceUid;
    QString seriesDescription;
    QString modality;

    int instanceNumber     = 0;
    bool hasInstanceNumber = false;

    DicomVector3 imagePositionPatient;
    DicomVector3 rowDirection;
    DicomVector3 columnDirection;
    DicomVector3 sliceNormal;                   /** @note sliceNormal = rowDirection [cross] columnDirection */
    bool hasImagePositionPatient    = false;
    bool hasImageOrientationPatient = false;
    bool hasSliceNormal             = false;

    double sliceThickness  = 0.0;
    bool hasSliceThickness = false;

    double windowCenter    = 0.0;
    double windowWidth     = 0.0;
    bool hasWindowCenter   = false;
    bool hasWindowWidth    = false;
};

struct DicomReadResult
{
    bool success                  = false;
    bool readableDicom            = false;
    bool missingSeriesInstanceUid = false;
    DicomSliceInfo sliceInfo;
    QString errorMessage;
};
