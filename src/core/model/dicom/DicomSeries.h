#pragma once

#include <QVector>
#include <QString>

#include "core/model/dicom/DicomSliceInfo.h"

struct DicomSeries
{
    QString seriesInstanceUid;
    QString seriesDescription;
    QString modality;
    QString pathSummary;
    QVector<DicomSliceInfo> slices;

    QString displayName() const
    {
        const QString description = seriesDescription.isEmpty() ? QStringLiteral("(No Series Description)") : seriesDescription;
        return QStringLiteral("%1 | %2 | %3 slices").arg(description, modality).arg(slices.size());
    }
};
