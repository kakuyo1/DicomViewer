#include "ImageOverlayWidget.h"

#include <algorithm>

#include <QLabel>
#include <QResizeEvent>

namespace
{

constexpr int kOverlayMargin   = 14;
constexpr int kOverlayMaxWidth = 280;

}

ImageOverlayWidget::ImageOverlayWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_NoSystemBackground, true);

    mTopLeftLabel     = createCornerLabel();
    mTopRightLabel    = createCornerLabel();
    mBottomLeftLabel  = createCornerLabel();
    mBottomRightLabel = createCornerLabel();

    mOrientationTopLabel    = createOrientationLabel();
    mOrientationBottomLabel = createOrientationLabel();
    mOrientationLeftLabel   = createOrientationLabel();
    mOrientationRightLabel  = createOrientationLabel();
}

ImageOverlayWidget::~ImageOverlayWidget()
{
}

void ImageOverlayWidget::setOverlayInfo(const OverlayInfo &overlayInfo)
{
    mOverlayInfo = overlayInfo;

    updateLabel(mTopLeftLabel, mOverlayInfo.topLeftLines);
    updateLabel(mTopRightLabel, mOverlayInfo.topRightLines);
    updateLabel(mBottomLeftLabel, mOverlayInfo.bottomLeftLines);
    updateLabel(mBottomRightLabel, mOverlayInfo.bottomRightLines);
    updateLabelGeometries();
}

void ImageOverlayWidget::clearOverlay()
{
    setOverlayInfo({});
    clearOrientationMarkers();
}

void ImageOverlayWidget::setOrientationMarkers(const QString &left, const QString &right, const QString &top, const QString &bottom)
{
    updateTextLabel(mOrientationLeftLabel, left);
    updateTextLabel(mOrientationRightLabel, right);
    updateTextLabel(mOrientationTopLabel, top);
    updateTextLabel(mOrientationBottomLabel, bottom);
    updateLabelGeometries();
}

void ImageOverlayWidget::clearOrientationMarkers()
{
    setOrientationMarkers(QString(), QString(), QString(), QString());
}

void ImageOverlayWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateLabelGeometries();
}

QLabel *ImageOverlayWidget::createCornerLabel()
{
    auto *label = new QLabel(this);
    label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    label->setStyleSheet(QStringLiteral(
        "color: white;"
        "background: transparent;"
        "font-size: 13px;"
        "font-family: Sans Serif;"
        "font-weight: 500;"));
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(false);
    label->hide();
    return label;
}

QLabel *ImageOverlayWidget::createOrientationLabel()
{
    auto *label = new QLabel(this);
    label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    label->setStyleSheet(QStringLiteral(
        "color: rgb(255, 215, 0);"
        "background: transparent;"
        "font-size: 14px;"
        "font-family: Sans Serif;"
        "font-weight: 600;"));
    label->setTextFormat(Qt::PlainText);
    label->setAlignment(Qt::AlignCenter);
    label->hide();
    return label;
}

void ImageOverlayWidget::updateLabel(QLabel *label, const QStringList &lines)
{
    if (label == nullptr) {
        return;
    }

    const QString text = lines.join('\n').trimmed(); // 以换行符连接所有的string
    label->setMaximumWidth(kOverlayMaxWidth);
    label->setText(text);
    label->setVisible(!text.isEmpty());
    if (!text.isEmpty()) {
        label->adjustSize();
    }
}

void ImageOverlayWidget::updateTextLabel(QLabel *label, const QString &text)
{
    if (label == nullptr) {
        return;
    }

    const QString trimmedText = text.trimmed();
    label->setText(trimmedText);
    label->setVisible(!trimmedText.isEmpty());
    if (!trimmedText.isEmpty()) {
        label->adjustSize();
    }
}

void ImageOverlayWidget::updateLabelGeometries()
{
    if (mTopLeftLabel != nullptr && mTopLeftLabel->isVisible()) {
        mTopLeftLabel->move(kOverlayMargin, kOverlayMargin);
    }

    if (mTopRightLabel != nullptr && mTopRightLabel->isVisible()) {
        const int x = width() - mTopRightLabel->width() - kOverlayMargin;
        mTopRightLabel->move(std::max(kOverlayMargin, x), kOverlayMargin);
    }

    if (mBottomLeftLabel != nullptr && mBottomLeftLabel->isVisible()) {
        const int y = height() - mBottomLeftLabel->height() - kOverlayMargin;
        mBottomLeftLabel->move(kOverlayMargin, std::max(kOverlayMargin, y));
    }

    if (mBottomRightLabel != nullptr && mBottomRightLabel->isVisible()) {
        const int x = width() - mBottomRightLabel->width() - kOverlayMargin;
        const int y = height() - mBottomRightLabel->height() - kOverlayMargin;
        mBottomRightLabel->move(std::max(kOverlayMargin, x), std::max(kOverlayMargin, y));
    }

    if (mOrientationTopLabel != nullptr && mOrientationTopLabel->isVisible()) {
        const int x = (width() - mOrientationTopLabel->width()) / 2;
        mOrientationTopLabel->move(std::max(kOverlayMargin, x), kOverlayMargin);
    }

    if (mOrientationBottomLabel != nullptr && mOrientationBottomLabel->isVisible()) {
        const int x = (width() - mOrientationBottomLabel->width()) / 2;
        const int y = height() - mOrientationBottomLabel->height() - kOverlayMargin;
        mOrientationBottomLabel->move(std::max(kOverlayMargin, x), std::max(kOverlayMargin, y));
    }

    if (mOrientationLeftLabel != nullptr && mOrientationLeftLabel->isVisible()) {
        const int y = (height() - mOrientationLeftLabel->height()) / 2;
        mOrientationLeftLabel->move(kOverlayMargin, std::max(kOverlayMargin, y));
    }

    if (mOrientationRightLabel != nullptr && mOrientationRightLabel->isVisible()) {
        const int x = width() - mOrientationRightLabel->width() - kOverlayMargin;
        const int y = (height() - mOrientationRightLabel->height()) / 2;
        mOrientationRightLabel->move(std::max(kOverlayMargin, x), std::max(kOverlayMargin, y));
    }
}
