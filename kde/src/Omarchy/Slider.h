#pragma once

// Slider (spec/components.md; the Qt Quick kit's ValueSlider / PanelSlider): a thin track in the
// selected fill, a foreground fill to the value, a round foreground knob ringed in the surface
// color, optional notches. Drag or click to set; the wheel, arrows (h/l), PageUp/PageDown, Home
// and End step it.

#include <QVariantAnimation>

#include "Omarchy/Control.h"

namespace Omarchy {

class OMARCHY_EXPORT Slider : public Control {
  Q_OBJECT
  Q_PROPERTY(qreal value READ value WRITE setValue)
public:
  explicit Slider(QWidget *parent = nullptr);

  qreal value() const { return m_value; }
  void setValue(qreal v);
  void setRange(qreal minimum, qreal maximum);
  qreal minimum() const { return m_min; }
  qreal maximum() const { return m_max; }
  void setStep(qreal s) { m_step = s; }
  void setInteger(bool on) { m_integer = on; }
  // Notches across the track (0 or 1 = none).
  void setTickCount(int n);
  bool isDragging() const { return m_dragging; }
  // The value shown: the drag's while dragging, else value().
  qreal liveValue() const { return m_live; }
  QRectF trackRect() const;
  QRectF knobRect() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  // While dragging or stepping, and on release; the caller sets value from them (setValue), or
  // leave setAutoSet(true), the default, to have it follow.
  void moved(qreal value);
  void released(qreal value);
  void rightClicked();

public:
  void setAutoSet(bool on) { m_autoSet = on; }

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;

private:
  qreal valueAt(qreal x) const;
  void nudge(qreal delta);
  int knobSize() const;
  qreal progress() const;
  qreal m_value = 0, m_live = 0, m_min = 0, m_max = 1, m_step = 0.05;
  qreal m_shown = 0;  // the knob's animated progress
  bool m_integer = false, m_dragging = false, m_autoSet = true;
  int m_ticks = 0;
  QVariantAnimation *m_anim;
};

}  // namespace Omarchy
