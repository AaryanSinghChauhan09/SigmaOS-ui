// Composed (spec/components/composed.md): SearchList's keys, mirroring qt/tests/tst_searchlist.qml
// case for case: typing filters (j, k, x and Space are text), Up/Down and the paging keys move the
// cursor, Return activates the cursor row, Delete asks to delete it, Escape clears the query and
// then asks to close, nothing matching shows the empty state, a click activates the row.

#include <QApplication>
#include <QSignalSpy>
#include <QTest>

#include <Omarchy/Widgets.h>

#include "harness.h"

using namespace Omarchy;

namespace {

// The composed section's four results; the caller filters, as SearchList asks.
const QList<SearchItem> &all() {
  static const QList<SearchItem> items = {
      {QStringLiteral("Terminal"), QStringLiteral("App"), QString(), QString(), QStringLiteral("Open a new terminal window.")},
      {QStringLiteral("Terminal font"), QStringLiteral("Style"), QString(), QString(), QStringLiteral("Pick the monospace font every terminal uses.")},
      {QStringLiteral("Terminal theme"), QStringLiteral("Style"), QString(), QString(),
       QStringLiteral("Match the terminal colors to the desktop theme.")},
      {QStringLiteral("Intermediate CA"), QStringLiteral("Certificate"), QString(), QString(),
       QStringLiteral("Trust a certificate authority for this machine.")},
  };
  return items;
}

void refilter(SearchList *list) {
  QList<SearchItem> shown;
  for (const SearchItem &i : all())
    if (SearchList::matches(i.title, list->query())) shown << i;
  list->setItems(shown);
}

QStringList titles(const SearchList *list) {
  QStringList t;
  for (const SearchItem &i : list->items()) t << i.title;
  return t;
}

}  // namespace

class Composed : public QObject {
  Q_OBJECT

  QScopedPointer<SearchList> list;
  QScopedPointer<QSignalSpy> activated, deleted, closed;

  SearchList *make(const QString &query = QString()) {
    list.reset(new SearchList);
    list->setShowDetail(true);
    connect(list.data(), &SearchList::queryChanged, list.data(), [l = list.data()] { refilter(l); });
    list->setQuery(query);
    refilter(list.data());
    activated.reset(new QSignalSpy(list.data(), &SearchList::activated));
    deleted.reset(new QSignalSpy(list.data(), &SearchList::deleteRequested));
    closed.reset(new QSignalSpy(list.data(), &SearchList::closeRequested));
    Harness::show(list.data(), QSize(360, 300));
    list->activateWindow();
    list->setFocus();
    QTest::qWait(10);
    return list.data();
  }

  // Keys go where the application sends them: the focused widget.
  static QWidget *target() { return QApplication::focusWidget(); }
  static void key(Qt::Key k, Qt::KeyboardModifiers m = Qt::NoModifier) { QTest::keyClick(target(), k, m); }
  static void type(const QString &text) { QTest::keyClicks(target(), text); }

private Q_SLOTS:
  void filter_line_holds_focus() {
    SearchList *l = make();
    QCOMPARE(target(), static_cast<QWidget *>(l->filterInput()));
  }

  void typing_filters() {
    SearchList *l = make();
    QCOMPARE(l->items().size(), 4);
    type(QStringLiteral("font"));
    QCOMPARE(l->query(), QStringLiteral("font"));
    QCOMPARE(titles(l), QStringList{QStringLiteral("Terminal font")});
    key(Qt::Key_Backspace);
    QCOMPARE(l->query(), QStringLiteral("fon"));
    QCOMPARE(l->filterInput()->text(), QStringLiteral("fon"));
  }

  void vim_keys_and_space_are_text() {
    SearchList *l = make();
    type(QStringLiteral("jk"));
    QVERIFY2(l->query() == QStringLiteral("jk"), "j and k type, they don't move");
    QVERIFY2(l->cursorIndex() == -1, "nothing matches jk");
    key(Qt::Key_Backspace);
    key(Qt::Key_Backspace);
    type(QStringLiteral("term"));
    key(Qt::Key_Space);
    QCOMPARE(l->query(), QStringLiteral("term "));
    QCOMPARE(l->items().size(), 0);
    key(Qt::Key_Backspace);
    type(QStringLiteral("x"));
    QVERIFY2(l->query() == QStringLiteral("termx"), "x is text here, not delete");
    QCOMPARE(deleted->count(), 0);
  }

  void arrows_move_the_cursor() {
    SearchList *l = make(QStringLiteral("term"));
    QVERIFY2(l->cursorIndex() == 0, "the cursor shows on the first match from the start");
    key(Qt::Key_Down);
    QCOMPARE(l->cursorIndex(), 1);
    key(Qt::Key_Down);
    key(Qt::Key_Down);
    QCOMPARE(l->cursorIndex(), 3);
    key(Qt::Key_Down);
    QVERIFY2(l->cursorIndex() == 3, "it stops at the last row");
    key(Qt::Key_Up);
    QCOMPARE(l->cursorIndex(), 2);
    key(Qt::Key_Home);
    QCOMPARE(l->cursorIndex(), 0);
    key(Qt::Key_End);
    QCOMPARE(l->cursorIndex(), 3);
    key(Qt::Key_PageUp);
    QCOMPARE(l->cursorIndex(), 0);
    key(Qt::Key_PageDown);
    QCOMPARE(l->cursorIndex(), 3);
    QVERIFY2(l->currentItem().title == QStringLiteral("Intermediate CA"), "the detail pane follows the cursor");
    QVERIFY(l->row(3)->hasCursor());
    QVERIFY(!l->row(0)->hasCursor());
  }

  void enter_activates_the_cursor_row() {
    SearchList *l = make(QStringLiteral("term"));
    key(Qt::Key_Down);
    key(Qt::Key_Down);
    key(Qt::Key_Return);
    QCOMPARE(activated->count(), 1);
    QCOMPARE(activated->at(0).at(0).toInt(), 2);
    QCOMPARE(l->items()[2].title, QStringLiteral("Terminal theme"));
    key(Qt::Key_Enter, Qt::KeypadModifier);
    QVERIFY2(activated->count() == 2, "keypad Enter too");
  }

  void new_query_starts_at_first_match() {
    SearchList *l = make(QStringLiteral("term"));
    key(Qt::Key_Down);
    key(Qt::Key_Down);
    QCOMPARE(l->cursorIndex(), 2);
    type(QStringLiteral("i"));
    QCOMPARE(l->query(), QStringLiteral("termi"));
    QCOMPARE(l->cursorIndex(), 0);
  }

  void delete_asks_to_delete_the_cursor_row() {
    make(QStringLiteral("term"));
    key(Qt::Key_Down);
    key(Qt::Key_Delete);
    QCOMPARE(deleted->count(), 1);
    QCOMPARE(deleted->at(0).at(0).toInt(), 1);
  }

  void escape_clears_then_closes() {
    SearchList *l = make(QStringLiteral("termix"));
    QCOMPARE(l->items().size(), 0);
    key(Qt::Key_Escape);
    QCOMPARE(l->query(), QString());
    QVERIFY2(l->items().size() == 4, "an empty query lists everything");
    QCOMPARE(closed->count(), 0);
    key(Qt::Key_Escape);
    QCOMPARE(closed->count(), 1);
  }

  void nothing_matches_shows_the_empty_state() {
    SearchList *l = make(QStringLiteral("zzz"));
    QCOMPARE(l->items().size(), 0);
    QCOMPARE(l->cursorIndex(), -1);
    key(Qt::Key_Return);
    QVERIFY2(activated->count() == 0, "Return with no rows does nothing");
    const QString expected = Theme::instance()->string(QStringLiteral("common.noMatchesFor"), QString()).replace(QStringLiteral("%1"), QStringLiteral("zzz"));
    QVERIFY2(expected.contains(QStringLiteral("zzz")), "the catalog has a no-matches line with the query");
    QVERIFY(l->emptyState()->isVisible());
    QCOMPARE(l->emptyState()->title(), expected);
    QVERIFY(!l->scrollArea()->isVisible());
    // An empty query with nothing shows the caller's empty text instead.
    l->setEmptyText(QStringLiteral("Clipboard is empty"));
    l->setQuery(QString());
    l->setItems({});
    QCOMPARE(l->emptyState()->title(), QStringLiteral("Clipboard is empty"));
  }

  void click_activates_the_row() {
    SearchList *l = make(QStringLiteral("term"));
    ListRow *row = l->row(1);
    QVERIFY(row && row->isVisible());
    QCOMPARE(row->title(), QStringLiteral("Terminal font"));
    QTest::mouseClick(row, Qt::LeftButton, Qt::NoModifier, row->rect().center());
    QCOMPARE(activated->count(), 1);
    QCOMPARE(activated->at(0).at(0).toInt(), 1);
    QCOMPARE(l->cursorIndex(), 1);
  }

  void matches_is_a_case_insensitive_substring() {
    QVERIFY(SearchList::matches(QStringLiteral("Terminal Font"), QStringLiteral("al f")));
    QVERIFY(SearchList::matches(QStringLiteral("anything"), QString()));
    QVERIFY(!SearchList::matches(QStringLiteral("Terminal"), QStringLiteral("zzz")));
  }

  void layout_follows_the_qt_quick_metrics() {
    SearchList *l = make(QStringLiteral("term"));
    // tokyo-night: a 2px menu border, panelPadding 18, a 34px header, md 6 below it.
    QCOMPARE(l->headerRect(), QRectF(20, 20, 320, 34));
    QCOMPARE(l->filterInput()->geometry(), QRect(20, 26, 320, 22));
    QCOMPARE(l->bodyRect().y(), 60.0);
    QCOMPARE(l->detailRect().width(), std::floor(320 * 0.45));
    QCOMPARE(SearchList().sizeHint(), QSize(480, 360));
  }

  void cleanup() {
    activated.reset();
    deleted.reset();
    closed.reset();
    list.reset();
  }
};

OMARCHY_TEST(Composed)
#include "tst_composed.moc"
