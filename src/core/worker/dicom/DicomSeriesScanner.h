#pragma once

#include <QString>

#include "services/model/ImportResult.h"

class DicomSeriesScanner
{
public:
    SeriesScanResult scanDirectory(const QString &directoryPath) const;
};
