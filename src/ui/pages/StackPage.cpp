#include "StackPage.h"

#include <QVBoxLayout>

#include "core/model/volume/VolumeData.h"
#include "services/state/ViewerSession.h"
#include "ui/widgets/SliceViewWidget.h"

StackPage::StackPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

StackPage::~StackPage()
{

}

void StackPage::setupUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    mSliceViewWidget = new SliceViewWidget(this);
    mSliceViewWidget->setMinimumSize(640, 480);
    rootLayout->addWidget(mSliceViewWidget);
}

void StackPage::setViewerSession(ViewerSession *viewerSession)
{
    if (mViewerSession == viewerSession) {
        return;
    }

    if (mViewerSession != nullptr) {
        disconnect(mViewerSession, nullptr, this, nullptr);
    }

    mViewerSession = viewerSession;
    if (mViewerSession != nullptr) {
        connect(mViewerSession, &ViewerSession::sessionChanged, this, &StackPage::refreshFromSession);
        connect(mViewerSession, &ViewerSession::sessionCleared, this, &StackPage::clearDisplay);
    }

    refreshFromSession();
}

void StackPage::refreshFromSession()
{
    if (mSliceViewWidget == nullptr || mViewerSession == nullptr) {
        return;
    }

    const VolumeData *volumeData = mViewerSession->currentVolumeData();
    if (volumeData == nullptr || !volumeData->isValid()) {
        clearDisplay();
        return;
    }

    /** @note 默认选择中间的切片展示，比较符合用户习惯 */
    const int defaultSliceIndex = volumeData->depth / 2;
    mSliceViewWidget->showAxialSlice(*volumeData, defaultSliceIndex);
}

void StackPage::clearDisplay()
{
    if (mSliceViewWidget != nullptr) {
        mSliceViewWidget->clearDisplay();
    }
}
