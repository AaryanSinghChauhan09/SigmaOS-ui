#pragma once

// Feedback (spec/components/feedback.md): Meter, ProgressBar, Spinner, EmptyState, Banner and
// Toast, drawn as the Qt Quick kit draws them. Tones are "neutral", "info", "success",
// "warning" and "error" (Qt's strings).

#include <QList>
#include <QPointer>
#include <QVariantAnimation>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

namespace Omarchy {

class Button;
class IconButton;

// A determinate level on a track: an optional header (label, value text), the track, a fill
// from the leading edge in the tone's color (urgent when alarming) and optional marker ticks.
// With `indeterminate` a quarter-track segment sweeps across instead (still, mid-track, under
// reduced motion).
class OMARCHY_EXPORT Meter : public Control {
  Q_OBJECT
  Q_PROPERTY(qreal value READ value WRITE setValue)
  Q_PROPERTY(QString tone READ tone WRITE setTone)
  Q_PROPERTY(bool alarming READ isAlarming WRITE setAlarming)
  Q_PROPERTY(bool indeterminate READ isIndeterminate WRITE setIndeterminate)
  Q_PROPERTY(QString label READ label WRITE setLabel)
  Q_PROPERTY(QString valueText READ valueText WRITE setValueText)

public:
  explicit Meter(QWidget *parent = nullptr);
  explicit Meter(qreal value, QWidget *parent = nullptr);

  qreal value() const { return m_value; }
  void setValue(qreal v);
  QString tone() const { return m_tone; }
  void setTone(const QString &tone);
  bool isAlarming() const { return m_alarming; }
  void setAlarming(bool on);
  QList<qreal> markers() const { return m_markers; }
  void setMarkers(const QList<qreal> &fractions);
  QString label() const { return m_label; }
  void setLabel(const QString &t);
  QString valueText() const { return m_valueText; }
  void setValueText(const QString &t);
  bool isIndeterminate() const { return m_indeterminate; }
  void setIndeterminate(bool on);
  // The track's thickness in px; 0 = spacing.meterThickness.
  qreal thickness() const;
  void setThickness(qreal px);

  // The fill's color: the tone's, urgent when alarming.
  QColor fillColor() const;
  // The track inside the widget.
  QRectF trackRect() const;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return QSize(0, sizeHint().height()); }

protected:
  void paintEvent(QPaintEvent *) override;
  void showEvent(QShowEvent *e) override;
  void hideEvent(QHideEvent *e) override;

private:
  bool hasHeader() const { return !m_label.isEmpty() || !m_valueText.isEmpty(); }
  qreal headerHeight() const;
  qreal barHeight() const;
  void sync();
  qreal m_value = 0, m_shown = 0, m_thickness = 0, m_sweep = 0;
  QString m_tone = QStringLiteral("neutral"), m_label, m_valueText;
  QList<qreal> m_markers;
  bool m_alarming = false, m_indeterminate = false;
  QVariantAnimation *m_fill, *m_segment;
};

// A Meter with an accent ("info") fill: progress of a task, indeterminate for unknown length.
class OMARCHY_EXPORT ProgressBar : public Meter {
  Q_OBJECT
public:
  explicit ProgressBar(QWidget *parent = nullptr);
  explicit ProgressBar(qreal value, QWidget *parent = nullptr);
};

// Busy: a glyph turning once every 900 ms about its own middle, or with `pulse` breathing
// between half and full opacity (420 ms each way). Still (and a pulse at half opacity) under
// reduced motion or when not running.
class OMARCHY_EXPORT Spinner : public Control {
  Q_OBJECT
  Q_PROPERTY(QString glyph READ glyph WRITE setGlyph)
  Q_PROPERTY(bool pulse READ pulse WRITE setPulse)
  Q_PROPERTY(bool running READ isRunning WRITE setRunning)

public:
  explicit Spinner(QWidget *parent = nullptr);

  QString glyph() const { return m_glyph; }
  void setGlyph(const QString &g);
  // The glyph's pixel size; 0 = font.icon.
  int size() const;
  void setSize(int px);
  bool pulse() const { return m_pulse; }
  void setPulse(bool on);
  QColor color() const;
  void setColor(const QColor &c);
  bool isRunning() const { return m_running; }
  void setRunning(bool on);
  // Whether it is moving now (running, visible, motion allowed).
  bool isAnimating() const;
  qreal rotation() const { return m_angle; }
  qreal glyphOpacity() const;

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
  void showEvent(QShowEvent *e) override;
  void hideEvent(QHideEvent *e) override;
  void themeMetricsChanged() override { sync(); }

private:
  void sync();
  QString m_glyph;
  int m_size = 0;
  bool m_pulse = false, m_running = true;
  QColor m_color;
  qreal m_angle = 0, m_phase = 0;
  QVariantAnimation *m_anim;
};

// What a list or panel shows when it has nothing: a centered column of glyph, title, optional
// hint and optional bordered action, in the width it is given. `overlay` is the pickers' larger
// look.
class OMARCHY_EXPORT EmptyState : public Control {
  Q_OBJECT
  Q_PROPERTY(QString title READ title WRITE setTitle)
  Q_PROPERTY(QString glyph READ glyph WRITE setGlyph)
  Q_PROPERTY(QString hint READ hint WRITE setHint)
  Q_PROPERTY(QString actionText READ actionText WRITE setActionText)

public:
  explicit EmptyState(const QString &title = QString(), QWidget *parent = nullptr);

  QString title() const { return m_title; }
  void setTitle(const QString &t);
  QString glyph() const { return m_glyph; }
  void setGlyph(const QString &g);
  QString hint() const { return m_hint; }
  void setHint(const QString &t);
  QString actionText() const { return m_actionText; }
  void setActionText(const QString &t);
  // "panel" (the default) or "overlay".
  QString size() const { return m_overlay ? QStringLiteral("overlay") : QStringLiteral("panel"); }
  void setSize(const QString &s);
  void setOverlay(bool on) { setSize(on ? QStringLiteral("overlay") : QStringLiteral("panel")); }
  Button *actionButton() const { return m_action; }

  // The exact implicit size, and the height at a width.
  QSizeF implicitSize() const;
  qreal implicitHeightFor(qreal width) const;
  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void actionClicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void themeMetricsChanged() override { place(); }

private:
  struct Parts {
    qreal glyphY, glyphH, titleY, titleH, hintY, hintH, actionY, height;
  };
  Parts parts(qreal width) const;
  QFont glyphFont() const;
  QFont titleFont() const;
  QFont hintFont() const;
  void place();
  QString m_title, m_glyph, m_hint, m_actionText;
  bool m_overlay = false;
  Button *m_action;
};

// An inline message: a tone glyph, the text (wraps), an optional bordered action and an
// optional dismiss button, on foreground glass with a border in the tone.
class OMARCHY_EXPORT Banner : public Control {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(QString tone READ tone WRITE setTone)
  Q_PROPERTY(QString glyph READ glyph WRITE setGlyph)
  Q_PROPERTY(QString actionText READ actionText WRITE setActionText)
  Q_PROPERTY(bool dismissible READ isDismissible WRITE setDismissible)

public:
  explicit Banner(const QString &text = QString(), QWidget *parent = nullptr);

  QString text() const { return m_text; }
  void setText(const QString &t);
  // "info" (the default; "neutral" draws as info), "success", "warning", "error".
  QString tone() const { return m_tone; }
  void setTone(const QString &t);
  // Replaces the tone's glyph.
  QString glyph() const { return m_glyph; }
  void setGlyph(const QString &g);
  QString actionText() const { return m_actionText; }
  void setActionText(const QString &t);
  bool isDismissible() const { return m_dismissible; }
  void setDismissible(bool on);
  QColor toneColor() const;
  QString toneGlyph() const;
  Button *actionButton() const { return m_action; }
  IconButton *dismissButton() const { return m_dismiss; }

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void actionClicked();
  void dismissed();

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void themeMetricsChanged() override { place(); }

private:
  struct Geo {
    qreal height, rowY, rowH, glyphW, textX, textW, textH, actionX, actionW, dismissX;
  };
  Geo geometry(qreal width) const;
  QMargins insets() const;
  void place();
  QString m_text, m_tone = QStringLiteral("info"), m_glyph, m_actionText;
  bool m_dismissible = false;
  Button *m_action;
  IconButton *m_dismiss;
};

// A transient message inside the app, on the notifications surface: an optional glyph, a bold
// title, wrapped text, an optional bordered action and a countdown line (1 → 0; below 0 hides
// it). It draws one state; the app owns when it shows and the timer.
class OMARCHY_EXPORT Toast : public Control {
  Q_OBJECT
  Q_PROPERTY(QString title READ title WRITE setTitle)
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(QString glyph READ glyph WRITE setGlyph)
  Q_PROPERTY(QString actionText READ actionText WRITE setActionText)
  Q_PROPERTY(qreal countdown READ countdown WRITE setCountdown)

public:
  explicit Toast(const QString &title = QString(), QWidget *parent = nullptr);

  QString title() const { return m_title; }
  void setTitle(const QString &t);
  QString text() const { return m_text; }
  void setText(const QString &t);
  QString glyph() const { return m_glyph; }
  void setGlyph(const QString &g);
  QString actionText() const { return m_actionText; }
  void setActionText(const QString &t);
  qreal countdown() const { return m_countdown; }
  void setCountdown(qreal left);
  Button *actionButton() const { return m_action; }

  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void actionClicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void themeMetricsChanged() override { place(); }

private:
  struct Geo {
    qreal height, rowH, glyphW, colX, colW, titleH, textY, textH, actionX, actionW;
  };
  Geo geometry(qreal width) const;
  void place();
  QString m_title, m_text, m_glyph, m_actionText;
  qreal m_countdown = -1;
  Button *m_action;
};

}  // namespace Omarchy
