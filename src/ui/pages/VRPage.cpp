#include "VRPage.h"

#include <QLabel>
#include <QVBoxLayout>

#include "core/model/dicom/DicomSeries.h"
#include "core/model/volume/VolumeData.h"
#include "services/state/ViewerSession.h"

VRPage::VRPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

VRPage::~VRPage()
{
}

void VRPage::setupUi()
{
    setObjectName(QStringLiteral("vrPage"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(24, 24, 24, 24);
    rootLayout->setSpacing(10);
    rootLayout->addStretch();

    mTitleLabel = new QLabel(QStringLiteral("VR View"), this);
    mTitleLabel->setObjectName(QStringLiteral("placeholderPageTitle"));
    mTitleLabel->setAlignment(Qt::AlignCenter);

    mHintLabel = new QLabel(QStringLiteral("Volume rendering placeholder"), this);
    mHintLabel->setObjectName(QStringLiteral("placeholderPageHint"));
    mHintLabel->setAlignment(Qt::AlignCenter);
    mHintLabel->setWordWrap(true);

    rootLayout->addWidget(mTitleLabel);
    rootLayout->addWidget(mHintLabel);
    rootLayout->addStretch();
}

void VRPage::setViewerSession(ViewerSession *viewerSession)
{
    if (mViewerSession == viewerSession) { // 重复
        return;
    }

    if (mViewerSession != nullptr) { // 已有
        disconnect(mViewerSession, nullptr, this, nullptr);
    }

    mViewerSession = viewerSession;
    if (mViewerSession != nullptr) {
        connect(mViewerSession, &ViewerSession::sessionChanged, this, &VRPage::refreshFromSession);
        connect(mViewerSession, &ViewerSession::sessionCleared, this, &VRPage::clearDisplay);
    }

    refreshFromSession();
}

void VRPage::resetView()
{
    refreshFromSession();
}

void VRPage::refreshFromSession()
{
    if (mHintLabel == nullptr) {
        return;
    }

    const DicomSeries *currentSeries = (mViewerSession != nullptr) ? mViewerSession->currentSeries() : nullptr;
    const VolumeData  *volumeData    = (mViewerSession != nullptr) ? mViewerSession->currentVolumeData() : nullptr;
    if (currentSeries == nullptr || volumeData == nullptr || !volumeData->isValid()) {
        clearDisplay();
        return;
    }

    const QString description = currentSeries->seriesDescription.isEmpty()
                                    ? QStringLiteral("Current series loaded")
                                    : currentSeries->seriesDescription;
    mHintLabel->setText(QStringLiteral("%1\nVolume: %2 x %3 x %4")
                            .arg(description)
                            .arg(volumeData->width)
                            .arg(volumeData->height)
                            .arg(volumeData->depth));
}

void VRPage::clearDisplay()
{
    if (mHintLabel != nullptr) {
        mHintLabel->setText(QStringLiteral("Import a CT series, then implement volume rendering here"));
    }
}
