#pragma once

#include <QObject>
#include <QString>
#include <QFutureWatcher>

#include "services/model/ImportResult.h"

class QWidget;

class ImportController : public QObject
{
    Q_OBJECT

public:
    explicit ImportController(QObject *parent = nullptr);

    void startImportFromFolder(QWidget *dialogParent);
    bool isBusy() const;

signals:
    void importStarted();
    void importCancelled();
    void importFailed(const QString &message);
    void importSucceeded(const ImportResult &result);

private:
    void handleScanFinished();
    void handleVolumeBuildFinished();
    void startVolumeBuild(const DicomSeries &selectedSeries);
    void resetState();

private:
    QWidget *mDialogParent = nullptr;
    QString mCurrentDirectoryPath;
    bool mBusy = false;

    SeriesScanResult mPendingScanResult;
    DicomSeries mPendingSelectedSeries;

    /** @note 用QFutureWatcher可以很方便地接入Qt的信号槽机制 -> 自动获取finished信号 */
    QFutureWatcher<SeriesScanResult>  mScanWatcher;
    QFutureWatcher<VolumeBuildResult> mVolumeWatcher;
};
