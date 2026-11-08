// A tiny Omarchy panel, vendoring the kit through qt/omarchy-ui.pri.
//
//   omarchy-ui-example-vendored                  a window, following the desktop theme
//   omarchy-ui-example-vendored --snap out.png   render once offscreen at 2x and exit

#include <QGuiApplication>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>

#include "omarchyui.h"

int main(int argc, char **argv) {
  const QStringList args(argv, argv + argc);
  const qsizetype snap = args.indexOf("--snap");
  if (snap > 0) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_SCALE_FACTOR", "2");
  }
  // The kit's components are drawn whole; no platform style underneath them.
  qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

  QGuiApplication app(argc, argv);
  OmarchyUi::registerTypes();  // before the engine loads anything

  QQmlApplicationEngine engine;
  engine.load(QUrl("qrc:/example/Main.qml"));
  if (engine.rootObjects().isEmpty()) return 1;

  if (snap > 0 && snap + 1 < args.size()) {
    auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    const QString path = args[snap + 1];
    QTimer::singleShot(600, &app, [&app, window, path] { app.exit(window->grabWindow().save(path) ? 0 : 1); });
  }
  return app.exec();
}
