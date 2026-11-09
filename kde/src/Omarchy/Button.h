#pragma once

// Button, IconButton and ButtonGroup (spec/components.md), drawn as the Qt Quick kit's Button,
// PanelActionButton and ButtonGroup: foreground glass, borderless at rest unless bordered,
// selected fills and bolds, focus looks like hover, a small give while pressed.

#include <QPointer>
#include <QStringList>
#include <QVariantAnimation>

#include "Omarchy/Control.h"

namespace Omarchy {

class OMARCHY_EXPORT Button : public Control {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(QString iconText READ iconText WRITE setIconText)
  Q_PROPERTY(bool selected READ isSelected WRITE setSelected)
  Q_PROPERTY(bool active READ isActive WRITE setActive)
  Q_PROPERTY(bool bordered READ isBordered WRITE setBordered)
  Q_PROPERTY(bool focusable READ isFocusable WRITE setFocusable)

public:
  explicit Button(const QString &text = QString(), QWidget *parent = nullptr);

  QString text() const { return m_text; }
  void setText(const QString &t);
  // A glyph (a Nerd Font codepoint) before the label.
  QString iconText() const { return m_icon; }
  void setIconText(const QString &t);
  bool isSelected() const { return m_selected; }
  void setSelected(bool on);
  bool isActive() const { return m_active; }
  void setActive(bool on);
  bool isBordered() const { return m_bordered; }
  void setBordered(bool on);
  // A Tab stop; Return, Enter and Space click it.
  bool isFocusable() const { return m_focusable; }
  void setFocusable(bool on);
  void setTooltipText(const QString &t) { setToolTip(t); }
  // Drawn in this color instead of the foreground (an urgent action); invalid = the theme's.
  void setForeground(const QColor &c);
  void setBackground(const QColor &c);
  void setFontToken(const char *token);
  void setFontPixelSize(int px);
  void setPadding(int horizontal, int vertical);
  void setLeftAlign(bool on);

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return QSize(0, sizeHint().height()); }
  // The exact implicit size, before a layout rounds it up.
  QSizeF implicitSize() const;

Q_SIGNALS:
  void clicked();
  void rightClicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void activate(Qt::MouseButton b) override;

private:
  QMargins reserved() const;
  QColor fg() const;
  int fontPx() const;
  int hpad() const;
  int vpad() const;
  void press(bool down);

  QString m_text, m_icon;
  bool m_selected = false, m_active = false, m_bordered = false, m_focusable = false, m_leftAlign = false;
  QColor m_fg, m_bg;
  QByteArray m_fontToken = "body";
  int m_fontPx = 0, m_hpad = -1, m_vpad = -1;
  qreal m_scale = 1.0;
  QVariantAnimation *m_give = nullptr;
};

// A square, odd-sized button with one glyph (PanelActionButton).
class OMARCHY_EXPORT IconButton : public Control {
  Q_OBJECT
  Q_PROPERTY(QString iconText READ iconText WRITE setIconText)
  Q_PROPERTY(bool bordered READ isBordered WRITE setBordered)

public:
  explicit IconButton(const QString &glyph = QString(), QWidget *parent = nullptr);

  QString iconText() const { return m_icon; }
  void setIconText(const QString &t);
  bool isBordered() const { return m_bordered; }
  void setBordered(bool on);
  bool isFocusable() const { return m_focusable; }
  void setFocusable(bool on);
  void setTooltipText(const QString &t) { setToolTip(t); }
  // The hover tint: the urgent color for a destructive action.
  void setHoverColor(const QColor &c);
  void setForeground(const QColor &c);
  void setFontPixelSize(int px);

  QSize sizeHint() const override;

Q_SIGNALS:
  void clicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void activate(Qt::MouseButton b) override;

private:
  int side() const;
  QString m_icon;
  bool m_bordered = false, m_focusable = false;
  QColor m_fg, m_hover;
  int m_fontPx = 0;
};

// Bordered buttons `md` apart, one selected: a segmented choice. One Tab stop; Left/Right (h/l)
// walk it, Return/Space choose.
class OMARCHY_EXPORT ButtonGroup : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY changed)

public:
  struct Option {
    QString value, label, icon, tooltip;
  };
  explicit ButtonGroup(QWidget *parent = nullptr);

  void setOptions(const QList<Option> &options);
  void setOptions(const QStringList &labels);
  QList<Option> options() const { return m_options; }
  QString value() const { return m_value; }
  void setValue(const QString &v);
  // The panel cursor (-1 = none).
  int cursorIndex() const { return m_cursor; }
  void setCursorIndex(int i);
  int focusedIndex() const { return m_focused; }
  Button *button(int i) const { return m_buttons.value(i); }

  QSize sizeHint() const override;

Q_SIGNALS:
  // A different option was chosen (the caller sets value).
  void changed(const QString &value);
  void hovered(int index, bool on);

protected:
  void keyPressEvent(QKeyEvent *e) override;
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;

private:
  void rebuild();
  void refresh();
  void place();
  QList<Option> m_options;
  QList<Button *> m_buttons;
  QString m_value;
  int m_cursor = -1, m_focused = -1;
};

}  // namespace Omarchy
