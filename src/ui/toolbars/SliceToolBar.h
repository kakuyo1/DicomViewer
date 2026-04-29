#pragma once

#include <QWidget>

#include "ui/model/VRPreset.h"
#include "ui/model/ViewMode.h"
#include "ui/toolbars/SliceToolMode.h"

class QAbstractButton;
class QButtonGroup;
class QComboBox;
class QFrame;
class QToolButton;

class SliceToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit SliceToolBar(QWidget *parent = nullptr);
    ~SliceToolBar();

    void setActiveToolMode(SliceToolMode mode);
    void setViewMode(ViewMode mode);
    void setCrosshairVisible(bool visible);

signals:
    void toolModeChanged(SliceToolMode mode);
    void invertTriggered();
    void flipHorizontalTriggered();
    void flipVerticalTriggered();
    void resetTriggered();
    void vrPresetChanged(VRPreset preset);

private:
    void setupUi();
    QToolButton *createToolButton(const QString &text, const QString &iconPath, bool checkable);
    QFrame *createSeparator();
    void handleModeButtonClicked(QAbstractButton *button);

private:
    QComboBox    *mVRPresetCombo     = nullptr;
    QButtonGroup *mModeGroup         = nullptr;
    QToolButton  *mPanButton         = nullptr;
    QToolButton  *mZoomButton        = nullptr;
    QToolButton  *mWindowLevelButton = nullptr;
    QToolButton  *mMeasureButton     = nullptr;
    QToolButton  *mCrosshairButton   = nullptr;
    QFrame       *mModeSeparator      = nullptr;
    QToolButton  *mFlipHButton       = nullptr;
    QToolButton  *mFlipVButton       = nullptr;
    QToolButton  *mInvertButton      = nullptr;
    QFrame       *mActionSeparator    = nullptr;
    QToolButton  *mResetButton       = nullptr;
};
