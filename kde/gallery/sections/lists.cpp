#include <QKeyEvent>

#include "gallery.h"

using namespace Omarchy;

namespace {

// Hands the scroll area's keys to the cursor, as the Qt Quick gallery's Keys.onPressed does.
class KeysToCursor : public QObject {
public:
  KeysToCursor(CursorModel *cursor, QObject *parent) : QObject(parent), m_cursor(cursor) {}

protected:
  bool eventFilter(QObject *o, QEvent *e) override {
    if (e->type() == QEvent::KeyPress && m_cursor->handleKey(static_cast<QKeyEvent *>(e))) return true;
    return QObject::eventFilter(o, e);
  }

private:
  CursorModel *m_cursor;
};

ListRow *row(uint glyph, const QString &title, const QString &subtitle = QString()) {
  auto *r = new ListRow(glyph ? Gallery::glyph(glyph) : QString(), title);
  r->setSubtitle(subtitle);
  return r;
}

void build(Column *body, QWidget *parent) {
  // A ScrollArea capped at 340 whose rows a CursorModel drives (the keys and the pointer move it).
  auto *scroll = new ScrollArea(parent);
  scroll->setFixedWidth(420);
  scroll->setMaxHeight(340);
  scroll->setSpacing(Gallery::sp("xxs"));
  scroll->setFocusPolicy(Qt::StrongFocus);
  auto *cursor = new CursorModel(scroll);
  cursor->setSections({{QStringLiteral("rows"), 9}});
  cursor->setReferenceWidget(parent);
  scroll->installEventFilter(new KeysToCursor(cursor, scroll));

  QList<ListRow *> rows;
  ListRow *r = row(0xF0928, QStringLiteral("Coffee Shop"), QStringLiteral("WPA2"));
  r->setValue(QStringLiteral("5 GHz"));
  r->setTrailingGlyph(Gallery::glyph(0xF033E));
  rows << r;
  rows << row(0xF0925, QStringLiteral("Basecamp Guest"), QStringLiteral("Open"));
  r = row(0xF0928, QStringLiteral("Basecamp"));
  r->setStatus(QStringLiteral("Connected"), QStringLiteral("success"));
  r->setCurrent(true);
  rows << r;
  r = row(0xF091F, QStringLiteral("Neighbor 5G"), QStringLiteral("Out of range"));
  r->setEnabled(false);
  rows << r;
  r = row(0, QStringLiteral("Messages"), QStringLiteral("From Basecamp"));
  auto *avatar = new Avatar(QStringLiteral("Messages"));
  avatar->setSize(Avatar::Small);
  r->setLeading(avatar);
  r->setTrailing(new Badge(QStringLiteral("3"), QStringLiteral("accent"), true));
  rows << r;
  r = row(0xF00AF, QStringLiteral("Headphones"), QStringLiteral("Battery 80%"));
  auto *sw = new ToggleSwitch;
  sw->setChecked(true);
  sw->setInteractive(false);
  r->setTrailing(sw);
  rows << r;
  r = row(0xF0922, QStringLiteral("Office"), QStringLiteral("Saved"));
  r->setTrailingGlyph(Gallery::glyph(0xF033E));
  r->setActionGlyph(Gallery::glyph(0xF0159), true);
  r->setActionCursor(true);
  rows << r;
  // Past the cap, so the bar shows.
  r = row(0xF0922, QStringLiteral("Library"), QStringLiteral("WPA2"));
  r->setTrailingGlyph(Gallery::glyph(0xF033E));
  r->setActionGlyph(Gallery::glyph(0xF0159), true);
  rows << r;
  rows << row(0xF091F, QStringLiteral("Airport Free WiFi"), QStringLiteral("Open"));
  for (int i = 0; i < rows.size(); ++i) {
    scroll->addWidget(rows[i]);
    cursor->attach(rows[i], QStringLiteral("rows"), i);
  }
  QObject::connect(cursor, &CursorModel::moved, scroll, [scroll, rows](const QString &, int index, bool byKey) {
    if (byKey) scroll->ensureVisible(rows.value(index));
  });
  cursor->set(QStringLiteral("rows"), 1);
  body->addWidget(scroll);

  // Trailing glyphs that stay: drill-in chevrons and a check.
  Column *stay = Gallery::column(body, parent, Gallery::sp("xxs"));
  stay->parentWidget()->setFixedWidth(420);
  r = row(0xF03D8, QStringLiteral("Appearance"), QStringLiteral("Theme, font, rounding"));
  r->setTrailingGlyph(Gallery::glyph(0xF0142));
  stay->addWidget(r);
  r = row(0xF0379, QStringLiteral("Display"), QStringLiteral("2560 × 1440"));
  r->setTrailingGlyph(Gallery::glyph(0xF0142));
  stay->addWidget(r);
  r = row(0xF02CB, QStringLiteral("Headphones"), QStringLiteral("Output"));
  r->setCurrent(true);
  r->setTrailingGlyph(Gallery::glyph(0xF012C), QStringLiteral("accent"));
  stay->addWidget(r);

  auto *wifi = new Hero(Gallery::glyph(0xF0928), QStringLiteral("Wi-Fi"), parent);
  wifi->setStatus(QStringLiteral("Connected to Basecamp"));
  auto *on = new ToggleSwitch;
  on->setChecked(true);
  wifi->setTrailing(on);
  wifi->setFixedWidth(420);
  body->addWidget(wifi);

  auto *bt = new Hero(Gallery::glyph(0xF00B2), QStringLiteral("Bluetooth"), parent);
  bt->setStatus(QStringLiteral("No adapter found"));
  bt->setError(true);
  bt->setGlyphOpacity(0.5);
  bt->setFixedWidth(420);
  body->addWidget(bt);

  auto *details = new KeyValue(parent);
  details->setPairColumns(2);
  details->setPairs({
      {QStringLiteral("Ping"), QStringLiteral("18 ms"), QString()},
      {QStringLiteral("Packet loss"), QStringLiteral("4%"), QStringLiteral("urgent")},
      {QStringLiteral("Receiving"), QStringLiteral("1.2 MB/s"), QString()},
      {QStringLiteral("Sending"), QStringLiteral("240 KB/s"), QString()},
      {QStringLiteral("IP address"), QStringLiteral("192.168.1.24"), QString()},
      {QStringLiteral("Gateway"), QStringLiteral("192.168.1.1"), QString()},
  });
  body->addWidget(details);
}

Gallery::Register reg("lists", "Lists", build);

}  // namespace
