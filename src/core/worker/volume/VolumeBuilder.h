#pragma once

#include <optional>

#include <QString>
#include <QStringList>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"

class VolumeBuilder
{
public:
    std::optional<VolumeData> build(
        const DicomSeries &series,
        QString *errorMessage = nullptr,
        QStringList *warnings = nullptr,
        QString *buildSummary = nullptr) const;
};
