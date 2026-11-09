// The live switch for a Kirigami app (spec/following.md, under tools/live/run): a plain Kirigami
// item's Kirigami.Theme colors, and a Qt Quick Controls control drawn by KDE's desktop style,
// follow a theme switch after nothing but Omarchy::Widgets::apply().

#include <QApplication>
#include <QElapsedTimer>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QTest>
#include <cstdio>

#include <Omarchy/Widgets.h>

static int failures = 0;
static void check(bool ok, const char *what) {
  printf("%s: %s\n", ok ? "ok" : "FAIL", what);
  if (!ok) ++failures;
}

int main(int argc, char **argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);
  const QByteArray home = qgetenv("OMARCHY_UI_LIVE_HOME"), sw = qgetenv("OMARCHY_UI_LIVE_SWITCH");
  if (home.isEmpty() || sw.isEmpty()) {
    fprintf(stderr, "run me under tools/live/run\n");
    return 2;
  }
  Omarchy::Widgets::apply(app);
  check(qgetenv("QT_QUICK_CONTROLS_STYLE") == "org.kde.desktop", "Qt Quick Controls use KDE's desktop style (drawn by Omarchy::Style)");

  QQmlEngine engine;
  QQmlComponent c(&engine);
  c.setData("import QtQuick\nimport QtQuick.Controls\nimport org.kde.kirigami as Kirigami\n"
            "ApplicationWindow { visible: true; width: 200; height: 100\n"
            "  property alias bg: probe.bg; property alias fg: probe.fg\n"
            "  Item { id: probe; property color bg: Kirigami.Theme.backgroundColor; property color fg: Kirigami.Theme.textColor }\n"
            "  Button { text: \"Connect\" } }",
            QUrl());
  QObject *item = c.create();
  if (!item) {
    fprintf(stderr, "%s\n", qPrintable(c.errorString()));
    printf("FAILED: no Kirigami\n");
    return 1;
  }
  const QColor night(0x1a, 0x1b, 0x26), latte(0xef, 0xf1, 0xf5);
  QTest::qWait(200);
  check(item->property("bg").value<QColor>() == night, "Kirigami.Theme.backgroundColor starts on tokyo-night's");
  check(item->property("fg").value<QColor>() == QColor(0xa9, 0xb1, 0xd6), "Kirigami.Theme.textColor is the foreground");

  QProcess::execute(QString::fromLocal8Bit(sw), {QString::fromLocal8Bit(home), QStringLiteral("catppuccin-latte")});
  QElapsedTimer t;
  t.start();
  while (item->property("bg").value<QColor>() != latte && t.elapsed() < 5000) QTest::qWait(20);
  check(item->property("bg").value<QColor>() == latte, "Kirigami.Theme.backgroundColor follows the switch to #eff1f5");
  check(item->property("fg").value<QColor>() == QColor(0x4c, 0x4f, 0x69), "Kirigami.Theme.textColor follows it too");
  delete item;
  printf("%s: Kirigami live switch\n", failures ? "FAILED" : "passed");
  return failures ? 1 : 0;
}
