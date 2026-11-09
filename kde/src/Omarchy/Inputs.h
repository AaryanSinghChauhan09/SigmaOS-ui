#pragma once

// TextField, NumberField and Dropdown (spec/components.md): inputs drawn as the Qt Quick kit
// draws them (the normal glass and border at rest, focus looks like hover), with Qt's own line
// editing underneath.

#include <QLineEdit>
#include <QPointer>

#include "Omarchy/Control.h"

namespace Omarchy {

// A single-line input: a QLineEdit with the kit's surface, padding, placeholder and selection.
class OMARCHY_EXPORT TextField : public QLineEdit {
  Q_OBJECT
  Q_PROPERTY(bool hasCursor READ hasCursor WRITE setHasCursor)
public:
  explicit TextField(QWidget *parent = nullptr);
  explicit TextField(const QString &text, QWidget *parent = nullptr);

  bool hasCursor() const { return m_hasCursor; }
  void setHasCursor(bool on);
  void setPassword(bool on) { setEchoMode(on ? QLineEdit::Password : QLineEdit::Normal); }
  // Padding inside the border (default controlPaddingX × inputPaddingY); -1 = the token.
  void setPadding(int horizontal, int vertical);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;

Q_SIGNALS:
  void hovered(bool on);

protected:
  void paintEvent(QPaintEvent *e) override;
  void enterEvent(QEnterEvent *e) override;
  void leaveEvent(QEvent *e) override;
  bool event(QEvent *e) override;

private:
  void retheme();
  bool hot() const { return m_hovered || m_hasCursor; }
  bool m_hasCursor = false, m_hovered = false;
  int m_padX = -1, m_padY = -1;
};

// An integer stepper with a label: [−] value [+]. Up/Down (and k/j when the value isn't being
// typed), PageUp/PageDown and the wheel step it; typing a number sets it.
class OMARCHY_EXPORT NumberField : public Control {
  Q_OBJECT
  Q_PROPERTY(int value READ value WRITE setValue NOTIFY modified)
public:
  explicit NumberField(const QString &label = QString(), QWidget *parent = nullptr);

  QString label() const { return m_label; }
  void setLabel(const QString &t);
  int value() const { return m_value; }
  void setValue(int v);
  void setRange(int from, int to);
  int from() const { return m_from; }
  int to() const { return m_to; }
  void setStepSize(int s) { m_step = qMax(1, s); }
  void stepUp() { step(1); }
  void stepDown() { step(-1); }
  QLineEdit *editor() const { return m_edit; }
  QRectF fieldRect() const;
  QRectF downRect() const;
  QRectF upRect() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  // The user changed the value (typing, a step, a key, the wheel).
  void modified(int value);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  bool eventFilter(QObject *o, QEvent *e) override;
  void themeMetricsChanged() override { place(); }

private:
  void step(int dir);
  void place();
  void sync();
  qreal labelHeight() const;
  QString m_label;
  int m_value = 0, m_from = 0, m_to = 100, m_step = 1;
  int m_pressedPart = 0, m_hoverPart = 0;  // -1 down, 1 up
  QLineEdit *m_edit;
};

// A labeled single choice; the list opens below the trigger on the popups surface.
// Return, Enter, Space, Down or j opens it; j/k or Up/Down walk it; Return picks; Escape closes.
class OMARCHY_EXPORT Dropdown : public Control {
  Q_OBJECT
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY changed)
public:
  struct Option {
    QString value, label;
  };
  explicit Dropdown(const QString &label = QString(), QWidget *parent = nullptr);

  QString label() const { return m_label; }
  void setLabel(const QString &t);
  void setShowLabel(bool on);
  QList<Option> options() const { return m_options; }
  void setOptions(const QList<Option> &options);
  void setOptions(const QStringList &labels);
  QString value() const { return m_value; }
  void setValue(const QString &v);
  QString currentLabel() const;
  bool isPopupOpen() const;
  void open();
  void close();
  QWidget *popup() const;
  int popupIndex() const;
  QRectF triggerRect() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  void changed(const QString &value);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void activate(Qt::MouseButton b) override;
  void mousePressEvent(QMouseEvent *e) override;

private:
  friend class DropdownPopup;
  void choose(int index);
  qreal labelHeight() const;
  QString m_label, m_value;
  bool m_showLabel = true;
  QList<Option> m_options;
  QPointer<class DropdownPopup> m_popup;
};

}  // namespace Omarchy
