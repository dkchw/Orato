#include "FlowLayout.h"
#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpace(hSpacing)
    , m_vSpace(vSpacing) {
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout() {
    QLayoutItem *item;
    while ((item = takeAt(0))) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem *item) {
    m_itemList.append(item);
}

int FlowLayout::horizontalSpacing() const {
    if (m_hSpace >= 0) {
        return m_hSpace;
    }
    return 6;
}

int FlowLayout::verticalSpacing() const {
    if (m_vSpace >= 0) {
        return m_vSpace;
    }
    return 6;
}

int FlowLayout::count() const {
    return m_itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const {
    return m_itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index) {
    if (index >= 0 && index < m_itemList.size()) {
        return m_itemList.takeAt(index);
    }
    return nullptr;
}

Qt::Orientations FlowLayout::expandingDirections() const {
    return {};
}

bool FlowLayout::hasHeightForWidth() const {
    return true;
}

int FlowLayout::heightForWidth(int width) const {
    return doLayout(QRect(0, 0, width, 0), true);
}

void FlowLayout::setGeometry(const QRect &rect) {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const {
    return minimumSize();
}

QSize FlowLayout::minimumSize() const {
    QSize size;
    for (const QLayoutItem *item : m_itemList) {
        size = size.expandedTo(item->minimumSize());
    }
    const QMargins margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    const int spaceX = horizontalSpacing();
    const int spaceY = verticalSpacing();

    struct LineItem {
        QLayoutItem *item;
        int x;
        int width;
        int height;
    };
    QList<LineItem> currentLine;

    auto flushLine = [&](int finalLineHeight) {
        if (!testOnly) {
            for (const auto &li : currentLine) {
                int itemY = y + (finalLineHeight - li.height) / 2;
                li.item->setGeometry(QRect(li.x, itemY, li.width, li.height));
            }
        }
        currentLine.clear();
    };

    for (QLayoutItem *item : m_itemList) {
        QWidget *wid = item->widget();
        if (wid && !wid->isVisible()) {
            continue;
        }

        QSize itemSize = item->sizeHint();
        int itemW = itemSize.width();
        int itemH = itemSize.height();

        if (x + itemW > effectiveRect.right() && lineHeight > 0) {
            flushLine(lineHeight);
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            lineHeight = 0;
        }

        currentLine.append({item, x, itemW, itemH});
        x += itemW + spaceX;
        lineHeight = qMax(lineHeight, itemH);
    }

    if (!currentLine.isEmpty()) {
        flushLine(lineHeight);
    }

    return y + lineHeight - rect.y() + bottom;
}

