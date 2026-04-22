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
    int instanceNumber      = 0;
    int sliceIndex          = -1;
    ThumbnailState state    = ThumbnailState::NotRequested;
    QPixmap thumbnailPixmap;
};
