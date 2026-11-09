// Mirrors qt/tests/tst_forms.qml (Checkbox, RadioGroup, MultiSelect), plus PasswordField's
// reveal, the Dialog's keys and clicks, and the color pickers' keys.

#include <QApplication>
#include <QSignalSpy>
#include <QTest>

#include <Omarchy/Widgets.h>

#include "harness.h"

using namespace Omarchy;

// Show a host window and make it the active one, so focus moves as it would on a desktop.
static void showActive(QWidget *w, QSize size) {
  Harness::show(w, size);
  w->activateWindow();
  QVERIFY(QTest::qWaitForWindowActive(w));
}

// A key to whatever has focus now, as a keyboard would send it.
static void key(Qt::Key k, const QString &text = QString()) {
  QWidget *f = QApplication::focusWidget();
  QVERIFY(f);
  QTest::keyClick(f, k, Qt::NoModifier);
  Q_UNUSED(text);
}
static void type(const QString &s) {
  QWidget *f = QApplication::focusWidget();
  QVERIFY(f);
  QTest::keyClicks(f, s);
}

class Forms : public QObject {
  Q_OBJECT
private Q_SLOTS:
  void checkbox_flips_on_click_and_keys_unless_disabled() {
    Checkbox box(QStringLiteral("Show hidden files"));
    Harness::show(&box);
    QSignalSpy clicked(&box, &Checkbox::clicked);
    QTest::mouseClick(&box, Qt::LeftButton);
    QVERIFY(box.isChecked());
    box.setFocus();
    QTest::keyClick(&box, Qt::Key_Space);
    QVERIFY(!box.isChecked());
    QTest::keyClick(&box, Qt::Key_Return);
    QVERIFY(box.isChecked());
    QCOMPARE(clicked.count(), 3);
    box.setEnabled(false);
    QTest::mouseClick(&box, Qt::LeftButton);
    QVERIFY2(box.isChecked(), "a disabled checkbox doesn't flip");
    QTest::keyClick(&box, Qt::Key_Space);
    QVERIFY(box.isChecked());
    QCOMPARE(clicked.count(), 3);
  }

  void checkbox_caller_owns_the_value_without_auto_toggle() {
    Checkbox box(QStringLiteral("Show date"));
    box.setAutoToggle(false);
    Harness::show(&box);
    QSignalSpy clicked(&box, &Checkbox::clicked);
    QTest::mouseClick(&box, Qt::LeftButton);
    QCOMPARE(clicked.count(), 1);
    QVERIFY(!box.isChecked());
  }

  void radios_choose_on_click_and_keys() {
    RadioGroup group;
    group.setOptions(QList<Choice>{{QStringLiteral("top"), QStringLiteral("Top"), {}},
                                   {QStringLiteral("bottom"), QStringLiteral("Bottom"), {}},
                                   {QStringLiteral("hidden"), QStringLiteral("Hidden"), {}}});
    group.setValue(QStringLiteral("bottom"));
    Harness::show(&group);
    QSignalSpy changed(&group, &RadioGroup::changed);
    QCOMPARE(group.radio(2)->label(), QStringLiteral("Hidden"));
    QTest::mouseClick(group.radio(2), Qt::LeftButton);
    QCOMPARE(group.value(), QStringLiteral("hidden"));
    QCOMPARE(changed.first().first().toString(), QStringLiteral("hidden"));
    QVERIFY(group.radio(2)->isChecked() && !group.radio(1)->isChecked());
    QTest::mouseClick(group.radio(0), Qt::LeftButton);
    QCOMPARE(group.value(), QStringLiteral("top"));
    group.setFocus();
    QVERIFY2(group.radio(0)->hasCursor(), "the focused group's choice wears the cursor");
    const QList<QPair<Qt::Key, QString>> steps = {{Qt::Key_Down, "bottom"}, {Qt::Key_J, "hidden"}, {Qt::Key_Down, "hidden"}, {Qt::Key_K, "bottom"},
                                                  {Qt::Key_Up, "top"},     {Qt::Key_Up, "top"},    {Qt::Key_Right, "bottom"}, {Qt::Key_Left, "top"}};
    for (const auto &step : steps) {
      QTest::keyClick(&group, step.first);
      QCOMPARE(group.value(), step.second);
    }
    QCOMPARE(changed.count(), 8);
  }

  void multiselect_toggles_on_click() {
    QWidget host;
    MultiSelect ms(QStringLiteral("Notify for"), &host);
    ms.setOptions(QStringList{QStringLiteral("Calendar"), QStringLiteral("Mail"), QStringLiteral("Messages"), QStringLiteral("Updates"),
                              QStringLiteral("Battery")});
    ms.setValues({QStringLiteral("Mail"), QStringLiteral("Updates")});
    ms.setGeometry(0, 0, 260, ms.sizeHint().height());
    showActive(&host, QSize(600, 500));
    QTest::mouseClick(&ms, Qt::LeftButton, {}, ms.triggerRect().center().toPoint());
    QVERIFY(ms.isPopupOpen());
    QTRY_VERIFY2(ms.searchField()->hasFocus(), "the search field takes the keys");
    QTest::mouseClick(ms.popup(), Qt::LeftButton, {}, ms.popupRowRect(0).center().toPoint());
    QVERIFY(ms.values().contains(QStringLiteral("Calendar")));
    QTest::mouseClick(ms.popup(), Qt::LeftButton, {}, ms.popupRowRect(1).center().toPoint());
    QVERIFY(!ms.values().contains(QStringLiteral("Mail")));
    QCOMPARE(ms.values(), (QStringList{QStringLiteral("Updates"), QStringLiteral("Calendar")}));
    QCOMPARE(ms.selectionLabel(), QStringLiteral("Updates, Calendar"));
    // A press outside the popup and the trigger closes it; the trigger toggles it.
    QTest::mouseClick(&ms, Qt::LeftButton, {}, ms.triggerRect().center().toPoint());
    QVERIFY(!ms.isPopupOpen());
    QTest::mouseClick(&ms, Qt::LeftButton, {}, ms.triggerRect().center().toPoint());
    QVERIFY(ms.isPopupOpen());
    QTest::mouseClick(&host, Qt::LeftButton, {}, QPoint(500, 450));
    QVERIFY(!ms.isPopupOpen());
  }

  void multiselect_keys() {
    QWidget host;
    MultiSelect ms(QStringLiteral("Notify for"), &host);
    ms.setOptions(QStringList{QStringLiteral("Calendar"), QStringLiteral("Mail"), QStringLiteral("Messages"), QStringLiteral("Updates"),
                              QStringLiteral("Battery")});
    ms.setValues({QStringLiteral("Mail"), QStringLiteral("Updates")});
    ms.setGeometry(0, 0, 260, ms.sizeHint().height());
    showActive(&host, QSize(600, 500));
    QSignalSpy changed(&ms, &MultiSelect::changed);
    ms.setFocus();
    QTest::keyClick(&ms, Qt::Key_J);
    QVERIFY2(ms.isPopupOpen(), "j opens it from the trigger");
    QTRY_VERIFY2(ms.searchField()->hasFocus(), "the search field takes the keys");
    type(QStringLiteral("mes"));
    key(Qt::Key_Return);
    QCOMPARE(ms.values(), (QStringList{QStringLiteral("Mail"), QStringLiteral("Updates"), QStringLiteral("Messages")}));
    for (int i = 0; i < 3; ++i) key(Qt::Key_Backspace);
    key(Qt::Key_Down);
    key(Qt::Key_Space);
    QCOMPARE(ms.values(), (QStringList{QStringLiteral("Mail"), QStringLiteral("Updates"), QStringLiteral("Messages"), QStringLiteral("Calendar")}));
    key(Qt::Key_J);
    key(Qt::Key_Return);
    QCOMPARE(ms.values(), (QStringList{QStringLiteral("Updates"), QStringLiteral("Messages"), QStringLiteral("Calendar")}));
    QCOMPARE(changed.count(), 3);
    // Up off the first row goes back to the search; walking stops at the last row.
    key(Qt::Key_K);
    QCOMPARE(ms.popupIndex(), 0);
    key(Qt::Key_Up);
    QVERIFY(ms.searchField()->hasFocus());
    key(Qt::Key_Down);
    for (int i = 0; i < 8; ++i) key(Qt::Key_Down);
    QCOMPARE(ms.popupIndex(), 4);
    key(Qt::Key_Escape);
    QVERIFY2(!ms.isPopupOpen(), "Escape closes it");
    QVERIFY(ms.hasFocus());
    QCOMPARE(ms.selectionLabel(), QStringLiteral("Updates, Messages, Calendar"));
    ms.toggleValue(QStringLiteral("Battery"));
    QCOMPARE(ms.selectionLabel(), QStringLiteral("4 selected"));
  }

  void password_field_reveals_with_the_eye() {
    PasswordField pf;
    pf.setText(QStringLiteral("hunter22"));
    Harness::show(&pf, QSize(220, pf.sizeHint().height()));
    QVERIFY2(pf.eyeRect().isEmpty(), "no eye unless revealable");
    QCOMPARE(pf.field()->echoMode(), QLineEdit::Password);
    QCOMPARE(pf.field()->displayText(), QString(8, QChar(0x2022)));
    pf.setRevealable(true);
    QVERIFY(!pf.eyeRect().isEmpty());
    QSignalSpy revealed(&pf, &PasswordField::revealedChanged);
    QTest::mouseClick(pf.field(), Qt::LeftButton, {}, pf.eyeRect().center().toPoint());
    QVERIFY(pf.isRevealed());
    QCOMPARE(pf.field()->displayText(), QStringLiteral("hunter22"));
    QTest::mouseClick(pf.field(), Qt::LeftButton, {}, pf.eyeRect().center().toPoint());
    QVERIFY(!pf.isRevealed());
    QCOMPARE(revealed.count(), 2);
    QCOMPARE(pf.text(), QStringLiteral("hunter22"));
    // Typing and Return.
    QSignalSpy accepted(&pf, &PasswordField::accepted);
    pf.field()->setFocus();
    pf.field()->selectAll();
    QTest::keyClicks(pf.field(), QStringLiteral("letmein"));
    QTest::keyClick(pf.field(), Qt::Key_Return);
    QCOMPARE(pf.text(), QStringLiteral("letmein"));
    QCOMPARE(accepted.count(), 1);
    // An error adds its message under the field.
    const int plain = pf.heightForWidth(220);
    pf.setError(QStringLiteral("Wrong password, try again"));
    QVERIFY(pf.hasError());
    QVERIFY(pf.heightForWidth(220) > plain);
  }

  void text_area_grows_between_its_rows() {
    TextArea area;
    area.setFixedWidth(300);
    Harness::show(&area, QSize(300, area.sizeHint().height()));
    const int line = 16;
    const int three = area.sizeHint().height();
    QCOMPARE(area.rows(), 3);
    area.setText(QStringLiteral("1\n2\n3\n4\n5"));
    QCOMPARE(area.rows(), 5);
    QCOMPARE(area.sizeHint().height(), three + 2 * line);
    area.setText(QStringLiteral("1\n2\n3\n4\n5\n6\n7\n8"));
    QCOMPARE(area.rows(), 6);
    area.area()->setFocus();
    QTest::keyClicks(area.area(), QStringLiteral("x"));
    QVERIFY(area.text().contains(QLatin1Char('x')));
  }

  void dialog_keys_and_clicks() {
    QWidget host;
    Dialog d(QStringLiteral("Discard changes?"), QStringLiteral("Your edits will be lost."), &host);
    d.setDestructive(true);
    showActive(&host, QSize(420, 200));
    QVERIFY(!d.isOpened());
    d.open();
    QVERIFY(d.isOpened() && d.hasFocus());
    QCOMPARE(d.secondaryText(), QStringLiteral("Cancel"));
    QCOMPARE(d.primaryText(), QStringLiteral("Confirm"));
    QSignalSpy accepted(&d, &Dialog::accepted);
    QSignalSpy rejected(&d, &Dialog::rejected);
    QCOMPARE(d.selectedIndex(), 1);
    QTest::keyClick(&d, Qt::Key_Return);
    QCOMPARE(accepted.count(), 1);
    QTest::keyClick(&d, Qt::Key_Left);
    QCOMPARE(d.selectedIndex(), 0);
    QTest::keyClick(&d, Qt::Key_Enter);
    QCOMPARE(rejected.count(), 1);
    QTest::keyClick(&d, Qt::Key_L);
    QCOMPARE(d.selectedIndex(), 1);
    QTest::keyClick(&d, Qt::Key_Tab);
    QCOMPARE(d.selectedIndex(), 0);
    QVERIFY2(d.hasFocus(), "Tab stays in the dialog");
    QTest::keyClick(&d, Qt::Key_H);
    QCOMPARE(d.selectedIndex(), 1);
    QTest::keyClick(&d, Qt::Key_Escape);
    QCOMPARE(rejected.count(), 2);
    QTest::mouseClick(&d, Qt::LeftButton, {}, d.buttonRect(1).center().toPoint());
    QCOMPARE(accepted.count(), 2);
    QTest::mouseClick(&d, Qt::LeftButton, {}, d.buttonRect(0).center().toPoint());
    QCOMPARE(rejected.count(), 3);
    QTest::mouseClick(&d, Qt::LeftButton, {}, d.cardRect().center().toPoint());
    QCOMPARE(rejected.count() + accepted.count(), 5);  // a press on the card is no choice
    QTest::mouseClick(&d, Qt::LeftButton, {}, QPoint(2, 2));
    QCOMPARE(rejected.count(), 4);
    // handleKey, for a panel that keeps focus.
    QKeyEvent esc(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QVERIFY(d.handleKey(&esc));
    QCOMPARE(rejected.count(), 5);
    QKeyEvent other(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier, QStringLiteral("x"));
    QVERIFY(!d.handleKey(&other));
    d.close();
    QVERIFY(!d.handleKey(&esc));
  }

  void swatch_strip_and_color_well_keys() {
    SwatchStrip strip;
    strip.setColors(QList<QColor>{Qt::red, Qt::green, Qt::blue, Qt::yellow});
    strip.setCurrentIndex(2);
    Harness::show(&strip);
    QSignalSpy picked(&strip, &SwatchStrip::picked);
    strip.setFocus();
    QTest::keyClick(&strip, Qt::Key_L);
    QCOMPARE(strip.cursorIndex(), 3);
    QTest::keyClick(&strip, Qt::Key_Right);
    QCOMPARE(strip.cursorIndex(), 3);
    QTest::keyClick(&strip, Qt::Key_H);
    QTest::keyClick(&strip, Qt::Key_Left);
    QCOMPARE(strip.cursorIndex(), 1);
    QTest::keyClick(&strip, Qt::Key_Space);
    QCOMPARE(strip.currentIndex(), 1);
    QTest::mouseClick(&strip, Qt::LeftButton, {}, strip.cellRect(0).center().toPoint());
    QCOMPARE(strip.currentIndex(), 0);
    QCOMPARE(picked.count(), 2);

    ColorWell well(QColor(Qt::red));
    Harness::show(&well);
    QSignalSpy clicked(&well, &ColorWell::clicked);
    QTest::mouseClick(&well, Qt::LeftButton);
    well.setFocus();
    QTest::keyClick(&well, Qt::Key_Space);
    QTest::keyClick(&well, Qt::Key_Return);
    QCOMPARE(clicked.count(), 3);
  }
};

OMARCHY_TEST(Forms)

#include "tst_forms.moc"
