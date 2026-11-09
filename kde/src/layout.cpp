#include "Omarchy/Layout.h"

#include <QWidget>
#include <cmath>

namespace Omarchy {

void setFill(QWidget *w, bool horizontal, bool vertical) {
  QSizePolicy p = w->sizePolicy();
  p.setHorizontalPolicy(horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred);
  p.setVerticalPolicy(vertical ? QSizePolicy::Expanding : QSizePolicy::Preferred);
  w->setSizePolicy(p);
}

BoxLayout::BoxLayout(Qt::Orientation o, QWidget *parent) : QLayout(parent), m_o(o) {
  setContentsMargins(0, 0, 0, 0);
  setSpacing(0);
}

BoxLayout::~BoxLayout() {
  while (!m_items.isEmpty()) delete m_items.takeFirst();
}

void BoxLayout::addItem(QLayoutItem *item) { m_items.append(item); }

QLayoutItem *BoxLayout::takeAt(int i) { return i >= 0 && i < m_items.size() ? m_items.takeAt(i) : nullptr; }

void BoxLayout::addWidget(QWidget *w, Qt::Alignment align) {
  QLayout::addWidget(w);
  if (align) setAlignment(w, align);
}

void BoxLayout::addSpacing(int px) {
  addItem(m_o == Qt::Horizontal ? new QSpacerItem(px, 0, QSizePolicy::Fixed, QSizePolicy::Minimum)
                                : new QSpacerItem(0, px, QSizePolicy::Minimum, QSizePolicy::Fixed));
  invalidate();
}

void BoxLayout::addStretch() {
  addItem(m_o == Qt::Horizontal ? new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum)
                                : new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
  invalidate();
}

// A child that fills along `dir`: its policy expands and it may grow past its hint.
static bool canGrow(QLayoutItem *it, Qt::Orientation dir) {
  if (!(it->expandingDirections() & dir)) return false;
  const QSize max = it->maximumSize(), hint = it->sizeHint();
  return dir == Qt::Horizontal ? max.width() > hint.width() : max.height() > hint.height();
}

static int gap(const QLayout *l) { return qMax(0, l->spacing()); }

static int itemHeight(QLayoutItem *it, int width) {
  if (it->hasHeightForWidth()) return it->heightForWidth(width);
  return it->sizeHint().height();
}

QSize BoxLayout::hint(int width) const {
  const QMargins m = contentsMargins();
  int main = 0, cross = 0, n = 0;
  for (QLayoutItem *it : m_items) {
    if (it->isEmpty()) continue;
    const QSize s = it->sizeHint();
    if (m_o == Qt::Horizontal) {
      main += s.width();
      cross = qMax(cross, s.height());
    } else {
      int w = s.width();
      if (width >= 0 && (it->expandingDirections() & Qt::Horizontal)) w = width - m.left() - m.right();
      if (width >= 0) w = qMin(w, width - m.left() - m.right());
      main += width >= 0 ? itemHeight(it, w) : s.height();
      cross = qMax(cross, s.width());
    }
    ++n;
  }
  if (n > 1) main += gap(this) * (n - 1);
  QSize out = m_o == Qt::Horizontal ? QSize(main, cross) : QSize(cross, main);
  return out + QSize(m.left() + m.right(), m.top() + m.bottom());
}

QSize BoxLayout::sizeHint() const { return hint(-1); }
QSize BoxLayout::minimumSize() const {
  // Like Qt Quick Layouts, nothing below the implicit size along the main axis.
  return QSize(0, 0);
}

bool BoxLayout::hasHeightForWidth() const {
  if (m_o == Qt::Horizontal) return false;
  for (QLayoutItem *it : m_items)
    if (!it->isEmpty() && it->hasHeightForWidth()) return true;
  return false;
}

int BoxLayout::heightForWidth(int w) const { return hint(w).height(); }

Qt::Orientations BoxLayout::expandingDirections() const {
  Qt::Orientations o;
  for (QLayoutItem *it : m_items)
    if (!it->isEmpty()) o |= it->expandingDirections();
  return o;
}

void BoxLayout::setGeometry(const QRect &rect) {
  QLayout::setGeometry(rect);
  const QRect r = rect.marginsRemoved(contentsMargins());
  QList<QLayoutItem *> items;
  for (QLayoutItem *it : m_items)
    if (!it->isEmpty()) items << it;
  if (items.isEmpty()) return;
  const bool h = m_o == Qt::Horizontal;
  const Qt::Orientation mainDir = h ? Qt::Horizontal : Qt::Vertical;
  const Qt::Orientation crossDir = h ? Qt::Vertical : Qt::Horizontal;
  const int crossRoom = h ? r.height() : r.width();

  // Cross sizes first: a Column's heights can depend on widths.
  QList<int> cross(items.size()), main(items.size());
  int fixed = gap(this) * int(items.size() - 1), fills = 0;
  for (int i = 0; i < items.size(); ++i) {
    QLayoutItem *it = items[i];
    const QSize s = it->sizeHint();
    const bool fillCross = it->expandingDirections() & crossDir;
    const int c = h ? s.height() : s.width();
    cross[i] = fillCross ? crossRoom : qMin(c, crossRoom);
    if (fillCross) cross[i] = qMin(cross[i], h ? it->maximumSize().height() : it->maximumSize().width());
    main[i] = h ? s.width() : itemHeight(it, cross[i]);
    fixed += main[i];
    if (canGrow(it, mainDir)) ++fills;
  }
  const int room = (h ? r.width() : r.height()) - fixed;
  if (fills > 0 && room != 0) {
    int left = room, k = fills;
    for (int i = 0; i < items.size(); ++i) {
      if (!canGrow(items[i], mainDir)) continue;
      const int add = left / k;
      const int most = h ? items[i]->maximumSize().width() : items[i]->maximumSize().height();
      main[i] = qBound(0, main[i] + add, most);
      left -= add;
      --k;
    }
  }
  int pos = h ? r.left() : r.top();
  for (int i = 0; i < items.size(); ++i) {
    QLayoutItem *it = items[i];
    const Qt::Alignment a = it->alignment();
    int off = 0;
    if (h) {
      if (a & Qt::AlignTop) off = 0;
      else if (a & Qt::AlignBottom) off = crossRoom - cross[i];
      else off = int(std::lround((crossRoom - cross[i]) / 2.0));
      it->setGeometry(QRect(pos, r.top() + off, main[i], cross[i]));
    } else {
      if (a & Qt::AlignHCenter) off = int(std::lround((crossRoom - cross[i]) / 2.0));
      else if (a & Qt::AlignRight) off = crossRoom - cross[i];
      it->setGeometry(QRect(r.left() + off, pos, cross[i], main[i]));
    }
    pos += main[i] + gap(this);
  }
}

}  // namespace Omarchy
