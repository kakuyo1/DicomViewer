#pragma once

#include <optional>

#include <QString>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"

class VolumeBuilder
{
public:
    std::optional<VolumeData> build(const DicomSeries &series, QString *errorMessage = nullptr) const;
};
