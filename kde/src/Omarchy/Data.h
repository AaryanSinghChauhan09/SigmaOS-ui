#pragma once

// Data and layout (spec/components/data.md): Table, Tree, SplitView, Sidebar and Calendar, drawn
// as the Qt Quick kit's Table.qml, Tree.qml, SplitView.qml, Sidebar.qml and Calendar.qml.
//
// Table, Tree, Sidebar and Calendar paint their rows themselves (one widget each, no delegates),
// so every table in every app reads the same and lands on the Qt Quick kit's pixels. They share
// one row ladder: the hover fill on the cursor row (under the pointer, with focus, or with
// hasCursor), else the selected fill on the selected row. The app owns selection, sort, expanded
// nodes and the shown month: the widgets ask through signals and move their cursor themselves.

#include <QAbstractItemModel>
#include <QDate>
#include <QPointer>
#include <QSplitter>
#include <QStringList>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"


namespace Omarchy {

class IconButton;

// ----------------------------------------------------------------------------------------- Table

// A header of dim bold captions (the sorted column carries a chevron) over rows of elided text
// with hairline rules. Rows are string lists, or a QAbstractItemModel's DisplayRole text (its
// horizontal header gives the titles columns leave empty). Up/Down/Home/End/PageUp/PageDown move
// the cursor, Return/Enter/Space choose it; a header click asks for a sort. Given a height, the
// rows scroll (wheel, keys) under the fixed header.
class OMARCHY_EXPORT Table : public Control {
  Q_OBJECT
  Q_PROPERTY(int sortColumn READ sortColumn WRITE setSortColumn)
  Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending)
  Q_PROPERTY(int selectedRow READ selectedRow WRITE setSelectedRow NOTIFY selectedRowChanged)
  Q_PROPERTY(int cursorRow READ cursorRow WRITE setCursorRow NOTIFY cursorRowChanged)
  Q_PROPERTY(bool separators READ separators WRITE setSeparators)

public:
  struct Column {
    QString title;
    int width = 0;  // 0: share the width the fixed columns leave
    bool alignRight = false;
  };

  explicit Table(QWidget *parent = nullptr);

  void setColumns(const QList<Column> &columns);
  QList<Column> columns() const;
  void setRows(const QList<QStringList> &rows);
  // The rows shown (the model's, when one is set).
  QList<QStringList> rows() const;
  int rowCount() const;
  // Rows (and empty titles) from a model, kept in step with it. nullptr goes back to setRows'.
  void setModel(QAbstractItemModel *model);
  QAbstractItemModel *model() const { return m_model; }

  int sortColumn() const { return m_sortColumn; }
  void setSortColumn(int column);
  bool sortAscending() const { return m_sortAscending; }
  void setSortAscending(bool on);
  int selectedRow() const { return m_selected; }
  void setSelectedRow(int row);
  int cursorRow() const { return m_cursor; }
  void setCursorRow(int row);
  bool separators() const { return m_separators; }
  void setSeparators(bool on);

  int rowHeight() const;
  QList<int> columnWidths() const;
  // Where a header cell and a body row are (scrolled), in widget coordinates.
  QRect headerRect(int column) const;
  QRect rowRect(int row) const;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return QSize(0, rowHeight()); }

Q_SIGNALS:
  void rowActivated(int row);
  void sortRequested(int column);
  void selectedRowChanged(int row);
  void cursorRowChanged(int row);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;

private:
  void activateRow(int row);
  void moveCursor(int delta);
  void ensureVisible(int row);
  void setScroll(qreal y);
  qreal bodyHeight() const;
  int rowAt(const QPointF &pos) const;
  int headerAt(const QPointF &pos) const;
  void hover(const QPointF &pos);
  void reload();

  QList<Column> m_columns;
  QList<QStringList> m_rows, m_modelRows;
  QStringList m_modelTitles;
  QPointer<QAbstractItemModel> m_model;
  QList<QMetaObject::Connection> m_modelConnections;
  int m_sortColumn = -1, m_selected = -1, m_cursor = -1;
  bool m_sortAscending = true, m_separators = true, m_bodyHover = false;
  int m_pressRow = -1, m_pressHeader = -1, m_hoverRow = -1;
  qreal m_scroll = 0;
};

// ------------------------------------------------------------------------------------------ Tree

struct OMARCHY_EXPORT TreeNode {
  QString id, label, glyph;
  QList<TreeNode> children;
  TreeNode() = default;
  TreeNode(const QString &id, const QString &label, const QString &glyph = QString(), const QList<TreeNode> &children = {})
      : id(id), label(label), glyph(glyph), children(children) {}
};

// FileTree-style rows: indent per level, a chevron on branches, an optional glyph, the label.
// The app owns `expanded` and `selectedId`; the tree asks with toggled(id) (chevron click, double
// click, Right on a closed branch, Left on an open one) and activated(id) (click, Return/Space).
// Up/Down move the cursor.
class OMARCHY_EXPORT Tree : public Control {
  Q_OBJECT
  Q_PROPERTY(QStringList expanded READ expanded WRITE setExpanded)
  Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId)
  Q_PROPERTY(QString cursorId READ cursorId WRITE setCursorId NOTIFY cursorIdChanged)

public:
  struct Row {
    QString id, label, glyph;
    int depth = 0;
    bool branch = false, open = false;
  };

  explicit Tree(QWidget *parent = nullptr);

  void setNodes(const QList<TreeNode> &nodes);
  QList<TreeNode> nodes() const { return m_nodes; }
  QStringList expanded() const { return m_expanded; }
  void setExpanded(const QStringList &ids);
  QString selectedId() const { return m_selected; }
  void setSelectedId(const QString &id);
  QString cursorId() const { return m_cursor; }
  void setCursorId(const QString &id);

  // The rows shown, depth first (children only under expanded branches).
  const QList<Row> &visibleRows() const { return m_rows; }
  int rowHeight() const;
  // Where a row and its chevron slot are (scrolled), in widget coordinates; empty when hidden.
  QRect rowRect(const QString &id) const;
  QRect chevronRect(const QString &id) const;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return QSize(0, 0); }

Q_SIGNALS:
  void toggled(const QString &id);
  void activated(const QString &id);
  void cursorIdChanged(const QString &id);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;

private:
  void rebuild();
  int indexOf(const QString &id) const;
  int rowAt(const QPointF &pos) const;
  bool onChevron(int row, const QPointF &pos) const;
  void moveCursor(int delta);
  void ensureVisible(int row);
  void setScroll(qreal y);
  void hover(const QPointF &pos);

  QList<TreeNode> m_nodes;
  QStringList m_expanded;
  QList<Row> m_rows;
  QString m_selected, m_cursor;
  bool m_bodyHover = false, m_swallowRelease = false;
  int m_pressRow = -1, m_hoverRow = -1;
  bool m_pressChevron = false;
  qreal m_scroll = 0;
};

// ------------------------------------------------------------------------------------- SplitView

// Panes with draggable handles (a QSplitter): each handle a `md` strip holding a hairline in
// `track`, lit with the hover fill under the pointer, while dragged or with hasCursor. Add panes
// with their preferred, minimum and maximum size; the pane added with fill takes what is left.
// Hiding a pane takes its handle with it. saveState()/restoreState() are QSplitter's.
class OMARCHY_EXPORT SplitView : public QSplitter {
  Q_OBJECT
  Q_PROPERTY(bool hasCursor READ hasCursor WRITE setHasCursor)

public:
  explicit SplitView(Qt::Orientation orientation = Qt::Horizontal, QWidget *parent = nullptr);

  // A pane: `preferred` its starting size along the split (-1: its size hint), `minimum` and
  // `maximum` the clamp a drag respects (-1: none); `fill` takes the room the others leave.
  void addPane(QWidget *pane, int preferred = -1, int minimum = -1, int maximum = -1, bool fill = false);

  bool hasCursor() const { return m_hasCursor; }
  void setHasCursor(bool on);
  int handleThickness() const;

protected:
  QSplitterHandle *createHandle() override;
  void resizeEvent(QResizeEvent *e) override;

private:
  void distribute();
  struct Pane {
    QPointer<QWidget> widget;
    int preferred = -1;
    bool fill = false;
  };
  QList<Pane> m_panes;
  bool m_hasCursor = false, m_distributed = false;
};

// --------------------------------------------------------------------------------------- Sidebar

struct OMARCHY_EXPORT SidebarItem {
  QString id, label, glyph;
};
struct OMARCHY_EXPORT SidebarSection {
  QString title;
  QList<SidebarItem> items;
};

// An app's navigation: sections, each a section header over glyph + label rows, one selected.
// Collapsed, it is a rail of glyphs railWidth() wide. Up/Down walk every row across sections,
// Return/Enter/Space choose; the app owns selectedId and hears activated(id).
class OMARCHY_EXPORT Sidebar : public Control {
  Q_OBJECT
  Q_PROPERTY(QString selectedId READ selectedId WRITE setSelectedId)
  Q_PROPERTY(QString cursorId READ cursorId WRITE setCursorId NOTIFY cursorIdChanged)
  Q_PROPERTY(bool collapsed READ isCollapsed WRITE setCollapsed)

public:
  explicit Sidebar(QWidget *parent = nullptr);

  void setSections(const QList<SidebarSection> &sections);
  QList<SidebarSection> sections() const { return m_sections; }
  QString selectedId() const { return m_selected; }
  void setSelectedId(const QString &id);
  QString cursorId() const { return m_cursor; }
  void setCursorId(const QString &id);
  bool isCollapsed() const { return m_collapsed; }
  void setCollapsed(bool on);

  int railWidth() const;
  QRectF rowRect(const QString &id) const;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return QSize(0, 0); }

Q_SIGNALS:
  void activated(const QString &id);
  void cursorIdChanged(const QString &id);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;

private:
  struct Placed {
    QString id, label, glyph;
    QRectF rect;
  };
  struct Header {
    QString title;
    QRectF rect;
  };
  void place(QList<Placed> *rows, QList<Header> *headers, qreal *height) const;
  QString rowAt(const QPointF &pos) const;
  QStringList ids() const;
  void moveCursor(int delta);
  void hover(const QPointF &pos);

  QList<SidebarSection> m_sections;
  QString m_selected, m_cursor, m_pressId, m_hoverId;
  bool m_collapsed = false, m_bodyHover = false;
};

// -------------------------------------------------------------------------------------- Calendar

// A month grid from Monday: the month and year between previous/next IconButtons, a weekday row,
// then six weeks (always six). Today is ringed in the accent, the selected day takes the selected
// fill, days outside the month are faint and weekends dim. The app passes `today` (nothing reads
// the clock). The buttons and PageUp/PageDown step the month; arrows move the cursor by a day or
// a week, following it into the next month; Return/Enter/Space or a click pick the day (selected
// takes it and picked() fires).
class OMARCHY_EXPORT Calendar : public Control {
  Q_OBJECT
  Q_PROPERTY(int year READ year WRITE setYear NOTIFY monthChanged)
  Q_PROPERTY(int month READ month WRITE setMonth NOTIFY monthChanged)
  Q_PROPERTY(QDate today READ today WRITE setToday)
  Q_PROPERTY(QDate selected READ selected WRITE setSelected)
  Q_PROPERTY(QDate cursorDate READ cursorDate WRITE setCursorDate NOTIFY cursorDateChanged)

public:
  explicit Calendar(QWidget *parent = nullptr);

  int year() const { return m_year; }
  int month() const { return m_month; }
  void setYear(int year);
  void setMonth(int month);  // 1-12
  void setMonth(int year, int month);
  void stepMonth(int delta);
  QDate today() const { return m_today; }
  void setToday(const QDate &d);
  QDate selected() const { return m_selected; }
  void setSelected(const QDate &d);
  QDate cursorDate() const { return m_cursor; }
  void setCursorDate(const QDate &d);
  // English by default (the kit's catalog has none); twelve and seven names, Monday first.
  QStringList monthNames() const { return m_monthNames; }
  void setMonthNames(const QStringList &names);
  QStringList weekdayNames() const { return m_weekdayNames; }
  void setWeekdayNames(const QStringList &names);

  // The 42 days shown, from the Monday on or before the 1st.
  QDate firstShown() const;
  QRect dayRect(const QDate &d) const;
  IconButton *previousButton() const { return m_prev; }
  IconButton *nextButton() const { return m_next; }

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return sizeHint(); }

Q_SIGNALS:
  void picked(const QDate &date);
  void monthChanged(int year, int month);
  void cursorDateChanged(const QDate &date);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void resizeEvent(QResizeEvent *e) override;
  void themeMetricsChanged() override;

private:
  int cellWidth() const;
  int cellHeight() const;
  int cellSpacing() const;
  int gridWidth() const;
  int headerHeight() const;
  int gridTop() const;
  QDate dayAt(const QPointF &pos) const;
  void moveCursor(int days);
  void pick(const QDate &d);
  void hover(const QPointF &pos);
  void placeButtons();

  int m_year = 2026, m_month = 1;
  QDate m_today, m_selected, m_cursor, m_pressDay, m_hoverDay;
  QStringList m_monthNames, m_weekdayNames;
  IconButton *m_prev = nullptr, *m_next = nullptr;
  bool m_bodyHover = false;
};

}  // namespace Omarchy
