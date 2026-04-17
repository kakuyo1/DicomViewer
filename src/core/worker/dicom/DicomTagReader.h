#pragma once

#include <QString>

#include "core/model/dicom/DicomSliceInfo.h"

class DicomTagReader
{
public:
    DicomReadResult readSliceInfo(const QString &filePath) const;
};
