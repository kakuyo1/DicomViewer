#include "SliceToolBar.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QToolButton>

#include "common/Util.h"

SliceToolBar::SliceToolBar(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

SliceToolBar::~SliceToolBar()
{
}

void SliceToolBar::setupUi()
{
    setObjectName(QStringLiteral("sliceToolBar"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setMinimumHeight(52);

    /** @note [Pan] [Zoom] [WL] [Measure] [Crosshair] | [Flip H] [Flip V] [Invert] | [Reset] */
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    mVRPresetCombo = new QComboBox(this);
    mVRPresetCombo->setMinimumHeight(32);
    mVRPresetCombo->setCursor(Qt::PointingHandCursor);
    mVRPresetCombo->addItem(QStringLiteral("Bone"), static_cast<int>(VRPreset::Bone));
    mVRPresetCombo->addItem(QStringLiteral("Soft Tissue"), static_cast<int>(VRPreset::SoftTissue));
    mVRPresetCombo->addItem(QStringLiteral("Skin"), static_cast<int>(VRPreset::Skin));
    layout->addWidget(mVRPresetCombo);

    // 1.[Pan] [Zoom] [WL] [Measure] [Crosshair]
    mModeGroup = new QButtonGroup(this);
    mModeGroup->setExclusive(true);

    mPanButton         = createToolButton(QStringLiteral(" Pan"), QStringLiteral("resources/icons/Pan.png"), true);
    mZoomButton        = createToolButton(QStringLiteral(" Zoom"), QStringLiteral("resources/icons/Zoom.png"), true);
    mWindowLevelButton = createToolButton(QStringLiteral(" WW/WL"), QStringLiteral("resources/icons/WW-WL.png"), true);
    mMeasureButton     = createToolButton(QStringLiteral(" Measure"), QStringLiteral("resources/icons/Measure.png"), true);
    mCrosshairButton   = createToolButton(QStringLiteral(" Crosshair"), QStringLiteral("resources/icons/Crosshair.png"), true);

    mModeGroup->addButton(mPanButton);
    mModeGroup->addButton(mZoomButton);
    mModeGroup->addButton(mWindowLevelButton);
    mModeGroup->addButton(mMeasureButton);
    mModeGroup->addButton(mCrosshairButton);

    layout->addWidget(mPanButton);
    layout->addWidget(mZoomButton);
    layout->addWidget(mWindowLevelButton);
    layout->addWidget(mMeasureButton);
    layout->addWidget(mCrosshairButton);
    layout->addSpacing(4);
    mModeSeparator = createSeparator();
    layout->addWidget(mModeSeparator);
    layout->addSpacing(4);

    // 2.[Flip H] [Flip V] [Invert]
    mFlipHButton  = createToolButton(QStringLiteral(" Flip H"), QStringLiteral("resources/icons/Flip-H.png"), false);
    mFlipVButton  = createToolButton(QStringLiteral(" Flip V"), QStringLiteral("resources/icons/Flip-V.png"), false);
    mInvertButton = createToolButton(QStringLiteral(" Invert"), QStringLiteral("resources/icons/Invert.png"), false);

    layout->addWidget(mFlipHButton);
    layout->addWidget(mFlipVButton);
    layout->addWidget(mInvertButton);
    layout->addSpacing(4);
    mActionSeparator = createSeparator();
    layout->addWidget(mActionSeparator);
    layout->addSpacing(4);

    // 3.[Reset]
    mResetButton = createToolButton(QStringLiteral(" Reset"), QStringLiteral("resources/icons/Reset.png"), false);
    layout->addWidget(mResetButton);

    connect(mModeGroup, &QButtonGroup::buttonClicked, this, &SliceToolBar::handleModeButtonClicked);
    connect(mFlipHButton, &QToolButton::clicked, this, &SliceToolBar::flipHorizontalTriggered);
    connect(mFlipVButton, &QToolButton::clicked, this, &SliceToolBar::flipVerticalTriggered);
    connect(mInvertButton, &QToolButton::clicked, this, &SliceToolBar::invertTriggered);
    connect(mResetButton, &QToolButton::clicked, this, &SliceToolBar::resetTriggered);
    connect(mVRPresetCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (mVRPresetCombo == nullptr || index < 0) {
            return;
        }

        emit vrPresetChanged(static_cast<VRPreset>(mVRPresetCombo->itemData(index).toInt()));
    });

    mPanButton->setChecked(true);
    setViewMode(ViewMode::Stack);
}

QToolButton *SliceToolBar::createToolButton(const QString &text, const QString &iconPath, bool checkable)
{
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setCheckable(checkable);
    button->setMinimumHeight(32);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setIconSize(QSize(24, 24));
    button->setCursor(Qt::PointingHandCursor);

    const QString resolvedIconPath = util::resolveProjectRelativePath(iconPath);
    if (!resolvedIconPath.isEmpty()) {
        button->setIcon(QIcon(resolvedIconPath));
    }

    return button;
}

QFrame *SliceToolBar::createSeparator()
{
    auto *separator = new QFrame(this);
    separator->setObjectName(QStringLiteral("sliceToolBarSeparator"));
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Plain);
    separator->setFixedWidth(2);
    separator->setMinimumHeight(24);
    return separator;
}

void SliceToolBar::setActiveToolMode(SliceToolMode mode)
{
    if (mode == SliceToolMode::Pan && mPanButton != nullptr) {
        mPanButton->setChecked(true);
        return;
    }
    if (mode == SliceToolMode::Zoom && mZoomButton != nullptr) {
        mZoomButton->setChecked(true);
        return;
    }
    if (mode == SliceToolMode::WindowLevel && mWindowLevelButton != nullptr) {
        mWindowLevelButton->setChecked(true);
        return;
    }
    if (mode == SliceToolMode::Measure && mMeasureButton != nullptr) {
        mMeasureButton->setChecked(true);
        return;
    }
    if (mode == SliceToolMode::Crosshair && mCrosshairButton != nullptr && mCrosshairButton->isVisible()) {
        mCrosshairButton->setChecked(true);
    }
}

void SliceToolBar::setViewMode(ViewMode mode)
{
    const bool stackMode = (mode == ViewMode::Stack);
    const bool mprMode   = (mode == ViewMode::MPR);
    const bool vrMode    = (mode == ViewMode::VR);

    if (mVRPresetCombo != nullptr) {
        mVRPresetCombo->setVisible(vrMode);
    }
    if (mPanButton != nullptr) {
        mPanButton->setVisible(!vrMode);
    }
    if (mZoomButton != nullptr) {
        mZoomButton->setVisible(!vrMode);
    }
    if (mWindowLevelButton != nullptr) {
        mWindowLevelButton->setVisible(!vrMode);
    }
    if (mMeasureButton != nullptr) {
        mMeasureButton->setVisible(!vrMode);
    }
    if (mCrosshairButton != nullptr) {
        mCrosshairButton->setVisible(mprMode);
    }
    if (mModeSeparator != nullptr) {
        mModeSeparator->setVisible(!vrMode);
    }
    if (mFlipHButton != nullptr) {
        mFlipHButton->setVisible(stackMode || mprMode);
    }
    if (mFlipVButton != nullptr) {
        mFlipVButton->setVisible(stackMode || mprMode);
    }
    if (mInvertButton != nullptr) {
        mInvertButton->setVisible(stackMode || mprMode);
    }
    if (mActionSeparator != nullptr) {
        mActionSeparator->setVisible(!vrMode);
    }
    if (mResetButton != nullptr) {
        mResetButton->setVisible(true);
    }
}

void SliceToolBar::setCrosshairVisible(bool visible)
{
    if (mCrosshairButton == nullptr) {
        return;
    }

    if (!visible && mCrosshairButton->isChecked()) {
        setActiveToolMode(SliceToolMode::Pan);
    }
    mCrosshairButton->setVisible(visible);
}

void SliceToolBar::handleModeButtonClicked(QAbstractButton *button)
{
    if (button == mPanButton) {
        emit toolModeChanged(SliceToolMode::Pan);
        return;
    }
    if (button == mZoomButton) {
        emit toolModeChanged(SliceToolMode::Zoom);
        return;
    }
    if (button == mWindowLevelButton) {
        emit toolModeChanged(SliceToolMode::WindowLevel);
        return;
    }
    if (button == mMeasureButton) {
        emit toolModeChanged(SliceToolMode::Measure);
        return;
    }
    if (button == mCrosshairButton) {
        emit toolModeChanged(SliceToolMode::Crosshair);
    }
}
