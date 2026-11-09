// A Kirigami app on the Omarchy theme: one call before the engine loads, nothing else.

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <cstring>

#include <Omarchy/Widgets.h>

int main(int argc, char **argv) {
  const char *snap = nullptr;
  for (int i = 1; i + 1 < argc; ++i)
    if (std::strcmp(argv[i], "--snap") == 0) snap = argv[i + 1];
  if (snap) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    qputenv("QT_SCALE_FACTOR", "2");
  }

  // A QApplication: KDE's desktop style for Qt Quick Controls draws through the QStyle.
  QApplication app(argc, argv);
  // Palette, Omarchy::Style, the font, the KDE color scheme Kirigami reads, and the
  // org.kde.desktop controls style (unless QT_QUICK_CONTROLS_STYLE is set) - kept live.
  Omarchy::Widgets::apply(app);

  QQmlApplicationEngine engine(QUrl(QStringLiteral("qrc:/Main.qml")));
  if (engine.rootObjects().isEmpty()) return 1;
  auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
  if (snap && window) {
    QTimer::singleShot(800, &app, [&] { app.exit(window->grabWindow().save(QString::fromLocal8Bit(snap)) ? 0 : 1); });
  }
  return app.exec();
}
