#pragma once

// ToggleSwitch and Toggle (spec/components.md): a bare switch with a cursor ring around it, and
// a labeled row (title, description) with the switch at its end where the whole row is the
// control. Square on sharp corners, a pill when Hyprland rounds them.

#include <QVariantAnimation>

#include "Omarchy/Control.h"

namespace Omarchy {

class OMARCHY_EXPORT ToggleSwitch : public Control {
  Q_OBJECT
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
  explicit ToggleSwitch(QWidget *parent = nullptr);

  bool isChecked() const { return m_checked; }
  // Sets the state (the knob slides). The caller owns it: a click emits toggled() and, unless
  // setAutoToggle(false), flips it.
  void setChecked(bool on);
  void setAutoToggle(bool on) { m_auto = on; }
  // In flight: clicks are swallowed, hover and cursor stay.
  void setBusy(bool on) { m_busy = on; }
  // Off when a surrounding row owns the click and the cursor (as in Toggle).
  void setInteractive(bool on);
  // A smaller track for compact placements; 0 = the token size.
  void setTrackHeight(int px);

  QSize sizeHint() const override;
  QRectF trackRect() const;

Q_SIGNALS:
  void toggled(bool checked);
  void clicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void activate(Qt::MouseButton b) override;

private:
  int trackHeight() const;
  int pad() const;
  bool m_checked = false, m_auto = true, m_busy = false, m_interactive = true;
  int m_trackHeight = 0;
  qreal m_knob = 0;  // 0 off .. 1 on
  QVariantAnimation *m_slide;
  friend class Toggle;
};

class OMARCHY_EXPORT Toggle : public Control {
  Q_OBJECT
  Q_PROPERTY(QString label READ label WRITE setLabel)
  Q_PROPERTY(QString description READ description WRITE setDescription)
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
public:
  explicit Toggle(const QString &label = QString(), const QString &description = QString(), QWidget *parent = nullptr);

  QString label() const { return m_label; }
  void setLabel(const QString &t);
  QString description() const { return m_description; }
  void setDescription(const QString &t);
  bool isChecked() const;
  void setChecked(bool on);
  void setAutoToggle(bool on) { m_auto = on; }
  ToggleSwitch *toggleSwitch() const { return m_switch; }

  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void clicked();
  void toggled(bool checked);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void activate(Qt::MouseButton b) override;
  void themeMetricsChanged() override { place(); }

private:
  void place();
  qreal contentHeight(int width) const;
  QString m_label, m_description;
  bool m_auto = true;
  ToggleSwitch *m_switch;
};

}  // namespace Omarchy
