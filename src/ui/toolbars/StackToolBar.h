#pragma once

#include <QWidget>

#include "ui/toolbars/StackToolMode.h"

class QAbstractButton;
class QButtonGroup;
class QToolButton;

class StackToolBar : public QWidget
{
    Q_OBJECT

public:
    explicit StackToolBar(QWidget *parent = nullptr);
    ~StackToolBar();

    void setActiveToolMode(StackToolMode mode);

signals:
    void toolModeChanged(StackToolMode mode);
    void invertTriggered();
    void flipHorizontalTriggered();
    void flipVerticalTriggered();
    void resetTriggered();

private:
    void setupUi();
    QToolButton *createToolButton(const QString &text, bool checkable);
    void handleModeButtonClicked(QAbstractButton *button);

private:
    QButtonGroup *mModeGroup         = nullptr;
    QToolButton  *mPanButton         = nullptr;
    QToolButton  *mZoomButton        = nullptr;
    QToolButton  *mWindowLevelButton = nullptr;
    QToolButton  *mMeasureButton     = nullptr;
    QToolButton  *mFlipHButton       = nullptr;
    QToolButton  *mFlipVButton       = nullptr;
    QToolButton  *mInvertButton      = nullptr;
    QToolButton  *mResetButton       = nullptr;
};
