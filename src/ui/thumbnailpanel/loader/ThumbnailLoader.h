#pragma once

#include <QImage>
#include <QObject>

#include "core/worker/SliceImageBuilder.h"

struct ThumbnailLoadResult
{
    int row        = -1;
    int generation = -1;
    bool success   = false;
    QImage image;
};

class ThumbnailLoader : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailLoader(QObject *parent = nullptr);
    ~ThumbnailLoader();

    void requestThumbnail(int row, const SliceImageBuildInput &input, int generation, double windowCenter, double windowWidth);

signals:
    void thumbnailLoaded(int row, int generation, const QImage &image);
    void thumbnailFailed(int row, int generation);
};
