#include "FlowLayout.h"

#include <QWidget>

FlowLayout::FlowLayout(QWidget *parent, int spacing)
    : QLayout(parent), m_spacing(spacing) {}

FlowLayout::~FlowLayout() {
    QLayoutItem *item;
    while ((item = takeAt(0)) != nullptr) delete item;
}

void FlowLayout::addItem(QLayoutItem *item) { m_items.append(item); }

int FlowLayout::count() const { return m_items.size(); }

QLayoutItem *FlowLayout::itemAt(int index) const { return m_items.value(index); }

QLayoutItem *FlowLayout::takeAt(int index) {
    if (index < 0 || index >= m_items.size()) return nullptr;
    return m_items.takeAt(index);
}

Qt::Orientations FlowLayout::expandingDirections() const { return {}; }

bool FlowLayout::hasHeightForWidth() const { return true; }

int FlowLayout::heightForWidth(int width) const { return doLayout(QRect(0, 0, width, 0), true); }

QSize FlowLayout::minimumSize() const {
    QSize size;
    for (const QLayoutItem *item : m_items) size = size.expandedTo(item->minimumSize());
    const QMargins margins = contentsMargins();
    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

QSize FlowLayout::sizeHint() const { return minimumSize(); }

void FlowLayout::setGeometry(const QRect &rect) {
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const {
    const QRect area = rect.marginsRemoved(contentsMargins());
    int x = area.x();
    int y = area.y();
    int rowHeight = 0;

    for (QLayoutItem *item : m_items) {
        if (item->widget() && item->widget()->isHidden()) continue;
        const QSize hint = item->sizeHint();
        if (x + hint.width() > area.right() + 1 && rowHeight > 0) {
            x = area.x();
            y += rowHeight + m_spacing;
            rowHeight = 0;
        }
        if (!testOnly) item->setGeometry(QRect(QPoint(x, y), hint));
        x += hint.width() + m_spacing;
        rowHeight = qMax(rowHeight, hint.height());
    }
    return y + rowHeight - rect.y() + contentsMargins().bottom();
}
