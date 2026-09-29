#include "SessionItemDelegate.h"
#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QFontMetrics>
#include <algorithm>

SessionItemDelegate::SessionItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent) {
}

QSize SessionItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    int viewWidth = option.rect.width();
    if (viewWidth <= 0 && option.widget) {
        auto *list = qobject_cast<const QAbstractScrollArea*>(option.widget);
        if (list && list->viewport()) {
            viewWidth = list->viewport()->width();
        } else {
            viewWidth = option.widget->width();
        }
    }
    if (viewWidth <= 0) {
        viewWidth = 240;
    }

    // Usable width inside card:
    // Outer card margins (4px left, 4px right) + inner card padding (14px left, 10px right)
    int availTextWidth = std::max(60, viewWidth - 32);

    QString title = index.data(Qt::DisplayRole).toString();
    if (title.trimmed().isEmpty()) {
        title = QStringLiteral("Untitled Session");
    }

    QFont titleFont = option.font;
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    QFontMetrics fmTitle(titleFont);

    // Compute bounding rect with full word wrapping
    QRect titleRect = fmTitle.boundingRect(0, 0, availTextWidth, 10000, Qt::TextWordWrap, title);
    int titleHeight = std::max(fmTitle.height(), titleRect.height());

    QFont metaFont = option.font;
    metaFont.setPointSize(std::max(8, metaFont.pointSize() - 1));
    QFontMetrics fmMeta(metaFont);
    int metaHeight = fmMeta.height();

    // 8px top padding + titleHeight + 4px spacing + metaHeight + 8px bottom padding + 6px card margin
    int totalHeight = 8 + titleHeight + 4 + metaHeight + 8 + 6;
    return QSize(viewWidth, std::max(totalHeight, 54));
}

void SessionItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    bool isSelected = option.state & QStyle::State_Selected;
    bool isHovered = option.state & QStyle::State_MouseOver;

    // Outer card bounds
    QRect cardRect = option.rect.adjusted(4, 3, -4, -3);
    painter->setClipRect(cardRect);

    // Card background
    QColor bgColor = isSelected ? QColor("#222738") : (isHovered ? QColor("#1c1f2b") : QColor("#151821"));
    QColor borderColor = isSelected ? QColor("#6366f1") : (isHovered ? QColor("#33384a") : QColor("#232733"));

    painter->setPen(QPen(borderColor, isSelected ? 1.5 : 1.0));
    painter->setBrush(bgColor);
    painter->drawRoundedRect(cardRect, 6, 6);

    // Left selection accent indicator
    if (isSelected) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor("#818cf8"));
        painter->drawRoundedRect(QRect(cardRect.left() + 2, cardRect.top() + 6, 3, cardRect.height() - 12), 1.5, 1.5);
    }

    int leftPad = isSelected ? 14 : 10;
    QRect contentRect = cardRect.adjusted(leftPad, 8, -10, -8);

    QString title = index.data(Qt::DisplayRole).toString();
    if (title.trimmed().isEmpty()) {
        title = QStringLiteral("Untitled Session");
    }
    QString dateStr = index.data(Qt::UserRole + 1).toString();
    QString durStr = index.data(Qt::UserRole + 2).toString();
    int takesCount = index.data(Qt::UserRole + 4).toInt();

    // 1. Meta Row at bottom of card
    QFont metaFont = option.font;
    metaFont.setPointSize(std::max(8, metaFont.pointSize() - 1));
    QFontMetrics fmMeta(metaFont);
    int metaHeight = fmMeta.height();
    QRect metaBox(contentRect.left(), contentRect.bottom() - metaHeight, contentRect.width(), metaHeight);

    // 2. Title Area filling space above meta row
    QFont titleFont = option.font;
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    painter->setFont(titleFont);
    painter->setPen(isSelected ? QColor("#ffffff") : QColor("#f1f5f9"));

    QRect titleArea(contentRect.left(), contentRect.top(), contentRect.width(), std::max(fmMeta.height(), metaBox.top() - contentRect.top() - 3));
    painter->drawText(titleArea, Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, title);

    // Draw Meta Text
    painter->setFont(metaFont);
    painter->setPen(isSelected ? QColor("#c7d2fe") : QColor("#94a3b8"));

    QString metaText = durStr.isEmpty() ? dateStr : QString("%1  •  %2").arg(dateStr, durStr);
    if (takesCount > 1) {
        metaText += QString("  •  %1 takes").arg(takesCount);
    }
    painter->drawText(metaBox, Qt::AlignLeft | Qt::AlignVCenter, metaText);

    painter->restore();
}
