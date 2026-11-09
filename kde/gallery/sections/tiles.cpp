#include <QCoreApplication>

#include "gallery.h"

using namespace Omarchy;

namespace {

// The sample picture, the Qt gallery's qt/gallery/assets/avatar.png, built into the gallery.
QString photo() { return QStringLiteral(":/gallery/avatar.png"); }

Avatar *avatar(const QString &name, Avatar::Size size = Avatar::Medium) {
  auto *a = new Avatar(name);
  a->setSize(size);
  return a;
}

void build(Column *body, QWidget *parent) {
  Row *badges = Gallery::row(body, parent, Gallery::sp("controlGap"));
  badges->addWidget(new Badge(QStringLiteral("New")));
  badges->addWidget(new Badge(QStringLiteral("3"), QStringLiteral("accent"), true));
  badges->addWidget(new Badge(QStringLiteral("12"), QStringLiteral("urgent"), true));
  badges->addWidget(new Badge(QStringLiteral("Online"), QStringLiteral("success")));
  badges->addWidget(new Badge(QStringLiteral("Low"), QStringLiteral("warning")));
  badges->addWidget(new Badge(QStringLiteral("Offline"), QStringLiteral("urgent")));

  const int xxl = Gallery::sp("xxl");
  // Docked dots ringed in the window background (read live: it fades with the theme).
  QList<Avatar *> grounded;
  Row *dots = Gallery::row(body, parent, xxl);
  for (const char *tone : {"success", "warning", "urgent", "muted", "accent"}) dots->addWidget(new StatusDot(QLatin1String(tone)));
  Avatar *a = avatar(QStringLiteral("Ryan"));
  a->setRound(true);
  a->setStatus(QStringLiteral("success"));
  grounded << a;
  dots->addWidget(a);
  a = avatar(QStringLiteral("Photo"));
  a->setSource(photo());
  a->setStatus(QStringLiteral("warning"));
  grounded << a;
  dots->addWidget(a);
  a = avatar(QStringLiteral("Server"));
  a->setGlyph(Gallery::glyph(0xF048B));
  a->setStatus(QStringLiteral("urgent"));
  grounded << a;
  dots->addWidget(a);

  auto ground = [grounded] {
    for (Avatar *g : grounded) g->setGround(shown().background());
  };
  ground();
  QObject::connect(Theme::instance(), &Theme::colorsChanged, dots->parentWidget(), ground);

  Row *faces = Gallery::row(body, parent, xxl);
  faces->addWidget(avatar(QStringLiteral("ada"), Avatar::Small));
  faces->addWidget(avatar(QStringLiteral("Basecamp")));
  a = avatar(QStringLiteral("Omarchy"), Avatar::Large);
  a->setRound(true);
  faces->addWidget(a);
  a = avatar(QStringLiteral("Robot"));
  a->setGlyph(Gallery::glyph(0xF06A9));
  faces->addWidget(a);
  a = avatar(QStringLiteral("Photo"), Avatar::Small);
  a->setSource(photo());
  faces->addWidget(a);
  a = avatar(QStringLiteral("Photo"));
  a->setSource(photo());
  a->setRound(true);
  faces->addWidget(a);
  a = avatar(QStringLiteral("Photo"), Avatar::Large);
  a->setSource(photo());
  faces->addWidget(a);
  a = avatar(QStringLiteral("Missing"));
  a->setSource(QStringLiteral("qrc:/gallery/assets/missing.png"));
  faces->addWidget(a);

  // By icon name, from the icon theme (the fixture icon theme under bin/review).
  Row *icons = Gallery::row(body, parent, xxl);
  struct Icon {
    const char *name, *icon;
    Avatar::Size size;
  };
  for (const Icon &i : {Icon{"Files", "org.gnome.Nautilus", Avatar::Medium}, Icon{"Terminal", "utilities-terminal", Avatar::Medium},
                        Icon{"Firefox", "firefox", Avatar::Medium}, Icon{"Chromium", "chromium", Avatar::Small},
                        Icon{"Steam", "steam", Avatar::Large}, Icon{"Btop", "btop", Avatar::Small}}) {
    a = avatar(QLatin1String(i.name), i.size);
    a->setIconName(QLatin1String(i.icon));
    icons->addWidget(a);
  }
  a = avatar(QStringLiteral("Nothing"));
  a->setIconName(QStringLiteral("no-such-app"));
  a->setGlyph(Gallery::glyph(0xF120));
  icons->addWidget(a);

  auto *grid = new TileGrid(parent);
  grid->setColumns(4);
  grid->setCursorIndex(2);
  TileGrid::Tile photoTile{QStringLiteral("Photos"), QString(), true, QStringLiteral("Photos"), photo()};
  grid->setTiles({
      {QStringLiteral("Terminal"), Gallery::glyph(0xF120)},
      {QStringLiteral("Files"), Gallery::glyph(0xF07B)},
      {QStringLiteral("Browser"), Gallery::glyph(0xF0AC)},
      {QStringLiteral("Music"), Gallery::glyph(0xF001)},
      {QStringLiteral("Settings"), Gallery::glyph(0xF013)},
      {QStringLiteral("Calendar"), Gallery::glyph(0xF073)},
      {QStringLiteral("Screen Recorder"), Gallery::glyph(0xF03D)},
      photoTile,
  });
  body->addWidget(grid);
}

Gallery::Register reg("tiles", "Tiles", build);

}  // namespace
