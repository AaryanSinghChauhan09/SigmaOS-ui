// The Qt gallery: every Omarchy UI component in the live theme, live-updating on theme switches.
//
//   omarchy-qt-gallery                          a window, following the desktop theme
//   omarchy-qt-gallery --section buttons        one section (spec/gallery.md)
//   omarchy-qt-gallery --snap out.png           render offscreen to a PNG and exit
//   omarchy-qt-gallery --dump-tokens            print the canonical token JSON and exit
//   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-qt-gallery --snap nord.png

#include <QCommandLineParser>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonValue>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QTimer>
#include <cstdio>

#include "omarchyui.h"

class Gallery : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString section MEMBER section CONSTANT)
  Q_PROPERTY(int width MEMBER width CONSTANT)

public:
  QString section;
  int width = 760;
};

static bool hasArg(int argc, char **argv, const char *flag) {
  for (int i = 1; i < argc; ++i)
    if (qstrcmp(argv[i], flag) == 0) return true;
  return false;
}

static const char *argValue(int argc, char **argv, const char *flag, const char *fallback) {
  for (int i = 1; i + 1 < argc; ++i)
    if (qstrcmp(argv[i], flag) == 0) return argv[i + 1];
  return fallback;
}

int main(int argc, char **argv) {
  const bool headless = hasArg(argc, argv, "--snap") || hasArg(argc, argv, "--dump-tokens");
  if (headless) qputenv("QT_QPA_PLATFORM", "offscreen");
  // The device pixel ratio has to be set before the application exists.
  if (hasArg(argc, argv, "--snap")) qputenv("QT_SCALE_FACTOR", argValue(argc, argv, "--scale", "2"));
  qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");

  QGuiApplication app(argc, argv);
  app.setApplicationName("omarchy-qt-gallery");
  app.setDesktopFileName("omarchy-qt-gallery");

  QCommandLineParser args;
  args.addHelpOption();
  args.addOption({"section", "Show only this section.", "name"});
  args.addOption({"snap", "Render offscreen to this PNG and exit.", "path"});
  args.addOption({"scale", "Device pixel ratio for --snap (default 2).", "n", "2"});
  args.addOption({"width", "Window width in points (default 760).", "n", "760"});
  args.addOption({"dump-tokens", "Print the resolved tokens as canonical JSON and exit."});
  args.process(app);

  Gallery gallery;
  gallery.section = args.value("section");
  gallery.width = args.value("width").toInt();

  OmarchyUi::registerTypes();
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("gallery", &gallery);

  if (args.isSet("dump-tokens")) {
    QQmlComponent dumper(&engine);
    dumper.setData("import QtQuick\nimport Omarchy.Theme\nQtObject { function dump() { return JSON.stringify(Tokens.canonical()) } }", QUrl());
    QObject *obj = dumper.create();
    if (!obj) {
      fprintf(stderr, "%s\n", qPrintable(dumper.errorString()));
      return 1;
    }
    QVariant out;
    QMetaObject::invokeMethod(obj, "dump", Q_RETURN_ARG(QVariant, out));
    const QJsonDocument doc = QJsonDocument::fromJson(out.toString().toUtf8());
    printf("%s", doc.toJson(QJsonDocument::Indented).constData());
    return 0;
  }

  engine.load(QUrl("qrc:/gallery/Main.qml"));
  if (engine.rootObjects().isEmpty()) return 1;
  auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());

  if (args.isSet("snap")) {
    const QString path = args.value("snap");
    // Let layout settle and a few frames render before grabbing.
    QTimer::singleShot(400, &app, [&] {
      window->setWidth(gallery.width);
      QTimer::singleShot(200, &app, [&] {
        const QImage img = window->grabWindow();
        if (img.isNull() || !img.save(path)) {
          fprintf(stderr, "could not write %s\n", qPrintable(path));
          app.exit(1);
          return;
        }
        app.exit(0);
      });
    });
  }
  return app.exec();
}

#include "main.moc"
