// Mirrors qt/tests/tst_cursormodel.qml (itself case for case with the Rune CursorModel's tests):
// the first key only shows the cursor, rows cross sections and skip empty ones, pages stay in
// their section, grids move by columns, the action keys and type-ahead report themselves. Plus
// what clicks and the pointer do to ListRow and TileGrid, and the ScrollArea's reveal.

#include <QKeyEvent>
#include <QSignalSpy>
#include <QTest>

#include <Omarchy/Widgets.h>

#include "harness.h"

using namespace Omarchy;

namespace {

// A focused widget that hands every key to its model first, as a panel's Keys.onPressed does
// (Tab included, before QWidget turns it into a focus move).
class Panel : public QWidget {
public:
  CursorModel model;
  Panel() {
    setFocusPolicy(Qt::StrongFocus);
    model.setSections({{QStringLiteral("header"), 1}, {QStringLiteral("connected"), 2}, {QStringLiteral("known"), 0}, {QStringLiteral("discovered"), 3}});
  }

protected:
  bool event(QEvent *e) override {
    if (e->type() == QEvent::KeyPress && model.handleKey(static_cast<QKeyEvent *>(e))) return true;
    return QWidget::event(e);
  }
};

using At = QPair<QString, int>;
At at(const CursorModel &c) { return {c.focusSection(), c.selectedIndex()}; }

}  // namespace

class Lists : public QObject {
  Q_OBJECT

  Panel *make() {
    auto *p = new Panel;
    p->resize(100, 100);
    p->show();
    QTest::qWaitForWindowExposed(p);
    p->activateWindow();
    p->setFocus();
    return p;
  }

private Q_SLOTS:
  void first_key_only_shows_the_cursor() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    QVERIFY(!c.isAt(QStringLiteral("header"), 0));
    QTest::keyClick(p.data(), Qt::Key_J, Qt::NoModifier);
    QVERIFY(c.isAt(QStringLiteral("header"), 0));
    QTest::keyClick(p.data(), Qt::Key_J);
    QVERIFY(c.isAt(QStringLiteral("connected"), 0));
  }

  void rows_cross_sections_and_skip_empty_ones() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    c.set(QStringLiteral("connected"), 1);
    QTest::keyClick(p.data(), Qt::Key_Down);
    QVERIFY2(c.isAt(QStringLiteral("discovered"), 0), "skips the empty known section");
    QTest::keyClick(p.data(), Qt::Key_K);
    QVERIFY2(c.isAt(QStringLiteral("connected"), 1), "back to the previous section's last row");
    QTest::keyClick(p.data(), Qt::Key_End);
    QVERIFY(c.isAt(QStringLiteral("discovered"), 2));
    QTest::keyClick(p.data(), Qt::Key_Down);
    QVERIFY2(c.isAt(QStringLiteral("discovered"), 2), "stops at the last row");
    QTest::keyClick(p.data(), Qt::Key_Home);
    QVERIFY(c.isAt(QStringLiteral("header"), 0));
    QTest::keyClick(p.data(), Qt::Key_Up);
    QVERIFY2(c.isAt(QStringLiteral("header"), 0), "stops at the first row");
  }

  void tab_jumps_sections() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    c.set(QStringLiteral("header"), 0);
    QTest::keyClick(p.data(), Qt::Key_Tab);
    QVERIFY(c.isAt(QStringLiteral("connected"), 0));
    QTest::keyClick(p.data(), Qt::Key_Tab);
    QVERIFY2(c.isAt(QStringLiteral("discovered"), 0), "skips the empty section");
    QTest::keyClick(p.data(), Qt::Key_Backtab);
    QVERIFY(c.isAt(QStringLiteral("connected"), 0));
    QTest::keyClick(p.data(), Qt::Key_Tab, Qt::ShiftModifier);
    QVERIFY(c.isAt(QStringLiteral("header"), 0));
  }

  void pages_stay_in_their_section() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    c.setSections({{QStringLiteral("a"), 12}, {QStringLiteral("b"), 4}});
    c.set(QStringLiteral("a"), 0);
    QTest::keyClick(p.data(), Qt::Key_PageDown);
    QCOMPARE(at(c), At(QStringLiteral("a"), 5));
    QTest::keyClick(p.data(), Qt::Key_PageDown);
    QTest::keyClick(p.data(), Qt::Key_PageDown);
    QCOMPARE(at(c), At(QStringLiteral("a"), 11));
    QTest::keyClick(p.data(), Qt::Key_PageUp);
    QCOMPARE(c.selectedIndex(), 6);
  }

  void grids_move_by_columns() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    c.setList(8);
    c.setColumns(4);
    c.set(QString(), 1);
    QTest::keyClick(p.data(), Qt::Key_L);
    QCOMPARE(c.selectedIndex(), 2);
    QTest::keyClick(p.data(), Qt::Key_Down);
    QCOMPARE(c.selectedIndex(), 6);
    QTest::keyClick(p.data(), Qt::Key_Right);
    QTest::keyClick(p.data(), Qt::Key_Right);
    QCOMPARE(c.selectedIndex(), 7);  // stays on its line
    QTest::keyClick(p.data(), Qt::Key_K);
    QCOMPARE(c.selectedIndex(), 3);
    QTest::keyClick(p.data(), Qt::Key_H);
    QCOMPARE(c.selectedIndex(), 2);

    QScopedPointer<Panel> l(make());
    l->model.setList(3);
    l->model.set(QString(), 1);
    QTest::keyClick(l.data(), Qt::Key_Right);
    QCOMPARE(l->model.selectedIndex(), 1);  // a list ignores column moves
  }

  void short_last_line() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    c.setList(6);
    c.setColumns(4);
    c.set(QString(), 3);
    QTest::keyClick(p.data(), Qt::Key_Down);
    QCOMPARE(c.selectedIndex(), 5);  // lands on the short line's last tile
  }

  void actions_and_type_ahead() {
    QScopedPointer<Panel> p(make());
    CursorModel &c = p->model;
    c.setList(3);
    c.set(QString(), 1);
    QSignalSpy activated(&c, &CursorModel::activated);
    QSignalSpy deleted(&c, &CursorModel::deleteRequested);
    QSignalSpy closed(&c, &CursorModel::closeRequested);
    QSignalSpy typed(&c, &CursorModel::textTyped);
    QTest::keyClick(p.data(), Qt::Key_Return);
    QTest::keyClick(p.data(), Qt::Key_Space);
    QCOMPARE(activated.count(), 2);
    QCOMPARE(activated.at(0).at(1).toInt(), 1);
    QTest::keyClick(p.data(), Qt::Key_Escape);
    QCOMPARE(closed.count(), 1);
    QTest::keyClick(p.data(), Qt::Key_X);
    QTest::keyClick(p.data(), Qt::Key_Delete);
    QCOMPARE(deleted.count(), 2);
    QTest::keyClick(p.data(), Qt::Key_Q);
    QTest::keyClick(p.data(), Qt::Key_7);
    QCOMPARE(typed.count(), 2);
    QCOMPARE(typed.at(0).at(0).toString(), QStringLiteral("q"));
    QCOMPARE(typed.at(1).at(0).toString(), QStringLiteral("7"));
    QTest::keyClick(p.data(), Qt::Key_Q, Qt::ControlModifier);
    QCOMPARE(typed.count(), 2);  // Ctrl+Q is not text
    // Non-ASCII text types too (QtTest's QML keyClick can't send it).
    QKeyEvent e(QEvent::KeyPress, 0, Qt::NoModifier, QStringLiteral("é"));
    QVERIFY(c.handleKey(&e));
    QCOMPARE(typed.count(), 3);
    QCOMPARE(typed.at(2).at(0).toString(), QStringLiteral("é"));
    QCOMPARE(CursorModel::printable(&e), QStringLiteral("é"));
  }

  void moves_announce_by_key() {
    CursorModel c;
    c.setList(4);
    QSignalSpy moved(&c, &CursorModel::moved);
    QSignalSpy changed(&c, &CursorModel::changed);
    c.move(0, 1);  // only shows it
    QVERIFY(c.isCursorActive());
    QCOMPARE(c.selectedIndex(), 0);
    c.move(0, 1);
    QCOMPARE(c.selectedIndex(), 1);
    QCOMPARE(moved.count(), 2);
    QCOMPARE(moved.last().at(2).toBool(), true);
    QVERIFY(changed.count() >= 2);
    c.reset();
    QVERIFY(!c.isAt(QString(), 1));
  }

  void list_row_clicks_and_its_action() {
    ListRow row(glyph(0xF0922), QStringLiteral("Office"));
    row.setSubtitle(QStringLiteral("Saved"));
    row.setTrailingGlyph(glyph(0xF033E));
    row.setActionGlyph(glyph(0xF0159), true);
    Harness::show(&row, QSize(320, row.sizeHint().height()));
    QSignalSpy clicked(&row, &ListRow::clicked);
    QSignalSpy action(&row, &ListRow::actionClicked);
    QEvent leave(QEvent::Leave);  // wherever an earlier test left the pointer
    QApplication::sendEvent(&row, &leave);
    QVERIFY(!row.isActionShown());
    row.setHasCursor(true);
    QVERIFY2(row.isActionShown(), "the cursor reveals the action");
    QTest::mouseClick(&row, Qt::LeftButton, {}, QPoint(40, row.height() / 2));
    QCOMPARE(clicked.count(), 1);
    QCOMPARE(action.count(), 0);
    const QRectF slot = row.actionRect();
    QVERIFY(!slot.isEmpty());
    QVERIFY2(slot.right() <= row.width() && slot.right() > row.width() - 20, "the end slot ends the row");
    QTest::mouseClick(&row, Qt::LeftButton, {}, slot.center().toPoint());
    QCOMPARE(action.count(), 1);
    QCOMPARE(clicked.count(), 1);

    row.setEnabled(false);
    QVERIFY2(!row.isActionShown(), "a disabled row never shows its action");
    QTest::mouseClick(&row, Qt::LeftButton, {}, QPoint(40, row.height() / 2));
    QCOMPARE(clicked.count(), 1);
  }

  void a_bare_trailing_glyph_clicks_the_row() {
    ListRow row(QString(), QStringLiteral("Appearance"));
    row.setTrailingGlyph(glyph(0xF0142));
    Harness::show(&row, QSize(320, row.sizeHint().height()));
    QSignalSpy clicked(&row, &ListRow::clicked);
    QSignalSpy action(&row, &ListRow::actionClicked);
    QTest::mouseClick(&row, Qt::LeftButton, {}, row.actionRect().center().toPoint());
    QCOMPARE(clicked.count(), 1);
    QCOMPARE(action.count(), 0);
  }

  void the_pointer_moves_the_cursor_through_the_gate() {
    QWidget panel;
    auto *col = new Column(&panel);
    CursorModel cursor;
    cursor.setSections({{QStringLiteral("rows"), 2}});
    cursor.setReferenceWidget(&panel);
    auto *a = new ListRow(QString(), QStringLiteral("A"));
    auto *b = new ListRow(QString(), QStringLiteral("B"));
    col->addWidget(a);
    col->addWidget(b);
    cursor.attach(a, QStringLiteral("rows"), 0);
    cursor.attach(b, QStringLiteral("rows"), 1);
    cursor.set(QStringLiteral("rows"), 0);
    Harness::show(&panel, QSize(240, 120));
    QVERIFY(a->hasCursor());
    // The first sample only primes the gate.
    QTest::mouseMove(b, QPoint(20, 10));
    QVERIFY(a->hasCursor());
    QTest::mouseMove(b, QPoint(30, 12));
    QVERIFY2(b->hasCursor() && !a->hasCursor(), "a real move takes the cursor");
    // A key resets the gate: the next sample primes again.
    cursor.move(0, -1);
    QVERIFY(a->hasCursor());
    QTest::mouseMove(b, QPoint(31, 12));
    QVERIFY(a->hasCursor());
  }

  void tile_grid_clicks_and_follows_the_cursor() {
    TileGrid grid;
    grid.setColumns(4);
    QList<TileGrid::Tile> tiles;
    for (int i = 0; i < 6; ++i) tiles << TileGrid::Tile{QStringLiteral("Tile %1").arg(i), glyph(0xF120)};
    grid.setTiles(tiles);
    CursorModel cursor;
    cursor.setList(6);
    cursor.setColumns(4);
    cursor.attach(&grid);
    Harness::show(&grid, grid.sizeHint());
    QCOMPARE(grid.cursorIndex(), -1);
    cursor.set(QString(), 2);
    QCOMPARE(grid.cursorIndex(), 2);
    cursor.move(0, 1);
    QCOMPARE(grid.cursorIndex(), 5);  // the short line's last tile
    QSignalSpy activated(&grid, &TileGrid::activated);
    QTest::mouseClick(&grid, Qt::LeftButton, {}, grid.tileRect(4).center().toPoint());
    QCOMPARE(activated.count(), 1);
    QCOMPARE(activated.at(0).at(0).toInt(), 4);
    QCOMPARE(grid.sizeHint().width(), 4 * shown().space(88));
  }

  void scroll_area_caps_and_reveals() {
    ScrollArea area;
    area.setMaxHeight(100);
    area.setSpacing(2);
    QList<ListRow *> rows;
    for (int i = 0; i < 6; ++i) {
      auto *r = new ListRow(QString(), QStringLiteral("Row %1").arg(i));
      r->setSubtitle(QStringLiteral("Open"));
      area.addWidget(r);
      rows << r;
    }
    QCOMPARE(area.heightForWidth(300), 100);
    Harness::show(&area, QSize(300, 100));
    QVERIFY(area.isOverflowing());
    QVERIFY(!area.barRect().isEmpty());
    QCOMPARE(area.barRect().right(), 300.0);
    QVERIFY(area.thumbRect().height() >= shown().space(24));
    QCOMPARE(area.scrollY(), 0.0);
    area.ensureVisible(rows.last());
    QCOMPARE(area.scrollY(), area.maxScrollY());
    QVERIFY(area.thumbRect().bottom() > 99.0);
    area.ensureVisible(rows.first());
    QCOMPARE(area.scrollY(), 0.0);
    // Wheel scrolls and stops at the ends.
    QWheelEvent down(QPointF(10, 10), area.mapToGlobal(QPointF(10, 10)), QPoint(), QPoint(0, -1200), Qt::NoButton, Qt::NoModifier,
                     Qt::NoScrollPhase, false);
    QApplication::sendEvent(&area, &down);
    QCOMPARE(area.scrollY(), area.maxScrollY());
    // A short list doesn't overflow, so no bar.
    ScrollArea shortArea;
    shortArea.setMaxHeight(400);
    shortArea.addWidget(new ListRow(QString(), QStringLiteral("Only")));
    Harness::show(&shortArea, QSize(300, shortArea.heightForWidth(300)));
    QVERIFY(!shortArea.isOverflowing());
    QVERIFY(shortArea.barRect().isEmpty());
  }

  void avatar_falls_back_in_order() {
    Avatar a(QStringLiteral("ada"));
    QCOMPARE(Avatar::initial(QStringLiteral("  ada")), QStringLiteral("A"));
    QCOMPARE(a.drawn(), QStringLiteral("fallback"));
    a.setSource(QStringLiteral(OMARCHY_SOURCE_DIR "/../qt/gallery/assets/avatar.png"));
    QCOMPARE(a.drawn(), QStringLiteral("picture"));
    a.setSource(QStringLiteral("/nonexistent/missing.png"));
    QCOMPARE(a.drawn(), QStringLiteral("fallback"));
    a.setIconName(QStringLiteral("no-such-app-anywhere"));
    QCOMPARE(a.drawn(), QStringLiteral("fallback"));
    a.setSize(Avatar::Small);
    QCOMPARE(a.sizeHint(), QSize(shown().fontSize("iconLarge"), shown().fontSize("iconLarge")));
    a.setSize(Avatar::Large);
    QCOMPARE(a.sizeHint().width(), shown().fontSize("display") * 2);
  }

  void badge_is_never_narrower_than_tall() {
    Badge one(QStringLiteral("3"), QStringLiteral("accent"), true);
    QVERIFY(one.implicitSize().width() >= one.implicitSize().height());
    Badge word(QStringLiteral("Offline"), QStringLiteral("urgent"));
    QCOMPARE(word.implicitSize().height(), one.implicitSize().height());
    QVERIFY(word.implicitSize().width() > one.implicitSize().width());
  }

private:
  static QString glyph(uint cp) { return QString::fromUcs4(reinterpret_cast<const char32_t *>(&cp), 1); }
};

OMARCHY_TEST(Lists)

#include "tst_lists.moc"
