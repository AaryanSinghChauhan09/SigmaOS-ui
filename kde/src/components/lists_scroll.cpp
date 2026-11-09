// ScrollArea: qt/qml/Omarchy/Ui/ScrollArea.qml. A Flickable over a Column (children at Qt Quick's
// fractional positions), a ScrollBar overlaid on the right edge while the content overflows.

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QVariant>
#include <QWheelEvent>
#include <cmath>

#include "Omarchy/Lists.h"
#include "Omarchy/Paint.h"

namespace Omarchy {

static int ceilPx(qreal v) { return int(std::ceil(v - 1e-6)); }

class ScrollArea::Bar : public QWidget {
public:
  explicit Bar(ScrollArea *area) : QWidget(area), m_area(area) {
    follow(this);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
  }
  bool pressed = false;

protected:
  void paintEvent(QPaintEvent *) override {
    const Tokens &tk = shown();
    QPainter p(this);
    const qreal r = tk.smallRadius();
    Paint::surface(p, QRectF(rect()), r, tk.track());
    const QRectF thumb = m_area->thumbRect().translated(-pos());
    const QColor c = pressed || underMouse() ? tk.textDim() : tk.border(State::Normal).color;
    Paint::surface(p, thumb, r, c);
  }
  void mousePressEvent(QMouseEvent *e) override {
    if (e->button() != Qt::LeftButton) return;
    const QRectF thumb = m_area->thumbRect().translated(-pos());
    m_grab = thumb.contains(e->position()) ? e->position().y() - thumb.y() : thumb.height() / 2;
    pressed = true;
    drag(e->position().y());
    update();
  }
  void mouseMoveEvent(QMouseEvent *e) override {
    if (pressed) drag(e->position().y());
  }
  void mouseReleaseEvent(QMouseEvent *) override {
    pressed = false;
    update();
  }
  bool event(QEvent *e) override {
    if (e->type() == QEvent::HoverEnter || e->type() == QEvent::HoverLeave) update();
    return QWidget::event(e);
  }

private:
  // The thumb's top at `y` less the grab offset: ScrollBar.position, then contentY.
  void drag(qreal y) {
    const qreal h = height();
    const qreal ch = m_area->contentHeight();
    if (h <= 0 || ch <= 0) return;
    const qreal size = qMin<qreal>(1, h / ch);
    const qreal minimum = qMin<qreal>(1, shown().space(24) / qMax<qreal>(1, h));
    qreal visual = (y - m_grab) / h;
    // Undo the minimum-size mapping (QQuickScrollBar's visual position).
    qreal position = minimum > size ? visual / qMax<qreal>(1e-9, 1 - minimum) * (1 - size) : visual;
    position = qBound<qreal>(0, position, 1 - size);
    m_area->setScrollY(position * ch);
  }
  ScrollArea *m_area;
  qreal m_grab = 0;
};

ScrollArea::ScrollArea(QWidget *parent) : QWidget(parent) {
  follow(this);
  QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
  sp.setHeightForWidth(true);
  setSizePolicy(sp);
  m_viewport = new QWidget(this);
  m_content = new QWidget(m_viewport);
  m_content->installEventFilter(this);
  m_bar = new Bar(this);
  m_bar->hide();
  connect(Theme::instance(), &Theme::metricsChanged, this, [this] {
    updateGeometry();
    place();
  });
}

void ScrollArea::setMaxHeight(int h) {
  m_maxHeight = h;
  updateGeometry();
  place();
}

void ScrollArea::setSpacing(int px) {
  m_spacing = px;
  updateGeometry();
  place();
}

void ScrollArea::addWidget(QWidget *w) {
  w->setParent(m_content);
  m_items << w;
  w->installEventFilter(this);
  w->show();
  updateGeometry();
  place();
}

qreal ScrollArea::itemHeight(QWidget *w, qreal width) const {
  if (auto *row = qobject_cast<ListRow *>(w)) return row->implicitSize(width).height();
  if (w->hasHeightForWidth()) return w->heightForWidth(int(std::lround(width)));
  const QVariant v = w->property("implicitSize");
  if (v.isValid() && v.canConvert<QSizeF>()) return v.toSizeF().height();
  return w->sizeHint().height();
}

qreal ScrollArea::contentHeight(qreal width) const {
  if (width < 0) width = this->width();
  qreal y = 0;
  int n = 0;
  for (QWidget *w : m_items) {
    if (w->isHidden()) continue;
    y += itemHeight(w, width);
    ++n;
  }
  return y + (n > 1 ? m_spacing * (n - 1) : 0);
}

qreal ScrollArea::maxScrollY() const { return qMax<qreal>(0, contentHeight() - height()); }

bool ScrollArea::isOverflowing() const { return contentHeight() > height() + 0.5; }

void ScrollArea::setScrollY(qreal y) {
  y = qBound<qreal>(0, y, maxScrollY());
  if (qFuzzyCompare(y + 1, m_scrollY + 1)) return;
  m_scrollY = y;
  place();
  emit scrolled(y);
}

void ScrollArea::ensureVisibleRange(qreal y, qreal h) {
  const qreal top = m_scrollY, bottom = top + height();
  if (y < top) setScrollY(qMax<qreal>(0, y));
  else if (y + h > bottom) setScrollY(qMin(maxScrollY(), y + h - height()));
}

QRectF ScrollArea::itemRect(QWidget *w) const {
  const int i = int(m_items.indexOf(w));
  return i >= 0 && i < m_rects.size() ? m_rects[i] : QRectF();
}

void ScrollArea::ensureVisible(QWidget *w) {
  if (!w) return;
  QRectF r = itemRect(w);
  if (r.isNull()) {
    // A widget inside a child: map it into the content.
    const QPoint p = w->mapTo(m_content, QPoint(0, 0));
    r = QRectF(p, QSizeF(w->size()));
  }
  ensureVisibleRange(r.y(), r.height());
}

QRectF ScrollArea::barRect() const {
  if (!isOverflowing()) return QRectF();
  const int thick = shown().spacing("scrollbarThickness");
  return QRectF(width() - thick, 0, thick, height());
}

// QQuickScrollBar's visual area: the thumb at least minimumSize long, the position remapped.
QRectF ScrollArea::thumbRect() const {
  const QRectF bar = barRect();
  if (bar.isEmpty()) return QRectF();
  const qreal h = bar.height();
  const qreal ch = contentHeight();
  const qreal size = qBound<qreal>(0, h / ch, 1);
  const qreal position = m_scrollY / ch;
  const qreal minimum = shown().space(24) / qMax<qreal>(1, h);
  qreal visualPos = position;
  if (minimum > size) visualPos = position / (1.0 - size) * (1.0 - minimum);
  qreal visualSize = qBound<qreal>(0, qMax(size, minimum) + qMin<qreal>(0, visualPos), qMax<qreal>(0, 1.0 - visualPos));
  visualPos = qBound<qreal>(0, visualPos, qMax<qreal>(0, 1.0 - visualSize));
  return QRectF(bar.x(), bar.y() + visualPos * h, bar.width(), visualSize * h);
}

void ScrollArea::place() {
  if (m_placing) return;
  m_placing = true;
  const qreal w = width();
  m_scrollY = qBound<qreal>(0, m_scrollY, maxScrollY());
  m_rects.clear();
  qreal y = 0;
  bool first = true;
  for (QWidget *child : m_items) {
    if (child->isHidden()) {
      m_rects << QRectF();
      continue;
    }
    if (!first) y += m_spacing;
    first = false;
    const qreal h = itemHeight(child, w);
    const QRectF r(0, y, w, h);
    m_rects << r;
    if (auto *c = qobject_cast<Control *>(child)) c->setGeometryF(r);
    else child->setGeometry(QRect(0, int(std::lround(y)), int(w), ceilPx(h)));
    y += h;
  }
  m_viewport->setGeometry(rect());
  m_content->setGeometry(0, -int(std::lround(m_scrollY)), int(w), qMax(ceilPx(y), height()));
  const QRectF bar = barRect();
  m_bar->setVisible(!bar.isEmpty());
  if (!bar.isEmpty()) {
    m_bar->setGeometry(bar.toAlignedRect());
    m_bar->raise();
    m_bar->update();
  }
  m_placing = false;
}

QSize ScrollArea::sizeHint() const {
  qreal w = 0;
  for (QWidget *c : m_items) {
    if (c->isHidden()) continue;
    const QVariant v = c->property("implicitSize");
    w = qMax<qreal>(w, v.isValid() && v.canConvert<QSizeF>() ? v.toSizeF().width() : c->sizeHint().width());
  }
  qreal h = contentHeight(w);
  if (m_maxHeight > 0) h = qMin<qreal>(h, m_maxHeight);
  return QSize(ceilPx(w), ceilPx(h));
}

int ScrollArea::heightForWidth(int w) const {
  qreal h = contentHeight(w);
  if (m_maxHeight > 0) h = qMin<qreal>(h, m_maxHeight);
  return ceilPx(h);
}

void ScrollArea::wheelEvent(QWheelEvent *e) {
  qreal dy = 0;
  if (!e->pixelDelta().isNull()) dy = e->pixelDelta().y();
  else dy = e->angleDelta().y() / 120.0 * 3 * 20;
  const qreal before = m_scrollY;
  setScrollY(m_scrollY - dy);
  // At an end, let an outer view take it.
  e->setAccepted(m_scrollY != before);
}

bool ScrollArea::eventFilter(QObject *o, QEvent *e) {
  switch (e->type()) {
  case QEvent::LayoutRequest:
  case QEvent::ShowToParent:
  case QEvent::HideToParent:
    if (o == m_content || m_items.contains(static_cast<QWidget *>(o))) {
      updateGeometry();
      place();
    }
    break;
  case QEvent::ChildRemoved:
    if (o == m_content) {
      auto *child = static_cast<QChildEvent *>(e)->child();
      m_items.removeAll(static_cast<QWidget *>(child));
      updateGeometry();
      place();
    }
    break;
  default: break;
  }
  return QWidget::eventFilter(o, e);
}

}  // namespace Omarchy
