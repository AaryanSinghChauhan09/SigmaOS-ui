// The Qt gallery: every Omarchy UI component in the live theme, live-updating on theme switches.
//
//   omarchy-qt-gallery                          a window, following the desktop theme
//   omarchy-qt-gallery --section buttons        one section (spec/gallery.md)
//   omarchy-qt-gallery --snap out.png           render offscreen to a PNG and exit
//   omarchy-qt-gallery --dump-tokens            print the canonical token JSON and exit
//   omarchy-qt-gallery --qml view.qml --snap out.png   render any QML file with the kit (an Item or a Window)
//   omarchy-qt-gallery --watch-tokens           print it on one line, then a line per change (Tokens.changed)
//   omarchy-qt-gallery --icon firefox --icon-size 96   print the file an icon name resolves to
//   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-qt-gallery --snap nord.png

#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QQuickItem>
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
#include "themesource.h"
#include "iconlookup.h"

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
  const bool headless = hasArg(argc, argv, "--snap") || hasArg(argc, argv, "--dump-tokens") || hasArg(argc, argv, "--watch-tokens") || hasArg(argc, argv, "--icon");
  if (headless) qputenv("QT_QPA_PLATFORM", "offscreen");
  // The device pixel ratio has to be set before the application exists.
  if (hasArg(argc, argv, "--snap")) qputenv("QT_SCALE_FACTOR", argValue(argc, argv, "--scale", "2"));
  qputenv("QT_QUICK_CONTROLS_STYLE", "Basic");
  // Under a systemd session Qt logs to the journal; console.log from a --qml file belongs on stderr.
  if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) qputenv("QT_FORCE_STDERR_LOGGING", "1");

  QGuiApplication app(argc, argv);
  app.setApplicationName("omarchy-qt-gallery");
  app.setDesktopFileName("omarchy-qt-gallery");

  QCommandLineParser args;
  args.addHelpOption();
  args.addOption({"section", "Show only this section.", "name"});
  args.addOption({"qml", "Load this QML file (an Item or a Window) instead of the gallery.", "file"});
  args.addOption({"snap", "Render offscreen to this PNG and exit.", "path"});
  args.addOption({"scale", "Device pixel ratio for --snap (default 2).", "n", "2"});
  args.addOption({"width", "Window width in points (default 760).", "n", "760"});
  args.addOption({"dump-tokens", "Print the resolved tokens as canonical JSON and exit."});
  args.addOption({"watch-tokens", "Print the resolved tokens on one line, then a line per change (the changed hook)."});
  args.addOption({"icon", "Print the file this icon name resolves to in the icon theme (empty if none) and exit.", "name"});
  args.addOption({"icon-size", "Size in pixels for --icon (default 48).", "n", "48"});
  args.process(app);

  Gallery gallery;
  gallery.section = args.value("section");
  gallery.width = args.value("width").toInt();

  OmarchyUi::registerTypes();
  if (args.isSet("icon")) {
    printf("%s\n", qPrintable(IconLookup::instance()->path(args.value("icon"), args.value("icon-size").toInt())));
    return 0;
  }
  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("gallery", &gallery);

  if (args.isSet("dump-tokens") || args.isSet("watch-tokens")) {
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
    if (!args.isSet("watch-tokens")) {
      printf("%s", doc.toJson(QJsonDocument::Indented).constData());
      return 0;
    }
    // As `omarchy-theme --watch` prints them: one compact line now, then one per change, from
    // the C++ changed hook (the same moment and tokens as QML's Tokens.changed).
    printf("%s\n", doc.toJson(QJsonDocument::Compact).constData());
    fflush(stdout);
    QObject::connect(OmarchyUi::Theme::instance(), &OmarchyUi::Theme::changed, &app, [](const QJsonObject &tokens) {
      printf("%s\n", QJsonDocument(tokens).toJson(QJsonDocument::Compact).constData());
      fflush(stdout);
    });
    return app.exec();
  }

  QQuickWindow *window = nullptr;
  QQuickItem *hosted = nullptr;
  const int pad = 16;
  if (args.isSet("qml")) {
    // Any QML file: a Window shows as itself, an Item is hosted in a window sized to it on the
    // theme background, so an agent can render its own view with the kit and look at it.
    QQmlComponent component(&engine, QUrl::fromLocalFile(QFileInfo(args.value("qml")).absoluteFilePath()));
    QObject *root = component.create();
    if (!root) {
      fprintf(stderr, "%s\n", qPrintable(component.errorString()));
      return 1;
    }
    window = qobject_cast<QQuickWindow *>(root);
    if (!window) {
      auto *item = qobject_cast<QQuickItem *>(root);
      if (!item) return 1;
      window = new QQuickWindow();
      QQmlComponent bg(&engine);
      bg.setData("import QtQuick\nimport Omarchy.Theme as T\nQtObject { property color c: T.Color.background }", QUrl());
      QObject *b = bg.create();
      window->setColor(b ? b->property("c").value<QColor>() : Qt::black);
      item->setParentItem(window->contentItem());
      item->setPosition(QPointF(pad, pad));
      hosted = item;
      auto fit = [window, item, pad] { window->resize(int(item->width()) + pad * 2, int(item->height()) + pad * 2); };
      QObject::connect(item, &QQuickItem::widthChanged, window, fit);
      QObject::connect(item, &QQuickItem::heightChanged, window, fit);
      fit();
      window->show();
    }
  } else {
    engine.load(QUrl("qrc:/gallery/Main.qml"));
    if (engine.rootObjects().isEmpty()) return 1;
    window = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
  }

  if (args.isSet("snap")) {
    const QString path = args.value("snap");
    // Let layout settle and a few frames render before grabbing.
    QTimer::singleShot(400, &app, [&] {
      if (!hosted) window->setWidth(gallery.width);
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
  const int rc = app.exec();
  // A hosted item and its window aren't owned by the engine; tear them down while the event
  // loop's dispatcher still exists, so their animations don't restart timers after it.
  if (hosted) {
    delete hosted;
    delete window;
  }
  return rc;
}

#include "main.moc"
