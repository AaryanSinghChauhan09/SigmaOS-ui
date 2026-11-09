#pragma once

// The base of the kit's widgets: follows the theme (repaints on every color frame, relayouts
// when sizes change), tracks hover and press, and carries `hasCursor`, the flag that forces the
// hover-cursor look for keyboard-driven lists and still renders (spec/components.md).

#include <QWidget>

#include "Omarchy/Export.h"
#include "Omarchy/Theme.h"

namespace Omarchy {

// Make any widget follow the theme: update() on each color frame, updateGeometry() and update()
// when sizes or fonts change. Kit widgets built on stock ones (TextField) call it.
OMARCHY_EXPORT void follow(QWidget *w);

// The colors on screen now (Theme::instance()->shown()).
OMARCHY_EXPORT const Tokens &shown();

class OMARCHY_EXPORT Control : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool hasCursor READ hasCursor WRITE setHasCursor NOTIFY hasCursorChanged)

public:
  explicit Control(QWidget *parent = nullptr);

  bool hasCursor() const { return m_hasCursor; }
  void setHasCursor(bool on);
  bool isHovered() const { return m_hovered; }
  bool isPressed() const { return m_pressed; }
  // Hover or cursor: the hover-cursor state.
  bool isHot() const { return (m_hovered || m_hasCursor) && isEnabled(); }

  // Where it draws inside its geometry: all of it, unless a positioner placed it at a fraction
  // of a pixel (Qt Quick's Row does), when the widget covers the whole pixels around that.
  QRectF paintRect() const { return m_paintRect.isValid() ? m_paintRect : QRectF(rect()); }
  void setPaintRect(const QRectF &r);
  // Places it at a fractional rectangle in its parent: geometry rounded out, paintRect inside.
  void setGeometryF(const QRectF &r);

Q_SIGNALS:
  void hasCursorChanged(bool on);
  void hovered(bool on);

protected:
  const Tokens &t() const { return shown(); }
  void enterEvent(QEnterEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;
  void changeEvent(QEvent *e) override;
  // A left click released inside: the default emits nothing; subclasses act on it.
  virtual void activate(Qt::MouseButton) {}
  // Called when the theme's sizes change, before updateGeometry().
  virtual void themeMetricsChanged() {}

private:
  bool m_hasCursor = false;
  bool m_hovered = false;
  bool m_pressed = false;
  QRectF m_paintRect;
};

}  // namespace Omarchy
