#pragma once

#include <QWidget>

#include "ui/model/ViewMode.h"

class QButtonGroup;
class QToolButton;

class ViewModeBar : public QWidget
{
    Q_OBJECT

public:
    explicit ViewModeBar(QWidget *parent = nullptr);
    ~ViewModeBar();

signals:
    void viewModeChanged(ViewMode mode);

private:
    void setupUi();
    QToolButton *createModeButton(const QString &text, const QString &iconPath);

private:
    QButtonGroup *mModeGroup    = nullptr;
    QToolButton *mStackButton   = nullptr;
    QToolButton *mMprButton     = nullptr;
    QToolButton *mVrButton      = nullptr;
};
