// The KDE / Qt Widgets gallery: every Omarchy UI component as a widget, in the live theme,
// following it live (spec/gallery.md).
//
//   omarchy-widgets-gallery                                   a window, following the desktop
//   omarchy-widgets-gallery --section buttons                 one section
//   omarchy-widgets-gallery --list-sections                   the sections, in order
//   omarchy-widgets-gallery --snap out.png --section buttons --size 760x400 --scale 2
//                                                              render offscreen to a PNG and exit
//   OMARCHY_UI_THEME_DIR=fixtures/themes/nord OMARCHY_UI_ROUNDING=8 omarchy-widgets-gallery ...

#include <QApplication>
#include <QCommandLineParser>
#include <QMap>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <cstdio>

#include "gallery.h"

using namespace Omarchy;

namespace Gallery {

static const char *const kOrder[] = {"buttons", "toggles", "inputs",     "slider", "surfaces", "lists",    "tiles",
                                     "progress", "messages", "menus",    "navigation", "forms",  "pickers",  "data",
                                     "layout",  "timeline", "color",    "composed", "tokens", "stock"};

static QMap<QString, Section> &registry() {
  static QMap<QString, Section> r;
  return r;
}

Register::Register(const char *name, const char *title, Build build) { registry().insert(QLatin1String(name), Section{name, title, build}); }

QList<Section> sections() {
  QList<Section> out;
  for (const char *name : kOrder)
    if (registry().contains(QLatin1String(name))) out << registry().value(QLatin1String(name));
  return out;
}

Row *row(BoxLayout *into, QWidget *parent, int spacing, Qt::Alignment align) {
  auto *w = new QWidget(parent);
  auto *r = new Row(w);
  r->setSpacing(spacing);
  into->addWidget(w, align);
  return r;
}

Column *column(BoxLayout *into, QWidget *parent, int spacing, Qt::Alignment align) {
  auto *w = new QWidget(parent);
  auto *c = new Column(w);
  c->setSpacing(spacing);
  into->addWidget(w, align);
  return c;
}

QString glyph(uint cp) { return QString::fromUcs4(reinterpret_cast<const char32_t *>(&cp), 1); }

namespace {
class Painted : public QWidget {
public:
  Painted(QSize size, std::function<void(QPainter &, const QRect &)> paint, QWidget *parent) : QWidget(parent), m_paint(std::move(paint)) {
    setFixedSize(size);
    follow(this);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    m_paint(p, rect());
  }

private:
  std::function<void(QPainter &, const QRect &)> m_paint;
};
}  // namespace

QWidget *painted(QSize size, std::function<void(QPainter &, const QRect &)> paint, QWidget *parent) { return new Painted(size, std::move(paint), parent); }

int sp(const char *name) { return shown().spacing(name); }

}  // namespace Gallery

// The window: the theme background, the sections panelPadding inside it.
class Page : public QWidget {
public:
  explicit Page(const QString &only) {
    follow(this);
    auto *col = new Column(this);
    const int pad = Gallery::sp("panelPadding");
    col->setContentsMargins(pad, pad, pad, pad);
    col->setSpacing(Gallery::sp("huge") * 2);
    for (const Gallery::Section &s : Gallery::sections()) {
      if (!only.isEmpty() && only != QLatin1String(s.name)) continue;
      auto *section = new QWidget(this);
      setFill(section);
      auto *sc = new Column(section);
      sc->setSpacing(Gallery::sp("lg"));
      auto *header = new SectionHeader(QString::fromUtf8(s.title), section);
      sc->addWidget(header);
      auto *body = new QWidget(section);
      setFill(body);
      auto *bc = new Column(body);
      bc->setSpacing(Gallery::sp("xxl"));
      s.build(bc, body);
      sc->addWidget(body);
      col->addWidget(section);
    }
    col->addStretch();
  }

protected:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.fillRect(rect(), shown().background());
  }
};

static const char *argValue(int argc, char **argv, const char *flag) {
  for (int i = 1; i + 1 < argc; ++i)
    if (qstrcmp(argv[i], flag) == 0) return argv[i + 1];
  return nullptr;
}

int main(int argc, char **argv) {
  const bool snap = argValue(argc, argv, "--snap");
  bool list = false;
  for (int i = 1; i < argc; ++i) list |= qstrcmp(argv[i], "--list-sections") == 0;
  if (snap || list) qputenv("QT_QPA_PLATFORM", "offscreen");
  // The device pixel ratio has to be set before the application exists.
  if (snap) qputenv("QT_SCALE_FACTOR", argValue(argc, argv, "--scale") ? argValue(argc, argv, "--scale") : "2");
  if (!qEnvironmentVariableIsSet("QT_FORCE_STDERR_LOGGING")) qputenv("QT_FORCE_STDERR_LOGGING", "1");

  QApplication app(argc, argv);
  app.setApplicationName(QStringLiteral("omarchy-widgets-gallery"));
  app.setDesktopFileName(QStringLiteral("omarchy-widgets-gallery"));

  QCommandLineParser args;
  args.addHelpOption();
  args.addOption({QStringLiteral("section"), QStringLiteral("Show only this section."), QStringLiteral("name")});
  args.addOption({QStringLiteral("list-sections"), QStringLiteral("Print the sections, one per line, in order, and exit.")});
  args.addOption({QStringLiteral("snap"), QStringLiteral("Render offscreen to this PNG and exit."), QStringLiteral("path")});
  args.addOption({QStringLiteral("size"), QStringLiteral("Window size in points for --snap (default 760x400)."), QStringLiteral("WxH"), QStringLiteral("760x400")});
  args.addOption({QStringLiteral("scale"), QStringLiteral("Device pixel ratio for --snap (default 2)."), QStringLiteral("n"), QStringLiteral("2")});
  args.process(app);

  if (args.isSet(QStringLiteral("list-sections"))) {
    for (const Gallery::Section &s : Gallery::sections()) printf("%s\n", s.name);
    return 0;
  }

  Omarchy::Widgets::apply(app);

  const QString only = args.value(QStringLiteral("section"));
  auto *page = new Page(only);
  // Nothing has keyboard focus at first, as in the Qt Quick gallery: the page holds it.
  page->setFocusPolicy(Qt::StrongFocus);
  QScrollArea window;
  window.setWindowTitle(QStringLiteral("Omarchy UI · KDE"));
  window.setFrameShape(QFrame::NoFrame);
  window.setWidgetResizable(true);
  window.setWidget(page);
  window.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  if (args.isSet(QStringLiteral("snap"))) {
    const QStringList size = args.value(QStringLiteral("size")).split(QLatin1Char('x'));
    const int w = size.value(0).toInt(), h = size.value(1).toInt();
    window.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    window.resize(w > 0 ? w : 760, h > 0 ? h : 400);
    window.show();
    page->setFocus();
    const QString path = args.value(QStringLiteral("snap"));
    QTimer::singleShot(300, &app, [&] {
      const QPixmap img = window.grab();
      if (img.isNull() || !img.save(path)) {
        fprintf(stderr, "could not write %s\n", qPrintable(path));
        app.exit(1);
        return;
      }
      app.exit(0);
    });
    return app.exec();
  }

  window.resize(760, qMin(900, qMax(200, page->sizeHint().height())));
  window.show();
  page->setFocus();
  return app.exec();
}
