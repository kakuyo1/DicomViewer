#pragma once

#include <QPixmap>
#include <QString>

enum class ThumbnailState
{
    NotRequested,
    Loading,
    Ready,
    Failed,
};

struct ThumbnailItemData
{
    QString filePath;
    int instanceNumber         = 0;
    int sliceIndex             = -1;
    ThumbnailState state       = ThumbnailState::NotRequested;
    int appliedDisplayRevision = -1;                            // 当前 `thumbnailPixmap` 是按哪个显示参数版本生成的
    int loadingDisplayRevision = -1;                            // 当前正在后台生成的任务，是为哪个显示参数版本服务的
    QPixmap thumbnailPixmap;
};
