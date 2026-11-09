#include "Omarchy/Control.h"

#include <QEnterEvent>
#include <QMouseEvent>
#include <cmath>

namespace Omarchy {

const Tokens &shown() { return Theme::instance()->shown(); }

void follow(QWidget *w) {
  Theme *theme = Theme::instance();
  QObject::connect(theme, &Theme::colorsChanged, w, [w] { w->update(); });
  QObject::connect(theme, &Theme::metricsChanged, w, [w] {
    w->updateGeometry();
    w->update();
  });
}

Control::Control(QWidget *parent) : QWidget(parent) {
  setAttribute(Qt::WA_Hover);
  setMouseTracking(true);
  Theme *theme = Theme::instance();
  connect(theme, &Theme::colorsChanged, this, [this] { update(); });
  connect(theme, &Theme::metricsChanged, this, [this] {
    themeMetricsChanged();
    updateGeometry();
    update();
  });
}

void Control::setHasCursor(bool on) {
  if (m_hasCursor == on) return;
  m_hasCursor = on;
  update();
  emit hasCursorChanged(on);
}

void Control::setPaintRect(const QRectF &r) {
  m_paintRect = r;
  update();
}

void Control::setGeometryF(const QRectF &r) {
  const QRect g(int(std::floor(r.x())), int(std::floor(r.y())), 0, 0);
  const int right = int(std::ceil(r.right() - 1e-9)), bottom = int(std::ceil(r.bottom() - 1e-9));
  setGeometry(QRect(g.x(), g.y(), right - g.x(), bottom - g.y()));
  m_paintRect = QRectF(r.x() - g.x(), r.y() - g.y(), r.width(), r.height());
  update();
}

void Control::enterEvent(QEnterEvent *e) {
  QWidget::enterEvent(e);
  if (!m_hovered) {
    m_hovered = true;
    update();
    emit hovered(true);
  }
}

void Control::leaveEvent(QEvent *e) {
  QWidget::leaveEvent(e);
  if (m_hovered) {
    m_hovered = false;
    update();
    emit hovered(false);
  }
}

void Control::mousePressEvent(QMouseEvent *e) {
  if (e->button() == Qt::LeftButton || e->button() == Qt::RightButton) {
    m_pressed = true;
    if (focusPolicy() & Qt::ClickFocus) setFocus(Qt::MouseFocusReason);
    update();
    e->accept();
    return;
  }
  QWidget::mousePressEvent(e);
}

void Control::mouseReleaseEvent(QMouseEvent *e) {
  if (m_pressed) {
    m_pressed = false;
    update();
    if (rect().contains(e->position().toPoint())) activate(e->button());
    e->accept();
    return;
  }
  QWidget::mouseReleaseEvent(e);
}

void Control::focusInEvent(QFocusEvent *e) {
  QWidget::focusInEvent(e);
  update();
}

void Control::focusOutEvent(QFocusEvent *e) {
  QWidget::focusOutEvent(e);
  update();
}

void Control::changeEvent(QEvent *e) {
  QWidget::changeEvent(e);
  if (e->type() == QEvent::EnabledChange) update();
}

}  // namespace Omarchy
