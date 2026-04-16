#pragma once

#include <optional>

#include <QString>

#include "core/model/dicom/DicomSliceInfo.h"

class DicomTagReader
{
public:
    std::optional<DicomSliceInfo> readSliceInfo(const QString &filePath) const;
};
