#pragma once

// Lists (spec/components/lists.md): ListRow, CursorModel, ScrollArea, KeyValue, Hero, Badge,
// StatusDot, Avatar and TileGrid, drawn as the Qt Quick kit's components of the same names.
//
//   auto *cursor = new Omarchy::CursorModel(panel);
//   cursor->setSections({{"wifi", 3}});
//   auto *scroll = new Omarchy::ScrollArea;  scroll->setMaxHeight(240);
//   auto *row = new Omarchy::ListRow(glyph, "Basecamp");
//   row->setStatus("Connected", "success");
//   scroll->addWidget(row);
//   cursor->attach(row, "wifi", 0);            // hasCursor follows the model, the pointer moves it
//   connect(cursor, &CursorModel::moved, ...)  // reveal: scroll->ensureVisible(row) when byKey
//
// Tones are strings, as in Qt Quick: "" (the component's own ink), "accent", "success",
// "warning", "urgent", "muted".

#include <QImage>
#include <QList>
#include <QPointer>
#include <QSizeF>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

class QKeyEvent;

namespace Omarchy {

class ListRow;
class TileGrid;

// ----------------------------------------------------------------------------------- CursorModel

// The keyboard cursor every panel re-implements: a section and a row within it, plus whether it
// shows. Non-visual: hand it a view's keys (handleKey) and its rows' pointer moves.
class OMARCHY_EXPORT CursorModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString focusSection READ focusSection NOTIFY changed)
  Q_PROPERTY(int selectedIndex READ selectedIndex NOTIFY changed)
  Q_PROPERTY(bool cursorActive READ isCursorActive NOTIFY changed)
  Q_PROPERTY(int columns READ columns WRITE setColumns)
  Q_PROPERTY(int pageSize READ pageSize WRITE setPageSize)

public:
  struct Section {
    QString name;
    int count = 0;
  };

  explicit CursorModel(QObject *parent = nullptr);

  QList<Section> sections() const { return m_sections; }
  void setSections(const QList<Section> &sections);
  // One unnamed section (a plain list or a grid).
  void setList(int count) { setSections({{QString(), count}}); }
  void setCount(const QString &section, int count);
  int count(const QString &section) const;

  QString focusSection() const { return m_section; }
  int selectedIndex() const { return m_index; }
  bool isCursorActive() const { return m_active; }
  int columns() const { return m_columns; }
  void setColumns(int n) { m_columns = qMax(1, n); }
  int pageSize() const { return m_pageSize; }
  void setPageSize(int n) { m_pageSize = n; }

  // A row's cursor flag.
  bool isAt(const QString &section, int index) const;
  bool cursorIn(const QString &section) const { return m_active && m_section == section; }

  void set(const QString &section, int index);
  // Hides the cursor.
  void reset();
  void move(int dx, int dy);
  void moveSection(int delta);
  void page(int delta);
  void home();
  void end();

  // A key the panels' way; true (and the event accepted) when it was a cursor key.
  bool handleKey(QKeyEvent *event);
  // The type-ahead text a key types: one printable character without Ctrl, Alt or Meta, else "".
  static QString printable(const QKeyEvent *event);

  // The pointer moved over row `index` of `section` (`pos` in `item`'s coordinates). Moves the
  // cursor only when the pointer really travelled (more than 1 point since the last accepted
  // sample, measured in referenceWidget()), so rows moving under a still pointer don't.
  bool pointerMoved(QWidget *item, const QPointF &pos, const QString &section, int index);
  // Something that doesn't scroll (the panel); default: the item itself.
  void setReferenceWidget(QWidget *w) { m_reference = w; }
  QWidget *referenceWidget() const { return m_reference; }
  // A view opened by the pointer keeps the row under it.
  void allowInitialSample();

  // Wire a row (or any Control) to row `index` of `section`: its hasCursor follows the model,
  // and a ListRow's pointer moves (or a TileGrid's, per tile) move the cursor.
  void attach(Control *item, const QString &section, int index);
  // Wire a TileGrid to `section`: its cursorIndex follows the model, the pointer moves it.
  void attach(TileGrid *grid, const QString &section = QString());

Q_SIGNALS:
  // Any change of section, index or visibility (repaint rows).
  void changed();
  // The cursor landed on a row; `byKey` when the keyboard moved it (reveal the row then).
  void moved(const QString &section, int index, bool byKey);
  void activated(const QString &section, int index);
  void deleteRequested(const QString &section, int index);
  void closeRequested();
  void textTyped(const QString &text);

private:
  int sectionIndex(const QString &name) const;
  int neighbour(int from, int dir) const;
  void settle();
  void step(int dx, int dy);
  bool wake();
  void announce(bool byKey);
  void resetGate();

  QList<Section> m_sections;
  QString m_section;
  int m_index = 0;
  bool m_active = false, m_pinned = false;
  int m_columns = 1;
  int m_pageSize = 5;
  QPointer<QWidget> m_reference;
  // PointerMoveGate
  bool m_primed = false, m_initialAllowed = false;
  QPointF m_last;
};

// --------------------------------------------------------------------------------------- ListRow

// One row of a keyboard-and-mouse list: a leading glyph (or widget), a bold title, a dim subtitle
// and a status line, a value (or trailing widget), and an end slot holding a trailing glyph at
// rest and the action in its place while the action shows. The look follows hasCursor and
// current only, never hover: hand pointerMoved to a CursorModel (CursorModel::attach does).
// Disabled (setEnabled(false)) draws everything faint and takes no clicks.
class OMARCHY_EXPORT ListRow : public Control {
  Q_OBJECT
  Q_PROPERTY(QString title READ title WRITE setTitle)
  Q_PROPERTY(QString subtitle READ subtitle WRITE setSubtitle)
  Q_PROPERTY(bool current READ isCurrent WRITE setCurrent)
  Q_PROPERTY(bool actionCursor READ actionCursor WRITE setActionCursor)

public:
  explicit ListRow(const QString &glyph = QString(), const QString &title = QString(), QWidget *parent = nullptr);

  QString glyph() const { return m_glyph; }
  void setGlyph(const QString &g);
  // 0 = font.title.
  void setGlyphSize(int px);
  // Invalid = the foreground.
  void setGlyphColor(const QColor &c);
  // Any leading widget (a small Avatar) instead of the glyph; the row owns it.
  void setLeading(QWidget *w);
  QWidget *leading() const { return m_leading; }

  QString title() const { return m_title; }
  void setTitle(const QString &t);
  QString subtitle() const { return m_subtitle; }
  void setSubtitle(const QString &t);
  QString status() const { return m_status; }
  void setStatus(const QString &s, const QString &tone = QString());
  void setStatusTone(const QString &tone);

  // Dim trailing text, or any trailing widget instead (a Badge, a non-interactive ToggleSwitch).
  void setValue(const QString &v);
  void setTrailing(QWidget *w);
  QWidget *trailing() const { return m_trailing; }

  // A glyph that stays in the end slot (a lock, a chevron, a check), dim unless toned.
  void setTrailingGlyph(const QString &g, const QString &tone = QString());
  // The action revealed in the end slot under the cursor or the pointer (forget).
  void setActionGlyph(const QString &g, bool urgent = false);
  void setActionUrgent(bool on);
  void setActionTip(const QString &tip);
  bool actionCursor() const { return m_actionCursor; }
  void setActionCursor(bool on);
  bool isActionShown() const;

  bool isCurrent() const { return m_current; }
  void setCurrent(bool on);

  // Opens the row: any widget across it under the main line (a password field and Connect).
  void setExpansion(QWidget *w);
  QWidget *expansion() const { return m_expansion; }

  // The end slot, in the row's coordinates (empty when the row has none).
  QRectF actionRect() const;
  // The exact (fractional) size Qt Quick gives the row at `width`.
  QSizeF implicitSize(qreal width = -1) const;
  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return m_expansion != nullptr; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void clicked();
  void actionClicked();
  // The pointer moved over the row (pos in the row's coordinates).
  void pointerMoved(const QPointF &pos);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  bool event(QEvent *e) override;
  void changeEvent(QEvent *e) override;
  void themeMetricsChanged() override { place(); }

private:
  struct Geometry;
  Geometry geometry(qreal width, qreal height = -1) const;
  void place();
  void adopt(QWidget *&slot, QWidget *w);
  QColor toneColor(const QString &tone) const;

  QString m_glyph, m_title, m_subtitle, m_status, m_statusTone, m_value;
  QString m_trailingGlyph, m_trailingTone, m_actionGlyph, m_actionTip;
  QColor m_glyphColor;
  int m_glyphSize = 0;
  bool m_actionUrgent = false, m_actionCursor = false, m_current = false;
  QPointer<QWidget> m_leading, m_trailing, m_expansion;
  QPointF m_pressAt;
};

// ------------------------------------------------------------------------------------ ScrollArea

// A vertical scroll view capped at a maximum height, its widgets stacked `spacing` apart as wide
// as the area (at Qt Quick's fractional positions), Omarchy's thin scrollbar over its right edge
// while the content overflows. Wheel and thumb drag scroll; it stops at the ends.
class OMARCHY_EXPORT ScrollArea : public QWidget {
  Q_OBJECT
public:
  explicit ScrollArea(QWidget *parent = nullptr);

  // 0 = uncapped.
  void setMaxHeight(int h);
  int maxHeight() const { return m_maxHeight; }
  void setSpacing(int px);
  void addWidget(QWidget *w);
  // The column the widgets sit in (their parent).
  QWidget *contentWidget() const { return m_content; }
  QList<QWidget *> widgets() const { return m_items; }

  qreal contentHeight(qreal width = -1) const;
  qreal scrollY() const { return m_scrollY; }
  void setScrollY(qreal y);
  qreal maxScrollY() const;
  bool isOverflowing() const;
  // Scroll just far enough to show a child (the cursor's row after a key), or a span of the content.
  void ensureVisible(QWidget *w);
  void ensureVisibleRange(qreal y, qreal h);
  // Where a child sits in the content (fractional).
  QRectF itemRect(QWidget *w) const;
  // The bar and its thumb, in the area's coordinates (empty when not overflowing).
  QRectF barRect() const;
  QRectF thumbRect() const;

  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void scrolled(qreal y);

protected:
  void resizeEvent(QResizeEvent *) override { place(); }
  void wheelEvent(QWheelEvent *e) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  class Bar;
  void place();
  qreal itemHeight(QWidget *w, qreal width) const;
  QWidget *m_viewport, *m_content;
  Bar *m_bar;
  QList<QWidget *> m_items;
  QList<QRectF> m_rects;
  int m_maxHeight = 0, m_spacing = 0;
  qreal m_scrollY = 0;
  bool m_placing = false;
};

// -------------------------------------------------------------------------------------- KeyValue

// Dim labels beside their values in a grid, `pairColumns` pairs to a line: a details block.
class OMARCHY_EXPORT KeyValue : public QWidget {
  Q_OBJECT
public:
  struct Pair {
    QString label, value, tone;
  };
  explicit KeyValue(QWidget *parent = nullptr);
  void setPairs(const QList<Pair> &pairs);
  QList<Pair> pairs() const { return m_pairs; }
  void setPairColumns(int n);
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  struct Cells;
  Cells cells() const;
  QList<Pair> m_pairs;
  int m_columns = 1;
};

// ------------------------------------------------------------------------------------------ Hero

// The head of a panel: a big glyph, a bold title, a dim uppercase status line (urgent on error)
// and a trailing control centered against them.
class OMARCHY_EXPORT Hero : public QWidget {
  Q_OBJECT
public:
  explicit Hero(const QString &glyph = QString(), const QString &title = QString(), QWidget *parent = nullptr);
  void setGlyph(const QString &g);
  void setTitle(const QString &t);
  void setStatus(const QString &s);
  void setError(bool on);
  void setGlyphOpacity(qreal o);
  // Invalid = the foreground.
  void setForeground(const QColor &c);
  // The row owns it.
  void setTrailing(QWidget *w);
  QWidget *trailing() const { return m_trailing; }
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  bool event(QEvent *e) override;

private:
  QFont statusFont() const;
  qreal labelsHeight() const;
  qreal trailingInset() const;
  void place();
  QString m_glyph, m_title, m_status;
  bool m_error = false;
  qreal m_glyphOpacity = 1;
  QColor m_fg;
  QPointer<QWidget> m_trailing;
};

// ----------------------------------------------------------------------------------------- Badge

// A count or a short label in a small pill: neutral glass, a tone at the selected fill alpha, or
// `filled` solid with the background as text. Never narrower than tall.
class OMARCHY_EXPORT Badge : public Control {
  Q_OBJECT
  Q_PROPERTY(QSizeF implicitSize READ implicitSize)
public:
  explicit Badge(const QString &text = QString(), const QString &tone = QString(), bool filled = false, QWidget *parent = nullptr);
  void setText(const QString &t);
  void setTone(const QString &tone);
  void setFilled(bool on);
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

  // A tone's color ("" = `neutral`, else the foreground).
  static QColor toneColor(const Tokens &tk, const QString &tone, const QColor &neutral = QColor());

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_text, m_tone;
  bool m_filled = false;
};

// ------------------------------------------------------------------------------------- StatusDot

// A dot ringed in the surface color it sits on. Always a circle.
class OMARCHY_EXPORT StatusDot : public Control {
  Q_OBJECT
  Q_PROPERTY(QSizeF implicitSize READ implicitSize)
public:
  explicit StatusDot(const QString &tone = QStringLiteral("success"), QWidget *parent = nullptr);
  void setTone(const QString &tone);
  // 0 = space(8); ring < 0 = max(1, space(2)).
  void setDotSize(qreal px);
  void setRing(qreal px);
  // Invalid = the popups background.
  void setGround(const QColor &c);
  qreal dotSize() const;
  qreal ring() const;
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QString m_tone;
  qreal m_size = 0, m_ring = -1;
  QColor m_ground;
};

// ---------------------------------------------------------------------------------------- Avatar

// A person's or an app's picture with fallbacks, tried in order: the image, an icon from the
// icon theme by name, a glyph, the first letter of the name. A source that fails gives way.
class OMARCHY_EXPORT Avatar : public Control {
  Q_OBJECT
  Q_PROPERTY(QSizeF implicitSize READ implicitSize)
public:
  enum Size { Small, Medium, Large };
  explicit Avatar(const QString &name = QString(), QWidget *parent = nullptr);
  ~Avatar() override;

  void setName(const QString &n);
  // A file path, file:// URL or qrc: / :/ resource.
  void setSource(const QString &source);
  // A freedesktop icon name (or an absolute path), Omarchy's icon lookup.
  void setIconName(const QString &name);
  void setGlyph(const QString &g);
  void setSize(Size s);
  // 0 = from size().
  void setPixelSize(qreal px);
  qreal pixelSize() const;
  void setRound(bool on);
  // Docks a StatusDot on the bottom-right corner ("" = none).
  void setStatus(const QString &tone);
  // What the dot's ring is drawn in; invalid = the popups background.
  void setGround(const QColor &c);

  // What it draws now: "picture", "icon" or "fallback".
  QString drawn() const;
  static QString initial(const QString &name);
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
  bool event(QEvent *e) override;
  void themeMetricsChanged() override { dock(); }

private:
  qreal radius() const;
  int devicePixels() const;
  const QImage &picture() const;
  const QImage &icon() const;
  void dock();

  QString m_name, m_source, m_iconName, m_glyph, m_status;
  Size m_size = Medium;
  qreal m_pixelSize = 0;
  bool m_round = false;
  QColor m_ground;
  QPointer<StatusDot> m_dot;
  mutable QImage m_picture, m_icon;
  mutable QString m_pictureKey, m_iconKey;
};

// -------------------------------------------------------------------------------------- TileGrid

// A grid of tiles, each a glyph or a medium Avatar above a label of up to two lines: an app grid,
// an emoji picker. The cursor tile takes the hover look, the current one the selected fill.
// Drive it with a CursorModel with columns set (CursorModel::attach wires it as one section).
class OMARCHY_EXPORT TileGrid : public QWidget {
  Q_OBJECT
public:
  struct Tile {
    QString label, glyph;
    // An Avatar tile: a picture by `source`, else the glyph or name's initial.
    bool avatar = false;
    QString name, source;
  };
  explicit TileGrid(QWidget *parent = nullptr);

  void setTiles(const QList<Tile> &tiles);
  QList<Tile> tiles() const { return m_tiles; }
  void setColumns(int n);
  int columns() const { return m_columns; }
  // 0 = space(88).
  void setCellWidth(qreal w);
  qreal cellWidth() const;
  qreal cellHeight() const;
  int cursorIndex() const { return m_cursor; }
  void setCursorIndex(int i);
  int currentIndex() const { return m_current; }
  void setCurrentIndex(int i);
  QRectF tileRect(int index) const;
  int tileAt(const QPointF &pos) const;
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

Q_SIGNALS:
  void activated(int index);
  // The pointer moved over a tile (pos in the grid's coordinates).
  void pointerMoved(int index, const QPointF &pos);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void mouseMoveEvent(QMouseEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;

private:
  qreal iconHeight() const;
  void rebuild();
  void place();
  QList<Tile> m_tiles;
  QList<Avatar *> m_avatars;
  int m_columns = 4, m_cursor = -1, m_current = -1, m_pressed = -1;
  qreal m_cellWidth = 0;
};

}  // namespace Omarchy
