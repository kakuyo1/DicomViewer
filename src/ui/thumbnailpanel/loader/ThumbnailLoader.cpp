#include "ThumbnailLoader.h"

#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

namespace
{

constexpr int kThumbnailWidth  = 160;
constexpr int kThumbnailHeight = 112;

ThumbnailLoadResult loadThumbnailImage(const SliceImageBuildInput &input,
                                       int row,
                                       int generation,
                                       int displayRevision,
                                       double windowCenter,
                                       double windowWidth,
                                       bool invert,
                                       bool flipHorizontal,
                                       bool flipVertical)
{
    ThumbnailLoadResult result;
    result.row             = row;
    result.generation      = generation;
    result.displayRevision = displayRevision;

    SliceImageBuildOptions buildOptions;
    buildOptions.windowCenter   = windowCenter;
    buildOptions.windowWidth    = windowWidth;
    buildOptions.invert         = invert;
    buildOptions.flipHorizontal = flipHorizontal;
    buildOptions.flipVertical   = flipVertical;
    buildOptions.outputSize     = QSize(kThumbnailWidth, kThumbnailHeight);

    const QImage image = buildSliceImage(input, buildOptions);
    if (image.isNull()) {
        return result;
    }

    result.success = true;
    result.image   = image;
    return result;
}

} // namespace

ThumbnailLoader::ThumbnailLoader(QObject *parent)
    : QObject(parent)
{
}

ThumbnailLoader::~ThumbnailLoader()
{
}

void ThumbnailLoader::requestThumbnail(int row,
                                       const SliceImageBuildInput &input,
                                       int generation,
                                       int displayRevision,
                                       double windowCenter,
                                       double windowWidth,
                                       bool invert,
                                       bool flipHorizontal,
                                       bool flipVertical)
{
    auto *watcher = new QFutureWatcher<ThumbnailLoadResult>(this);
    connect(watcher, &QFutureWatcher<ThumbnailLoadResult>::finished, this, [this, watcher]() {
        const ThumbnailLoadResult result = watcher->result();
        watcher->deleteLater();

        if (!result.success || result.image.isNull()) {
            emit thumbnailFailed(result.row, result.generation, result.displayRevision);
            return;
        }

        emit thumbnailLoaded(result.row, result.generation, result.displayRevision, result.image);
    });

    watcher->setFuture(QtConcurrent::run([input, row, generation, displayRevision, windowCenter, windowWidth, invert, flipHorizontal, flipVertical]() {
        return loadThumbnailImage(input, row, generation, displayRevision, windowCenter, windowWidth, invert, flipHorizontal, flipVertical);
    }));
}
