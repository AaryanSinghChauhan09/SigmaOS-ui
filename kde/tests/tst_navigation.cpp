// Mirrors qt/tests/tst_navigation.qml: what keys and clicks do to Menu (rows, the cursor,
// submenus, Escape), Tabs and IconRail; plus the popup paths (ContextMenu at a point, a press
// outside) and the key names Keycap draws.

#include <QApplication>
#include <QContextMenuEvent>
#include <QSignalSpy>
#include <QTest>

#include <Omarchy/Widgets.h>

#include "harness.h"

using namespace Omarchy;

namespace {

// The navigation section's menu, held open: a disabled row and a submenu row among plain ones.
struct TestMenu {
  Menu menu;
  QStringList ran;
  bool sidebar = true;
  Menu *sub = nullptr;

  TestMenu() {
    menu.setHeld(true);
    auto run = [this](MenuItem *it) { QObject::connect(it, &MenuItem::triggered, &menu, [this, it] { ran << it->text(); }); };
    run(menu.addItem(QStringLiteral("New"), QString(), QString(QChar(0xF15B))));
    run(menu.addItem(QStringLiteral("Open…")));
    run(menu.addItem(QStringLiteral("Save")));
    MenuItem *dup = menu.addItem(QStringLiteral("Duplicate"));
    dup->setEnabled(false);
    run(dup);
    menu.addSeparator();
    MenuItem *side = menu.addItem(QStringLiteral("Sidebar"));
    side->setCheckable(true);
    side->setChecked(sidebar);
    QObject::connect(side, &MenuItem::triggered, &menu, [this, side] {
      sidebar = !sidebar;
      side->setChecked(sidebar);
    });
    MenuItem *exp = menu.addItem(QStringLiteral("Export"));
    sub = new Menu;
    run(sub->addItem(QStringLiteral("PDF")));
    run(sub->addItem(QStringLiteral("PNG")));
    exp->setSubmenu(sub);
  }

  MenuItem *row(const QString &label) const {
    for (MenuItem *r : menu.rows())
      if (r->text() == label) return r;
    return nullptr;
  }

  void show() {
    Harness::show(&menu, menu.sizeHint());
    menu.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&menu));
    // The pointer off the rows (a previous test's clicks leave it over one, which takes the cursor).
    QTest::mouseMove(&menu, QPoint(menu.width() + 40, menu.height() + 40));
    menu.setCurrentIndex(-1);
    menu.setFocus();
  }
};

// Where the keyboard goes now: the open popup, else the focus widget.
QWidget *keyTarget() {
  if (QWidget *p = QApplication::activePopupWidget()) return p->focusWidget() ? p->focusWidget() : p;
  return QApplication::focusWidget();
}

void key(Qt::Key k) { QTest::keyClick(keyTarget(), k); }
void key(char c) { QTest::keyClick(keyTarget(), c); }

}  // namespace

class Navigation : public QObject {
  Q_OBJECT
private Q_SLOTS:
  void menu_rows_run_on_click() {
    TestMenu t;
    t.show();
    QTest::mouseClick(t.row(QStringLiteral("New")), Qt::LeftButton);
    QCOMPARE(t.ran, QStringList{QStringLiteral("New")});
    QTest::mouseClick(t.row(QStringLiteral("Sidebar")), Qt::LeftButton);
    QVERIFY2(!t.sidebar, "a check row flips");
    QTest::mouseClick(t.row(QStringLiteral("Duplicate")), Qt::LeftButton);
    QCOMPARE(t.ran, QStringList{QStringLiteral("New")});  // a disabled row doesn't run
  }

  void menu_walks_with_vim_keys_and_arrows() {
    TestMenu t;
    t.show();
    Menu &m = t.menu;
    key('j');
    QCOMPARE(m.currentItem()->text(), QStringLiteral("New"));  // the first key lands on the first row
    key('j');
    key(Qt::Key_Return);
    QCOMPARE(t.ran, QStringList{QStringLiteral("Open…")});
    key(Qt::Key_Down);
    key(Qt::Key_Space);
    QCOMPARE(t.ran, (QStringList{QStringLiteral("Open…"), QStringLiteral("Save")}));
    key('k');
    key(Qt::Key_Enter);
    QCOMPARE(t.ran, (QStringList{QStringLiteral("Open…"), QStringLiteral("Save"), QStringLiteral("Open…")}));
    key('j');
    key('j');
    QCOMPARE(m.currentItem()->text(), QStringLiteral("Sidebar"));  // skips the disabled row
    key(Qt::Key_Return);
    QVERIFY(!t.sidebar);
    key(Qt::Key_Down);
    key(Qt::Key_Down);
    QCOMPARE(m.currentItem()->text(), QStringLiteral("New"));  // wraps past the last row
    key(Qt::Key_Up);
    QCOMPARE(m.currentItem()->text(), QStringLiteral("Export"));  // and back past the first
    key(Qt::Key_Home);
    QCOMPARE(m.currentItem()->text(), QStringLiteral("New"));
    key(Qt::Key_End);
    QCOMPARE(m.currentItem()->text(), QStringLiteral("Export"));
  }

  void menu_submenu_opens_right_and_closes_left() {
    TestMenu t;
    t.show();
    Menu &m = t.menu;
    Menu *sub = t.sub;
    key(Qt::Key_End);
    key('l');
    QVERIFY2(sub->isOpened() && sub->isVisible(), "l opens the submenu");
    QVERIFY(sub->isWindow());
    QCOMPARE(keyTarget(), sub);
    QCOMPARE(sub->currentItem()->text(), QStringLiteral("PDF"));
    // It opens to the right of the row, its first row level with it.
    const QRect exportRow(t.row(QStringLiteral("Export"))->mapToGlobal(QPoint(0, 0)), t.row(QStringLiteral("Export"))->size());
    QVERIFY(sub->geometry().left() >= exportRow.right());
    QVERIFY(qAbs(sub->rows().first()->mapToGlobal(QPoint(0, 0)).y() - exportRow.top()) <= 1);
    key('j');
    key(Qt::Key_Return);
    QCOMPARE(t.ran, QStringList{QStringLiteral("PNG")});
    QVERIFY2(!sub->isOpened() && !sub->isVisible(), "running a row closes the submenu");
    QCOMPARE(keyTarget(), &m);  // and the held menu has the keys again
    key(Qt::Key_End);
    key(Qt::Key_Right);
    QVERIFY(sub->isOpened());
    key('h');
    QVERIFY2(!sub->isOpened(), "h goes back out");
    QCOMPARE(keyTarget(), &m);
    key(Qt::Key_Right);
    QVERIFY(sub->isOpened());
    key(Qt::Key_Left);
    QVERIFY2(!sub->isOpened(), "so does Left");
    key(Qt::Key_Right);
    key(Qt::Key_Escape);
    QVERIFY2(!sub->isOpened(), "and Escape");
    QVERIFY(m.isOpened() || m.isHeld());
  }

  void menu_escape_closes_an_open_menu() {
    TestMenu t;
    t.menu.setHeld(false);
    QSignalSpy closed(&t.menu, &Menu::closed);
    t.menu.popup(QPoint(40, 40));
    QVERIFY(QTest::qWaitForWindowExposed(&t.menu));
    QVERIFY(t.menu.isOpened());
    QVERIFY(t.menu.isWindow());
    QCOMPARE(QApplication::activePopupWidget(), &t.menu);
    key(Qt::Key_Escape);
    QVERIFY(!t.menu.isOpened());
    QVERIFY(!t.menu.isVisible());
    QCOMPARE(closed.count(), 1);
  }

  void menu_popup_runs_a_row_and_closes_the_chain() {
    TestMenu t;
    t.menu.setHeld(false);
    QSignalSpy triggered(&t.menu, &Menu::triggered);
    t.menu.popup(QPoint(40, 40));
    QVERIFY(QTest::qWaitForWindowExposed(&t.menu));
    key(Qt::Key_End);
    key('l');
    QVERIFY(t.sub->isOpened());
    key(Qt::Key_Return);
    QCOMPARE(t.ran, QStringList{QStringLiteral("PDF")});
    QCOMPARE(triggered.count(), 1);  // the top menu reports rows its submenus ran
    QVERIFY(!t.sub->isVisible());
    QVERIFY2(!t.menu.isVisible() && !t.menu.isOpened(), "running a row closes the whole chain");
  }

  void context_menu_opens_at_the_pointer() {
    QWidget canvas;
    Harness::show(&canvas, QSize(300, 200));
    ContextMenu ctx(&canvas);
    ctx.addItem(QStringLiteral("Copy"), QStringLiteral("Ctrl+C"));
    ctx.setTarget(&canvas);
    const QPoint at = canvas.mapToGlobal(QPoint(50, 60));
    QContextMenuEvent e(QContextMenuEvent::Mouse, QPoint(50, 60), at);
    QApplication::sendEvent(&canvas, &e);
    QVERIFY(ctx.isOpened());
    QVERIFY(QTest::qWaitForWindowExposed(&ctx));
    QCOMPARE(ctx.geometry().topLeft(), at);
    QCOMPARE(ctx.size(), ctx.sizeHint());
    key(Qt::Key_Escape);
    QVERIFY(!ctx.isOpened());
  }

  void menu_closes_on_a_press_outside() {
    QWidget page;
    Harness::show(&page, QSize(400, 300));
    TestMenu t;
    t.menu.setParent(&page, t.menu.windowFlags());
    t.menu.setHeld(false);
    t.menu.setTrigger(&page);
    t.menu.popup(page.mapToGlobal(QPoint(10, 10)));
    QVERIFY(QTest::qWaitForWindowExposed(&t.menu));
    key(Qt::Key_End);
    key('l');
    QVERIFY(t.sub->isOpened());
    // While a popup is open it has the pointer: the press goes to it, outside its rectangle.
    const QPoint outside = page.mapToGlobal(QPoint(390, 290));
    QTest::mouseClick(t.sub, Qt::LeftButton, {}, t.sub->mapFromGlobal(outside));
    QVERIFY2(!t.sub->isOpened() && !t.menu.isOpened(), "a press outside closes the chain");
    QVERIFY(!t.menu.isVisible() && !t.sub->isVisible());
    QVERIFY(!QApplication::activePopupWidget());
    // A press on the parent menu closes only the submenu.
    t.menu.popup(page.mapToGlobal(QPoint(10, 10)));
    key(Qt::Key_End);
    key('l');
    QVERIFY(t.sub->isOpened());
    QTest::mouseClick(t.sub, Qt::LeftButton, {}, t.sub->mapFromGlobal(t.menu.mapToGlobal(QPoint(20, 20))));
    QVERIFY(!t.sub->isOpened());
    QVERIFY(t.menu.isOpened() && t.menu.isVisible());
    QCOMPARE(keyTarget(), &t.menu);
  }

  void menu_opens_below_its_trigger() {
    QWidget page;
    auto *button = new Button(QStringLiteral("File"), &page);
    button->move(30, 20);
    Harness::show(&page, QSize(400, 300));
    Menu menu(&page);
    menu.addItem(QStringLiteral("Open…"));
    menu.setTrigger(button);
    menu.open();
    QVERIFY(QTest::qWaitForWindowExposed(&menu));
    QCOMPARE(menu.geometry().topLeft(), button->mapToGlobal(QPoint(0, button->height() + shown().spacing("xs"))));
    QCOMPARE(menu.currentIndex(), -1);
    key(Qt::Key_Escape);
    QVERIFY(!menu.isOpened());
  }

  void tooltip_shows_above_its_parent_with_the_shortcut() {
    QWidget page;
    auto *button = new Button(QStringLiteral("Save"), &page);
    button->move(150, 120);
    Harness::show(&page, QSize(400, 300));
    Tooltip tip(QStringLiteral("Save"), button);
    tip.setShortcut(QStringLiteral("Ctrl+S"));
    tip.setDelay(0);
    QVERIFY(!tip.isVisible());
    QVERIFY(tip.sizeHint().width() > Tooltip(QStringLiteral("Save")).sizeHint().width());  // room for the Keycaps
    QEvent enter(QEvent::Enter);
    QApplication::sendEvent(button, &enter);
    QTRY_VERIFY(tip.isVisible());
    QVERIFY(tip.isWindow());
    const QPoint b = button->mapToGlobal(QPoint(0, 0));
    QCOMPARE(tip.geometry().bottom() + 1 + shown().spacing("sm"), b.y());
    QVERIFY(qAbs(tip.geometry().center().x() - (b.x() + button->width() / 2)) <= 1);
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(button, &leave);
    QTRY_VERIFY(!tip.isVisible());
  }

  void tabs_choose_on_click_and_keys() {
    Tabs tabs;
    tabs.setModel({QStringLiteral("General"), QStringLiteral("Display"), QStringLiteral("Audio"), QStringLiteral("Network")});
    tabs.setCurrentIndex(1);
    Harness::show(&tabs, QSize(400, tabs.sizeHint().height()));
    tabs.activateWindow();
    QVERIFY(QTest::qWaitForWindowActive(&tabs));
    QSignalSpy activated(&tabs, &Tabs::activated);
    QTest::mouseClick(&tabs, Qt::LeftButton, {}, tabs.tabRect(3).center().toPoint());
    QCOMPARE(tabs.currentIndex(), 3);
    QCOMPARE(activated.count(), 1);
    QVERIFY2(tabs.hasFocus(), "a click focuses the tabs");
    tabs.setCurrentIndex(1);
    const QList<QPair<QVariant, int>> steps = {{int(Qt::Key_Right), 2}, {QChar('l'), 3}, {int(Qt::Key_Right), 3}, {QChar('h'), 2},
                                               {int(Qt::Key_Left), 1},  {int(Qt::Key_Home), 0}, {int(Qt::Key_Left), 0}, {int(Qt::Key_End), 3},
                                               {int(Qt::Key_Up), 3},    {int(Qt::Key_Down), 3}};
    for (const auto &[k, want] : steps) {
      if (k.typeId() == QMetaType::QChar) QTest::keyClick(&tabs, k.toChar().toLatin1());
      else QTest::keyClick(&tabs, Qt::Key(k.toInt()));
      QVERIFY2(tabs.currentIndex() == want, qPrintable(QStringLiteral("tabs after %1: %2").arg(k.toString()).arg(tabs.currentIndex())));
    }
  }

  void rail_chooses_on_click_and_keys() {
    IconRail rail;
    rail.setModel({{QString(QChar(0xF07B)), QStringLiteral("Files")},
                   {QString(QChar(0xF002)), QStringLiteral("Search")},
                   {QString(QChar(0xF126)), QStringLiteral("Source control")},
                   {QString(QChar(0xF188)), QStringLiteral("Debug")},
                   {QString(QChar(0xF013)), QStringLiteral("Settings")}});
    Harness::show(&rail, rail.sizeHint());
    QSignalSpy activated(&rail, &IconRail::activated);
    QTest::mouseClick(&rail, Qt::LeftButton, {}, QPoint(rail.width() / 2, rail.height() - rail.itemSize() / 2 - 4));
    QCOMPARE(rail.currentIndex(), 4);  // a press on the last button
    QCOMPARE(activated.count(), 1);
    rail.setFocus();
    rail.setCurrentIndex(0);
    const QList<QPair<QVariant, int>> steps = {{int(Qt::Key_Down), 1}, {QChar('j'), 2},         {QChar('k'), 1},        {int(Qt::Key_Up), 0},
                                               {int(Qt::Key_Up), 0},   {int(Qt::Key_End), 4},   {int(Qt::Key_Down), 4}, {int(Qt::Key_Home), 0},
                                               {int(Qt::Key_Right), 0}, {int(Qt::Key_Left), 0}};
    for (const auto &[k, want] : steps) {
      if (k.typeId() == QMetaType::QChar) QTest::keyClick(&rail, k.toChar().toLatin1());
      else QTest::keyClick(&rail, Qt::Key(k.toInt()));
      QVERIFY2(rail.currentIndex() == want, qPrintable(QStringLiteral("rail after %1: %2").arg(k.toString()).arg(rail.currentIndex())));
    }
  }

  void keycaps_name_keys_as_every_binding_does() {
    QCOMPARE(keyLabel(QStringLiteral("cmd")), QStringLiteral("Super"));
    QCOMPARE(keyLabel(QStringLiteral("Control")), QStringLiteral("Ctrl"));
    QCOMPARE(keyLabel(QStringLiteral("shift")), QStringLiteral("⇧"));
    QCOMPARE(keyLabel(QStringLiteral("Return")), QStringLiteral("⏎"));
    QCOMPARE(keyLabel(QStringLiteral("pagedown")), QStringLiteral("PgDn"));
    QCOMPARE(keyLabel(QStringLiteral("k")), QStringLiteral("K"));
    QCOMPARE(keyLabel(QStringLiteral("F11")), QStringLiteral("F11"));
    QCOMPARE(splitCombo(QStringLiteral("Ctrl+Shift+S")), (QStringList{QStringLiteral("Ctrl"), QStringLiteral("Shift"), QStringLiteral("S")}));
    QCOMPARE(splitCombo(QStringLiteral("Ctrl++")), (QStringList{QStringLiteral("Ctrl"), QStringLiteral("+")}));
  }
};

OMARCHY_TEST(Navigation)

#include "tst_navigation.moc"
