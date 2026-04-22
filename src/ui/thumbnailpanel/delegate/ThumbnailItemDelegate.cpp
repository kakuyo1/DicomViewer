#include "ThumbnailItemDelegate.h"

#include <QPainter>

#include "ui/thumbnailpanel/model/ThumbnailListModel.h"

// ┌────────────────────────────────────┐  ─┐
// │  ┌──────────────────────────────┐  │   │ kOuterPadding (10)
// │  │                              │  │   │
// │  │        图片区域               │  │   │ kImageHeight (112)
// │  │       (112 x 168)            │  │   │
// │  │                              │  │   │
// │  └──────────────────────────────┘  │   │
// │            ↓ 10px 间隔              │   │
// │  ┌──────────────────────────────┐  │   │
// │  │         文本标签              │  │   │ kTextHeight (18)
// │  └──────────────────────────────┘  │   │
// │                                    │   │
// └────────────────────────────────────┘  ─┘
//  ←────────── kItemWidth (188) ──────────→

namespace
{

constexpr int kItemWidth    = 188;
constexpr int kItemHeight   = 164;
constexpr int kOuterPadding = 10;
constexpr int kImageHeight  = 112;
constexpr int kTextHeight   = 18;

}

ThumbnailItemDelegate::ThumbnailItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

ThumbnailItemDelegate::~ThumbnailItemDelegate()
{
}

void ThumbnailItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (painter == nullptr || !index.isValid()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRect itemRect  = option.rect.adjusted(1, 1, -1, -1);     // 让矩形稍微内缩，为选中高亮留出空间
    const bool isSelected = (option.state & QStyle::State_Selected);

    // 绘制背景卡片
    painter->setPen(Qt::NoPen);
    painter->setBrush(isSelected ? QColor(56, 76, 112) : QColor(32, 35, 42));
    painter->drawRoundedRect(itemRect, 8.0, 8.0);

    // 绘制图片
    const QRect imageRect(
        itemRect.left()  + kOuterPadding,
        itemRect.top()   + kOuterPadding,
        itemRect.width() - 2 * kOuterPadding,
        kImageHeight);
    painter->setBrush(QColor(18, 18, 18));
    painter->drawRoundedRect(imageRect, 6.0, 6.0);

    const QPixmap pixmap = index.data(ThumbnailListModel::ThumbnailPixmapRole).value<QPixmap>();
    if (!pixmap.isNull()) {
        const QPixmap scaledPixmap = pixmap.scaled(imageRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const QPoint imageTopLeft(
            imageRect.center().x() - scaledPixmap.width() / 2,
            imageRect.center().y() - scaledPixmap.height() / 2);
        painter->drawPixmap(imageTopLeft, scaledPixmap);
    } else {
        painter->setPen(QColor(125, 132, 145));
        painter->drawText(imageRect, Qt::AlignCenter, QStringLiteral("Thumbnail"));
    }

    const ThumbnailState state = static_cast<ThumbnailState>(index.data(ThumbnailListModel::ThumbnailStateRole).toInt());
    if (state == ThumbnailState::Loading || state == ThumbnailState::Failed) {
        painter->setPen(state == ThumbnailState::Loading ? QColor(210, 214, 224) : QColor(224, 120, 120));
        painter->drawText(
            imageRect.adjusted(0, 0, 0, -6),
            Qt::AlignHCenter | Qt::AlignBottom,
            state == ThumbnailState::Loading ? QStringLiteral("Loading...") : QStringLiteral("Failed"));
    }

    // 绘制文本标签
    const QRect textRect(
        itemRect.left()    + kOuterPadding,
        imageRect.bottom() + 10,
        itemRect.width()   - 2 * kOuterPadding,
        kTextHeight);
    painter->setPen(QColor(230, 232, 238));
    painter->drawText(textRect, Qt::AlignCenter, index.data(Qt::DisplayRole).toString());

    painter->restore();
}

QSize ThumbnailItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return {kItemWidth, kItemHeight};
}
