#pragma once

// Composed (spec/components/composed.md): SearchList, the type-ahead overlay the shell builds
// seven times, on FilterInput (its search line), ListRow, CursorModel, ScrollArea and EmptyState.
//
//   auto *search = new Omarchy::SearchList;
//   search->setShowDetail(true);
//   connect(search, &SearchList::queryChanged, [=](const QString &q) {
//     QList<SearchItem> shown;                                   // the caller filters
//     for (const auto &i : all) if (SearchList::matches(i.title, q)) shown << i;
//     search->setItems(shown);
//   });
//   connect(search, &SearchList::activated, [=](int i) { open(search->items()[i]); });
//   connect(search, &SearchList::closeRequested, overlay, &QWidget::close);

#include <QFont>
#include <QList>
#include <QPointer>
#include <functional>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

namespace Omarchy {

class CursorModel;
class EmptyState;
class ListRow;
class ScrollArea;

// The search line of a keyboard-driven panel: the query in one line, no caret, growing only at
// its end (Left and Right belong to the panel). Takes input-method text (fcitx5 composing
// Japanese, Chinese or Korean). A key handler, when set, sees every key first; what it leaves
// unaccepted is typed here. Alt and Super chords are shortcuts, never text.
class OMARCHY_EXPORT FilterInput : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
  explicit FilterInput(QWidget *parent = nullptr);

  QString text() const { return m_text; }
  void setText(const QString &t);
  QString preeditText() const { return m_preedit; }
  // Drawn while the text and the preedit are empty (none by default).
  QString placeholderText() const { return m_placeholder; }
  void setPlaceholderText(const QString &t);
  // The font token (default "heading") and the text's color as a token path (default
  // "palette.foreground"), read at paint time. The placeholder is textFaint.
  void setFontToken(const char *token);
  void setColorPath(const QString &path);
  // Called with each key first; return true when it took the key.
  void setKeyHandler(std::function<bool(QKeyEvent *)> handler) { m_handler = std::move(handler); }

  // TextInput's implicit height: the line height, ceiled.
  QSize sizeHint() const override;

Q_SIGNALS:
  void textChanged(const QString &text);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void inputMethodEvent(QInputMethodEvent *e) override;
  QVariant inputMethodQuery(Qt::InputMethodQuery q) const override;
  bool event(QEvent *e) override;

private:
  QFont textFont() const;
  QString m_text, m_preedit, m_placeholder;
  QByteArray m_font = "heading";
  QString m_colorPath = QStringLiteral("palette.foreground");
  std::function<bool(QKeyEvent *)> m_handler;
};

// One result of a SearchList: a glyph, a bold title, a dim subtitle and value, and the detail
// the pane shows under the cursor.
struct OMARCHY_EXPORT SearchItem {
  QString title, subtitle, glyph, value, detail;
};

// A filter line, the matching rows under a keyboard cursor, an optional detail pane beside them
// (master-detail), and an empty state when nothing matches, on the [menu] surface. The list
// shows exactly the items it is given: filtering is the caller's (matches() is the plain case).
//
// Keys (the filter line holds focus; the list sees each key first): printable keys and Space
// type (so j, k and x are text); Backspace edits; Up, Down, Page Up/Down, Home and End move the
// cursor; Return/Enter activates; Delete asks to delete; Escape clears the query, or asks to
// close when it is empty. A new query puts the cursor back on the first match.
class OMARCHY_EXPORT SearchList : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
  Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
  Q_PROPERTY(bool showDetail READ showDetail WRITE setShowDetail)
  Q_PROPERTY(int cursorIndex READ cursorIndex NOTIFY cursorIndexChanged)

public:
  explicit SearchList(QWidget *parent = nullptr);

  QString query() const { return m_query; }
  void setQuery(const QString &q);
  // Default: common.search ("Search...") in the display language.
  QString placeholderText() const;
  void setPlaceholderText(const QString &t);
  QList<SearchItem> items() const { return m_items; }
  void setItems(const QList<SearchItem> &items);

  bool showDetail() const { return m_showDetail; }
  void setShowDetail(bool on);
  // A custom pane instead of the cursor item's title and detail (the list owns it).
  void setDetail(QWidget *w);
  QWidget *detail() const { return m_detail; }
  // The pane's width; < 0 = 45% of the body (floored).
  qreal detailWidth() const;
  void setDetailWidth(qreal w);

  // Shown when there are no items: "No matches for “query”", or emptyText with an empty query.
  void setEmptyGlyph(const QString &g);
  void setEmptyText(const QString &t);
  // %1 is the query; default common.noMatchesFor.
  void setNoMatchesText(const QString &t);
  QString emptyTitle() const;

  CursorModel *cursor() const { return m_cursor; }
  FilterInput *filterInput() const { return m_input; }
  // The cursor row, -1 when there are no items.
  int cursorIndex() const;
  // The cursor item; a default one (empty title) when there is none.
  SearchItem currentItem() const;

  // The rows (the first items().size() are shown), the scroll area and the empty state.
  ListRow *row(int index) const { return m_rows.value(index); }
  ScrollArea *scrollArea() const { return m_scroll; }
  EmptyState *emptyState() const { return m_empty; }
  // The card's parts, in the list's coordinates.
  QRectF headerRect() const;
  QRectF bodyRect() const;
  QRectF detailRect() const;

  // Case-insensitive substring: the clipboard's filter.
  static bool matches(const QString &text, const QString &query);

  // space(480) × space(360).
  QSize sizeHint() const override;

Q_SIGNALS:
  void queryChanged(const QString &query);
  void cursorIndexChanged(int index);
  void activated(int index);
  void deleteRequested(int index);
  void closeRequested();

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void focusInEvent(QFocusEvent *e) override;

private:
  bool handleKey(QKeyEvent *e);
  QMarginsF insets() const;
  int headerHeight() const;
  bool detailShown() const { return m_showDetail && !m_items.isEmpty(); }
  qreal listWidth() const;
  void sync();
  void place();

  QString m_query, m_placeholder, m_emptyGlyph, m_emptyText, m_noMatches;
  bool m_placeholderSet = false, m_noMatchesSet = false;
  QList<SearchItem> m_items;
  bool m_showDetail = false;
  qreal m_detailWidth = -1;
  QPointer<QWidget> m_detail;
  CursorModel *m_cursor;
  FilterInput *m_input;
  ScrollArea *m_scroll;
  QList<ListRow *> m_rows;
  EmptyState *m_empty;
  int m_lastCursor = -2;
};

}  // namespace Omarchy
