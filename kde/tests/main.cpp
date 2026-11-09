// Runs every OMARCHY_TEST class (tests/tst_*.cpp), offscreen, on the tokyo-night fixture at
// radius 0 with motion off, so results don't depend on the desktop. Extra arguments go to QTest
// (-silent, a function name, ...).
//
//   build/kde/omarchy-widgets-tests [-silent] [Class]

#include <QApplication>
#include <QDir>
#include <QWidget>
#include <cstdio>

#include <Omarchy/Widgets.h>

#include "harness.h"

namespace Harness {

QList<Entry> &registry() {
  static QList<Entry> r;
  return r;
}

void show(QWidget *w, QSize size) {
  if (size.isValid()) w->resize(size);
  else w->resize(w->sizeHint().expandedTo(QSize(200, 60)));
  w->show();
  QTest::qWaitForWindowExposed(w);
  QTest::qWait(20);
}

}  // namespace Harness

int main(int argc, char **argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (!qEnvironmentVariableIsSet("OMARCHY_UI_THEME_DIR")) {
    // The checkout's fixtures, found from the binary (build/kde/..) or the working directory.
    for (const QString &root : {QStringLiteral(OMARCHY_SOURCE_DIR "/.."), QDir::currentPath()}) {
      const QString dir = root + QStringLiteral("/fixtures/themes/tokyo-night");
      if (QDir(dir).exists()) {
        qputenv("OMARCHY_UI_THEME_DIR", dir.toUtf8());
        break;
      }
    }
  }
  qputenv("OMARCHY_UI_ROUNDING", "0");
  qputenv("OMARCHY_UI_REDUCE_MOTION", "1");
  QApplication app(argc, argv);
  Omarchy::Widgets::apply(app);

  QStringList args;
  for (int i = 0; i < argc; ++i) args << QString::fromLocal8Bit(argv[i]);
  QString only;
  if (args.size() > 1 && !args.last().startsWith(QLatin1Char('-')) && args.last()[0].isUpper()) only = args.takeLast();

  int failed = 0, ran = 0;
  for (const Harness::Entry &e : Harness::registry()) {
    if (!only.isEmpty() && only != QLatin1String(e.name)) continue;
    QObject *t = e.make();
    failed += QTest::qExec(t, args) != 0 ? 1 : 0;
    ++ran;
    delete t;
  }
  printf("%s: %d of %d test classes passed\n", failed ? "FAILED" : "passed", ran - failed, ran);
  return failed ? 1 : 0;
}
