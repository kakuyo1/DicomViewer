#pragma once

#include <QWidget>

#include "ui/model/OverlayInfo.h"

class QLabel;
class QResizeEvent;

class ImageOverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageOverlayWidget(QWidget *parent = nullptr);
    ~ImageOverlayWidget();

    void setOverlayInfo(const OverlayInfo &overlayInfo);
    void clearOverlay();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *createCornerLabel();
    void updateLabel(QLabel *label, const QStringList &lines);
    void updateLabelGeometries();

private:
    OverlayInfo mOverlayInfo;
    QLabel *mTopLeftLabel      = nullptr;
    QLabel *mTopRightLabel     = nullptr;
    QLabel *mBottomLeftLabel   = nullptr;
    QLabel *mBottomRightLabel  = nullptr;
};
