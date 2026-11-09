#pragma once

// Navigation (spec/components/navigation.md): Menu (MenuItem, MenuSeparator, MenuSection,
// submenus) and ContextMenu, Keycap and KeyCombo, ShortcutSheet, Tooltip with a shortcut,
// Toolbar (ToolbarSeparator), Tabs and IconRail, drawn as the Qt Quick kit draws them.
//
// A Menu is a real popup (a Qt::Popup window) until it is `held`, when it is an ordinary child
// widget drawn open in place (galleries, still renders, a panel's own menu):
//
//   auto *menu = new Omarchy::Menu(window);
//   menu->setTrigger(fileButton);                       // open() drops it below the trigger
//   menu->addItem(QStringLiteral("Open…"), QStringLiteral("Ctrl+O"), glyph);
//   menu->addSeparator();
//   auto *exportRow = menu->addItem(QStringLiteral("Export"));
//   exportRow->setSubmenu(exportMenu);                  // Right/l or a click opens it
//   connect(fileButton, &Omarchy::Button::clicked, menu, &Omarchy::Menu::open);
//
//   auto *ctx = new Omarchy::ContextMenu(window);
//   ctx->setTarget(canvas);                             // right-clicks open it at the pointer

#include <QList>
#include <QPair>
#include <QPointer>
#include <QStringList>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

class QVariantAnimation;
class QTimer;

namespace Omarchy {

class Menu;
class IconButton;

// ------------------------------------------------------------------------------------------ Menu

// One row of a Menu: [glyph] label ··· [shortcut hint] [mark]. Under the pointer or the menu's
// cursor it takes the menu's selected background and text; disabled rows are faint and inert;
// destructive rows are urgent.
class OMARCHY_EXPORT MenuItem : public Control {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(QString iconText READ iconText WRITE setIconText)
  Q_PROPERTY(QString shortcut READ shortcut WRITE setShortcut)
  Q_PROPERTY(bool checkable READ isCheckable WRITE setCheckable)
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
  Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive)
  Q_PROPERTY(bool destructive READ isDestructive WRITE setDestructive)
  Q_PROPERTY(bool hasSubmenu READ hasSubmenu WRITE setHasSubmenu)
  Q_PROPERTY(bool highlighted READ isHighlighted)
  Q_PROPERTY(bool hot READ isHot)

public:
  explicit MenuItem(const QString &text = QString(), QWidget *parent = nullptr);

  QString text() const { return m_text; }
  void setText(const QString &t);
  // A leading glyph (a Nerd Font codepoint).
  QString iconText() const { return m_icon; }
  void setIconText(const QString &t);
  // The dim hint at the right, plain text ("Ctrl+S").
  QString shortcut() const { return m_shortcut; }
  void setShortcut(const QString &s);
  bool isCheckable() const { return m_checkable; }
  void setCheckable(bool on);
  bool isChecked() const { return m_checked; }
  void setChecked(bool on);
  // A radio dot instead of a check: one of a set of exclusive choices.
  bool isExclusive() const { return m_exclusive; }
  void setExclusive(bool on);
  bool isDestructive() const { return m_destructive; }
  void setDestructive(bool on);
  // The menu Right/l or a click opens; the row takes ownership of a parentless one.
  Menu *submenu() const { return m_submenu; }
  void setSubmenu(Menu *m);
  // Draws the chevron without a submenu (a still render); defaults to submenu() != nullptr.
  bool hasSubmenu() const { return m_hasSubmenuSet ? m_hasSubmenu : m_submenu != nullptr; }
  void setHasSubmenu(bool on);

  // The Menu it is a row of.
  Menu *menu() const;
  // Under the menu's keyboard cursor.
  bool isHighlighted() const;
  // Enabled and under the pointer, the cursor or hasCursor.
  bool isHot() const;

  QSizeF implicitSize() const;
  QSize sizeHint() const override;

public Q_SLOTS:
  // Runs the row: opens its submenu, or emits triggered() and lets the menu close.
  void activate();

Q_SIGNALS:
  void triggered();
  void toggled(bool checked);

protected:
  void paintEvent(QPaintEvent *) override;
  void enterEvent(QEnterEvent *e) override;
  void activate(Qt::MouseButton b) override;

private:
  bool reserveIcon() const;
  bool reserveMark() const;
  QString m_text, m_icon, m_shortcut;
  bool m_checkable = false, m_checked = false, m_exclusive = false, m_destructive = false;
  bool m_hasSubmenu = false, m_hasSubmenuSet = false;
  QPointer<Menu> m_submenu;
};

// A 1px track rule between groups of rows, sm above and below.
class OMARCHY_EXPORT MenuSeparator : public Control {
  Q_OBJECT
public:
  explicit MenuSeparator(QWidget *parent = nullptr);
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
};

// A section header over a group of rows, inset like the labels.
class OMARCHY_EXPORT MenuSection : public Control {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
public:
  explicit MenuSection(const QString &text = QString(), QWidget *parent = nullptr);
  QString text() const { return m_text; }
  void setText(const QString &t);
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_text;
};

// The menu surface holding its rows. Keyboard while open (it holds focus): Up/Down or k/j move
// the cursor over enabled rows and wrap, Home/End jump, Enter or Space runs the row, Right or l
// opens its submenu with the first row under the cursor, Left, h or Esc leave a submenu, Esc
// closes. A press outside closes it. Running a row closes the whole chain (a held menu stays).
class OMARCHY_EXPORT Menu : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool opened READ isOpened NOTIFY openedChanged)
  Q_PROPERTY(bool held READ isHeld WRITE setHeld)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
  Q_PROPERTY(int gap READ gap WRITE setGap)
  Q_PROPERTY(int minimumWidth READ minimumMenuWidth WRITE setMinimumMenuWidth)
  Q_PROPERTY(bool hasIcons READ hasIcons)
  Q_PROPERTY(bool hasMarks READ hasMarks)

public:
  explicit Menu(QWidget *parent = nullptr);

  // Rows, in order. The convenience forms create the row and return it.
  MenuItem *addItem(const QString &text, const QString &shortcut = QString(), const QString &iconText = QString());
  void addItem(MenuItem *item);
  MenuSeparator *addSeparator();
  MenuSection *addSection(const QString &text);

  bool isOpened() const { return m_opened; }
  // Drawn open in place as a child widget: no positioning, no dismissal.
  bool isHeld() const { return m_held; }
  void setHeld(bool on);
  // The widget open() drops it below, gap() (xs by default) away.
  QWidget *trigger() const { return m_trigger; }
  void setTrigger(QWidget *w);
  int gap() const;
  void setGap(int px);
  // At least this wide (space(160) by default; -1 resets).
  int minimumMenuWidth() const;
  void setMinimumMenuWidth(int px);

  // The keyboard cursor, an index into rows() (-1: none).
  int currentIndex() const { return m_current; }
  void setCurrentIndex(int i);
  QList<MenuItem *> rows() const;
  MenuItem *currentItem() const;
  bool hasIcons() const;
  bool hasMarks() const;
  // The menu this one is open as a submenu of (nullptr at the top).
  Menu *parentMenu() const { return m_parentMenu; }

  // Handles a key as the menu does; true when it was the menu's (for a panel's own key catcher).
  bool handleKey(QKeyEvent *e);

  // The exact implicit size: max(minimumWidth, widest row) + padding + border.
  QSizeF implicitSize() const;
  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return sizeHint(); }
  // Border plus padding on each side.
  QMarginsF insets() const;

public Q_SLOTS:
  // Opens below the trigger (or where it is, without one).
  void open();
  // Opens with its top-left at `globalPos`, kept on the screen.
  void popup(const QPoint &globalPos);
  void close();

Q_SIGNALS:
  // A row ran, in this menu or a submenu; the menu has closed itself unless held.
  void triggered(MenuItem *item);
  void closed();
  void openedChanged(bool opened);
  void currentIndexChanged(int index);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void hideEvent(QHideEvent *e) override;
  bool event(QEvent *e) override;

private:
  friend class MenuItem;
  void addRow(QWidget *w);
  void relayout();
  void openAt(const QPoint *globalPos);
  void place(QPoint globalPos);
  void closeSubmenus();
  void closeAll();
  void moveCursor(int step);
  void jump(bool toEnd);
  void openSubmenu(MenuItem *item);
  void itemTriggered(MenuItem *item);
  Menu *root();
  void setPopupFlags(bool popup);

  QList<QWidget *> m_children;
  QPointer<QWidget> m_trigger;
  QPointer<Menu> m_parentMenu;
  bool m_opened = false, m_held = false, m_closing = false;
  int m_current = -1, m_gap = -1, m_minWidth = -1;
};

// A Menu that opens at a point: popup(globalPos), or a right-click (a context menu event) on its
// target opens it at the pointer.
class OMARCHY_EXPORT ContextMenu : public Menu {
  Q_OBJECT
public:
  explicit ContextMenu(QWidget *parent = nullptr);
  QWidget *target() const { return m_target; }
  void setTarget(QWidget *w);

protected:
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  QPointer<QWidget> m_target;
};

// ------------------------------------------------------------------------------------------ Keys

// Key names as both bindings draw them: "ctrl" -> "Ctrl", "shift" -> "⇧", "k" -> "K".
OMARCHY_EXPORT QString keyLabel(const QString &key);
// "Ctrl+Shift+S" -> {"Ctrl", "Shift", "S"}; a lone "+" is the plus key ("Ctrl++").
OMARCHY_EXPORT QStringList splitCombo(const QString &combo);

// One key as a small cap: normal control glass, caption text centered by ink.
class OMARCHY_EXPORT Keycap : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString key READ key WRITE setKey)
  Q_PROPERTY(QString label READ label)
public:
  explicit Keycap(const QString &key = QString(), QWidget *parent = nullptr);
  QString key() const { return m_key; }
  void setKey(const QString &k);
  QString label() const { return keyLabel(m_key); }
  // Invalid: the theme's foreground.
  QColor foreground() const { return m_fg; }
  void setForeground(const QColor &c);
  QSize sizeHint() const override;

  // A cap's size, and drawing one, for widgets that paint keys themselves.
  static QSizeF capSize(const QString &key);
  static void paintCap(QPainter &p, const QRectF &r, const QString &key, const QColor &foreground = QColor());

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_key;
  QColor m_fg;
};

// A key combination as Keycaps, xxs apart.
class OMARCHY_EXPORT KeyCombo : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString keys READ keys WRITE setKeys)
public:
  explicit KeyCombo(const QString &keys = QString(), QWidget *parent = nullptr);
  QString keys() const { return m_keys; }
  void setKeys(const QString &k);
  QSize sizeHint() const override;

  static QSizeF comboSize(const QString &keys);
  static void paintCombo(QPainter &p, const QPointF &topLeft, const QString &keys);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_keys;
};

// Grouped keyboard shortcuts in `columns` columns: a section header over rows of a KeyCombo and
// what it does. The sheet's content, for a Card or a dialog.
class OMARCHY_EXPORT ShortcutSheet : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int columns READ columns WRITE setColumns)
public:
  struct Group {
    QString title;
    QList<QPair<QString, QString>> keys;  // (combo, description)
  };
  explicit ShortcutSheet(QWidget *parent = nullptr);
  QList<Group> groups() const { return m_groups; }
  void setGroups(const QList<Group> &groups);
  void addGroup(const QString &title, const QList<QPair<QString, QString>> &keys);
  int columns() const { return m_columns; }
  void setColumns(int n);
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QSizeF layout(QPainter *p) const;
  QList<Group> m_groups;
  int m_columns = 2;
};

// ------------------------------------------------------------------------------------- Tooltip

// The tooltip surface with bodySmall text and an optional shortcut's Keycaps after it. Not held,
// it is a tooltip window: it shows centered above its parent widget, sm away, `delay` ms after
// the pointer enters it, and hides when the pointer leaves or presses. Held, it is a still child
// widget (galleries).
class OMARCHY_EXPORT Tooltip : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(QString shortcut READ shortcut WRITE setShortcut)
  Q_PROPERTY(bool held READ isHeld WRITE setHeld)
  Q_PROPERTY(int delay READ delay WRITE setDelay)
public:
  explicit Tooltip(const QString &text = QString(), QWidget *parent = nullptr);
  QString text() const { return m_text; }
  void setText(const QString &t);
  QString shortcut() const { return m_shortcut; }
  void setShortcut(const QString &s);
  bool isHeld() const { return m_held; }
  void setHeld(bool on);
  int delay() const { return m_delay; }
  void setDelay(int ms);
  // Shows it now above its parent (or hides it), as the hover would.
  void popup();
  void dismiss();

  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  void watch(QWidget *w);
  QString m_text, m_shortcut;
  bool m_held = false;
  int m_delay = 400;
  QTimer *m_timer = nullptr;
  QVariantAnimation *m_fade = nullptr;
  QPointer<QWidget> m_watched;
};

// ------------------------------------------------------------------------------------- Toolbar

// The vertical rule between groups in a Toolbar: 1px track, sm clear each side, space(16) tall.
class OMARCHY_EXPORT ToolbarSeparator : public QWidget {
  Q_OBJECT
public:
  explicit ToolbarSeparator(QWidget *parent = nullptr);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
};

// An app toolbar: leading items at the left, center items centered on the bar, trailing items
// at the right, then an optional overflow IconButton (…); items xxs apart, the bar sm inside its
// edges with a 1px track rule along the bottom when divided.
class OMARCHY_EXPORT Toolbar : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool overflow READ hasOverflow WRITE setOverflow)
  Q_PROPERTY(bool overflowHasCursor READ overflowHasCursor WRITE setOverflowHasCursor)
  Q_PROPERTY(bool divided READ isDivided WRITE setDivided)
public:
  explicit Toolbar(QWidget *parent = nullptr);
  void addLeading(QWidget *w);
  void addCenter(QWidget *w);
  void addTrailing(QWidget *w);
  ToolbarSeparator *addSeparator();  // to the leading slot
  bool hasOverflow() const;
  void setOverflow(bool on);
  bool overflowHasCursor() const;
  void setOverflowHasCursor(bool on);
  bool isDivided() const { return m_divided; }
  void setDivided(bool on);
  IconButton *overflowButton() const { return m_overflow; }

  QSize sizeHint() const override;

Q_SIGNALS:
  void overflowTriggered();

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  void place();
  void respace();
  QWidget *m_leading, *m_center, *m_end, *m_trailing;
  Row *m_leadingRow, *m_centerRow, *m_endRow, *m_trailingRow;
  IconButton *m_overflow;
  bool m_divided = true;
};

// --------------------------------------------------------------------------------- Tabs, rail

// Text tabs xxs apart over a 1px track rule, the selected one bold in the selected text color
// with an accent underline that slides between tabs. Keyboard (a Tab stop): Left/Right or h/l
// move the selection, Home/End jump.
class OMARCHY_EXPORT Tabs : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QStringList model READ model WRITE setModel)
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
  Q_PROPERTY(int cursorIndex READ cursorIndex WRITE setCursorIndex)
public:
  explicit Tabs(QWidget *parent = nullptr);
  QStringList model() const { return m_model; }
  void setModel(const QStringList &labels);
  int currentIndex() const { return m_current; }
  void setCurrentIndex(int i);
  // Forces the cursor look on one tab (-1: none).
  int cursorIndex() const { return m_cursor; }
  void setCursorIndex(int i);
  // Selects (clamped) and emits activated() when it changed.
  void select(int i);
  QRectF tabRect(int i) const;

  QSizeF implicitSize() const;
  QSize sizeHint() const override;

Q_SIGNALS:
  void activated(int index);
  void currentIndexChanged(int index);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;

private:
  int indexAt(const QPointF &p) const;
  void slide(int from);
  QStringList m_model;
  int m_current = 0, m_cursor = -1, m_hover = -1, m_pressed = -1;
  QVariantAnimation *m_anim;
  QRectF m_bar;  // the underline while it slides
};

// A vertical rail of icon tabs (an app's side panel switcher): square items sm apart, the
// selected glyph in the accent with an accent bar at the leading edge that slides between them,
// a 1px track rule down the trailing edge. Keyboard (a Tab stop): Up/Down or k/j, Home/End.
class OMARCHY_EXPORT IconRail : public QWidget {
  Q_OBJECT
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
  Q_PROPERTY(int cursorIndex READ cursorIndex WRITE setCursorIndex)
public:
  struct Item {
    QString icon, label;
  };
  explicit IconRail(QWidget *parent = nullptr);
  QList<Item> model() const { return m_model; }
  void setModel(const QList<Item> &items);
  int currentIndex() const { return m_current; }
  void setCurrentIndex(int i);
  int cursorIndex() const { return m_cursor; }
  void setCursorIndex(int i);
  void select(int i);
  // controlHeight + 2 × sm.
  int itemSize() const;
  QRectF itemRect(int i) const;

  QSize sizeHint() const override;

Q_SIGNALS:
  void activated(int index);
  void currentIndexChanged(int index);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;

private:
  int indexAt(const QPointF &p) const;
  qreal barY(int i) const;
  QList<Item> m_model;
  int m_current = 0, m_cursor = -1, m_hover = -1, m_pressed = -1;
  QVariantAnimation *m_anim;
  qreal m_barY = -1;
};

}  // namespace Omarchy
