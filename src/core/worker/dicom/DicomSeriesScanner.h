#pragma once

#include <QVector>
#include <QString>

#include "core/model/dicom/DicomSeries.h"

class DicomSeriesScanner
{
public:
    QVector<DicomSeries> scanDirectory(const QString &directoryPath) const;
};
