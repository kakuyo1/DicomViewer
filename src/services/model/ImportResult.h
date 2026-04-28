#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

#include <memory>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"

struct SeriesScanResult
{
    QString directoryPath;
    QVector<DicomSeries> seriesList;
    int scannedFileCount             = 0;
    int readableDicomCount           = 0;
    int acceptedSliceCount           = 0;
    int skippedNonCtCount            = 0;
    int skippedUnreadableFileCount   = 0;
    int skippedMissingSeriesUidCount = 0;
    int seriesCount                  = 0;
    QStringList warnings;
};

struct VolumeBuildResult
{
    bool success                     = false;
    std::shared_ptr<VolumeData> volumeData;
    QString errorMessage;
    QStringList warnings;
    QString buildSummary;
};

struct ImportResult
{
    DicomSeries selectedSeries;
    SeriesScanResult scanResult;
    VolumeBuildResult volumeBuildResult;
};
