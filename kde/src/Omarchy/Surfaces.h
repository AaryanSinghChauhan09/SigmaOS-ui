#pragma once

// Surfaces and text pieces (spec/components.md, components/surfaces.md): Surface (a widget
// drawn as a BorderSurface), SectionHeader, Separator, Heading, Card, Pane, MenuRow, Tooltip
// and ConfirmDialog.

#include <QPointer>
#include <functional>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

namespace Omarchy {

// A plain container drawn as the kit's BorderSurface: a fill, a border spec, the theme radius.
// Set the colors with a callback so they follow the theme (and its fade).
class OMARCHY_EXPORT Surface : public QWidget {
  Q_OBJECT
public:
  using Look = std::function<void(const Tokens &, QColor &fill, Border &border, qreal &radius)>;
  explicit Surface(Look look = {}, QWidget *parent = nullptr);
  void setLook(Look look);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  Look m_look;
};

// The caption-sized bold dim label over a group of rows (PanelSectionHeader).
class OMARCHY_EXPORT SectionHeader : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
public:
  explicit SectionHeader(const QString &text = QString(), QWidget *parent = nullptr);
  QString text() const { return m_text; }
  void setText(const QString &t);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_text;
};

// 1px foreground at 12%, full width (PanelSeparator).
class OMARCHY_EXPORT Separator : public QWidget {
  Q_OBJECT
public:
  explicit Separator(QWidget *parent = nullptr);
  void setStrength(qreal s);
  void setVertical(bool on);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  qreal m_strength = 0.12;
  bool m_vertical = false;
};

// A text line at a font token, in a color role; for a view's own labels. Elides.
class OMARCHY_EXPORT Label : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
public:
  enum Role { Foreground, Dim, Faint, Accent, Urgent, Success, Warning };
  explicit Label(const QString &text = QString(), const char *font = "body", Role role = Foreground, QWidget *parent = nullptr);
  QString text() const { return m_text; }
  void setText(const QString &t);
  void setBold(bool on);
  void setRole(Role r);
  void setColor(const QColor &c);
  void setFontToken(const char *token);
  void setAlignment(Qt::Alignment a);
  void setWordWrap(bool on);
  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return m_wrap; }
  int heightForWidth(int w) const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QFont font() const;
  QString m_text;
  QByteArray m_font;
  Role m_role;
  QColor m_color;
  bool m_bold = false, m_wrap = false;
  Qt::Alignment m_align = Qt::AlignLeft | Qt::AlignVCenter;
};

// A pane's, toolbar's or page's title: a bold line at "title", "heading" or "display", an
// optional dim subtitle, and trailing actions (addTrailing).
class OMARCHY_EXPORT Heading : public QWidget {
  Q_OBJECT
public:
  explicit Heading(const QString &text = QString(), const QString &subtitle = QString(), QWidget *parent = nullptr);
  void setText(const QString &t);
  void setSubtitle(const QString &t);
  void setLevel(const QString &level);
  void setCentered(bool on);
  void addTrailing(QWidget *w);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;

private:
  int titleSize() const;
  int subtitleSize() const;
  qreal labelsHeight() const;
  int trailingInset() const;
  void place();
  QString m_text, m_subtitle, m_level = QStringLiteral("title");
  bool m_centered = false;
  QWidget *m_trailing = nullptr;
  Row *m_trailingRow = nullptr;
};

// The raised surface panels, popups and flyouts are drawn on: [popups] colors and border,
// panelPadding inside, children in a column panelGap apart under an optional section header.
class OMARCHY_EXPORT Card : public QWidget {
  Q_OBJECT
public:
  explicit Card(const QString &title = QString(), QWidget *parent = nullptr);
  void setTitle(const QString &t);
  void addWidget(QWidget *w);
  Column *body() const { return m_body; }
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  void relayout();
  Border border() const;
  SectionHeader *m_header;
  Column *m_column, *m_body;
};

// A flat surface inside a window: padding, a hairline rule on one side ("top", "right",
// "bottom", "left"), or `bordered` glass; a Heading over children panelGap apart.
class OMARCHY_EXPORT Pane : public QWidget {
  Q_OBJECT
public:
  explicit Pane(const QString &title = QString(), const QString &subtitle = QString(), QWidget *parent = nullptr);
  void setTitle(const QString &t);
  void setSubtitle(const QString &t);
  void setBordered(bool on);
  void setDivider(const QString &side);
  void addAction(QWidget *w);
  void addWidget(QWidget *w);
  Column *body() const { return m_body; }
  Heading *heading() const { return m_heading; }

protected:
  void paintEvent(QPaintEvent *) override;

private:
  void relayout();
  Heading *m_heading;
  Column *m_column, *m_body;
  bool m_bordered = false;
  QString m_divider;
};

// One row of a small popup menu: a label, an accent check when checked, the hover fill when
// highlighted (PopupMenuRow).
class OMARCHY_EXPORT MenuRow : public Control {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked)
  Q_PROPERTY(bool highlighted READ isHighlighted WRITE setHighlighted)
public:
  explicit MenuRow(const QString &text = QString(), QWidget *parent = nullptr);
  QString text() const { return m_text; }
  void setText(const QString &t);
  bool isChecked() const { return m_checked; }
  void setChecked(bool on);
  bool isHighlighted() const { return m_highlighted; }
  void setHighlighted(bool on);
  QSize sizeHint() const override;

Q_SIGNALS:
  void triggered();

protected:
  void paintEvent(QPaintEvent *) override;
  void activate(Qt::MouseButton b) override;

private:
  QString m_text;
  bool m_checked = false, m_highlighted = false;
};

// The tooltip surface with bodySmall text, as a still widget. Omarchy::Tooltip (Navigation.h) is
// the tooltip that shows on hover, with a shortcut in keycaps; stock QToolTips draw this surface
// under Omarchy::Style.
class OMARCHY_EXPORT TooltipFrame : public QWidget {
  Q_OBJECT
public:
  explicit TooltipFrame(const QString &text = QString(), QWidget *parent = nullptr);
  void setText(const QString &t);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_text;
};

// A modal confirmation: a message, Cancel, and a destructive Confirm in urgent, in a card over a
// scrim that fills its parent. Left/Right (h/l, Tab) move between the two, Return chooses,
// Escape or a click on the scrim cancels.
class OMARCHY_EXPORT ConfirmDialog : public QWidget {
  Q_OBJECT
public:
  explicit ConfirmDialog(const QString &message = QString(), QWidget *parent = nullptr);
  void setMessage(const QString &m);
  void setConfirmText(const QString &t);
  void setCancelText(const QString &t);
  int selectedIndex() const { return m_selected; }
  void setSelectedIndex(int i);
  // Shows it over its parent, focused, Confirm selected.
  void open();
  bool isOpen() const { return isVisible(); }
  QRect cardRect() const;
  QRect buttonRect(int index) const;

Q_SIGNALS:
  void confirmed();
  void canceled();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  bool event(QEvent *e) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  QString cancelText() const;
  QString confirmText() const;
  QString m_message, m_confirm, m_cancel;
  int m_selected = 1;
};

}  // namespace Omarchy
