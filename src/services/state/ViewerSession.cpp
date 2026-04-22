#include "services/state/ViewerSession.h"

ViewerSession::ViewerSession(QObject *parent)
    : QObject(parent)
{
}

bool ViewerSession::hasActiveSeries() const
{
    return mCurrentImportResult.has_value();
}

bool ViewerSession::hasVolumeData() const
{
    return mCurrentImportResult.has_value()
        && mCurrentImportResult->volumeBuildResult.success
        && mCurrentImportResult->volumeBuildResult.volumeData.isValid();
}

const ImportResult *ViewerSession::currentImportResult() const
{
    return mCurrentImportResult.has_value() ? &(*mCurrentImportResult) : nullptr;
}

const DicomSeries *ViewerSession::currentSeries() const
{
    if (!mCurrentImportResult.has_value()) {
        return nullptr;
    }

    return &mCurrentImportResult->selectedSeries;
}

const VolumeBuildResult *ViewerSession::currentVolumeBuildResult() const
{
    if (!mCurrentImportResult.has_value()) {
        return nullptr;
    }

    return &mCurrentImportResult->volumeBuildResult;
}

const VolumeData *ViewerSession::currentVolumeData() const
{
    const VolumeBuildResult *buildResult = currentVolumeBuildResult();
    if (buildResult == nullptr || !buildResult->success || !buildResult->volumeData.isValid()) {
        return nullptr;
    }

    return &buildResult->volumeData;
}

void ViewerSession::setImportResult(const ImportResult &result)
{
    mCurrentImportResult = result;
    emit sessionChanged();          // 标志本次series导入（解析 + 构建volume）成功 -> 触发WorkSpace渲染/缩略图....
}

void ViewerSession::clear()
{
    if (!mCurrentImportResult.has_value()) {
        return;
    }

    mCurrentImportResult.reset();
    emit sessionCleared();
    emit sessionChanged();
}
