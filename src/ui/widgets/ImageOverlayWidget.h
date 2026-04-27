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
    void setOrientationMarkers(const QString &left, const QString &right, const QString &top, const QString &bottom);
    void clearOrientationMarkers();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *createCornerLabel();
    QLabel *createOrientationLabel();
    void updateLabel(QLabel *label, const QStringList &lines);
    void updateTextLabel(QLabel *label, const QString &text);
    void updateLabelGeometries();

private:
    OverlayInfo mOverlayInfo;
    QLabel *mTopLeftLabel      = nullptr;
    QLabel *mTopRightLabel     = nullptr;
    QLabel *mBottomLeftLabel   = nullptr;
    QLabel *mBottomRightLabel  = nullptr;

    QLabel *mOrientationTopLabel    = nullptr;
    QLabel *mOrientationBottomLabel = nullptr;
    QLabel *mOrientationLeftLabel   = nullptr;
    QLabel *mOrientationRightLabel  = nullptr;
};
