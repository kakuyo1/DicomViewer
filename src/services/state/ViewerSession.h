#pragma once

#include <QObject>

#include <memory>
#include <optional>

#include "services/model/ImportResult.h"

class ViewerSession : public QObject
{
    Q_OBJECT

public:
    explicit ViewerSession(QObject *parent = nullptr);

    bool hasActiveSeries() const;
    bool hasVolumeData() const;

    const ImportResult      *currentImportResult() const;
    const DicomSeries       *currentSeries() const;
    const VolumeBuildResult *currentVolumeBuildResult() const;
    const VolumeData        *currentVolumeData() const;
    std::shared_ptr<const VolumeData> currentVolumeDataShared() const;

    void setImportResult(const ImportResult &result);
    void clear();

signals:
    void sessionChanged();
    void sessionCleared();

private:
    std::optional<ImportResult> mCurrentImportResult;
};
