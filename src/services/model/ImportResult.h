#pragma once

#include <QVector>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"

struct SeriesScanResult
{
    QVector<DicomSeries> seriesList;
    int scannedFileCount   = 0;
    int readableDicomCount = 0;
    int acceptedSliceCount = 0;
    int skippedNonCtCount  = 0;
};

struct ImportResult
{
    DicomSeries selectedSeries;
    VolumeData volumeData;
    SeriesScanResult scanResult;
};
