#include "gallery.h"

using namespace Omarchy;

// Real UIs built only from kit components and tokens: the test that the kit is complete. No
// painting of its own, only layout (qt/gallery/sections/composed.qml).

namespace {

QString g(uint cp) { return Gallery::glyph(cp); }

ListRow *network(uint glyph, const QString &title, const QString &subtitle = QString()) {
  auto *r = new ListRow(g(glyph), title);
  r->setSubtitle(subtitle);
  return r;
}

// A widget holding a column of setting rows with no gap, as a QML Column.
QWidget *settings(const QList<SettingRow *> &rows) {
  auto *w = new QWidget;
  setFill(w);
  auto *c = new Column(w);
  c->setSpacing(0);
  for (SettingRow *r : rows) {
    setFill(r);
    c->addWidget(r);
  }
  return w;
}

// A Wi-Fi panel, as the shell's network panel.
QWidget *wifiPanel() {
  auto *card = new Card;
  card->setFixedWidth(360);

  auto *hero = new Hero(g(0xF0928), QStringLiteral("Wi-Fi"));
  hero->setStatus(QStringLiteral("Connected to Basecamp"));
  auto *on = new ToggleSwitch;
  on->setChecked(true);
  hero->setTrailing(on);
  setFill(hero);
  card->addWidget(hero);

  auto *header = new SectionHeader(QStringLiteral("Networks"));
  setFill(header);
  card->addWidget(header);

  auto *scroll = new ScrollArea;
  scroll->setMaxHeight(280);
  scroll->setSpacing(Gallery::sp("xxs"));
  setFill(scroll);

  ListRow *basecamp = network(0xF0928, QStringLiteral("Basecamp"));
  basecamp->setStatus(QStringLiteral("Connected"), QStringLiteral("success"));
  basecamp->setCurrent(true);
  scroll->addWidget(basecamp);

  ListRow *guest = network(0xF0925, QStringLiteral("Basecamp Guest"), QStringLiteral("Open"));
  guest->setActionGlyph(g(0xF0159), true);
  guest->setHasCursor(true);
  scroll->addWidget(guest);

  ListRow *coffee = network(0xF0922, QStringLiteral("Coffee Shop"), QStringLiteral("WPA2"));
  coffee->setTrailingGlyph(g(0xF033E));
  auto *expansion = new QWidget;
  auto *er = new Row(expansion);
  er->setSpacing(Gallery::sp("md"));
  auto *password = new PasswordField;
  password->setPlaceholderText(QStringLiteral("Password"));
  setFill(password);
  er->addWidget(password);
  auto *connect = new Button(QStringLiteral("Connect"));
  connect->setBordered(true);
  er->addWidget(connect);
  coffee->setExpansion(expansion);
  scroll->addWidget(coffee);

  ListRow *library = network(0xF091F, QStringLiteral("Library"), QStringLiteral("Saved"));
  library->setTrailingGlyph(g(0xF033E));
  library->setActionGlyph(g(0xF0159), true);
  scroll->addWidget(library);
  card->addWidget(scroll);

  auto *rule = new Separator;
  setFill(rule);
  card->addWidget(rule);

  auto *details = new KeyValue;
  details->setPairs({
      {QStringLiteral("IP"), QStringLiteral("192.168.1.24"), QString()},
      {QStringLiteral("Gateway"), QStringLiteral("192.168.1.1"), QString()},
      {QStringLiteral("DNS"), QStringLiteral("1.1.1.1"), QString()},
  });
  card->addWidget(details);
  return card;
}

// An app inspector, as omareel's side panel: a pane on the window, ruled off from the canvas.
QWidget *inspector() {
  auto *pane = new Pane(QStringLiteral("Recording 1"), QStringLiteral("1920 × 1080 · 0:42"));
  pane->setFixedWidth(300);
  pane->setDivider(QStringLiteral("left"));
  pane->addAction(new IconButton(g(0xF0E2)));
  pane->addAction(new IconButton(g(0xF01E)));
  auto *exportButton = new Button(QStringLiteral("Export"));
  exportButton->setBordered(true);
  pane->addAction(exportButton);

  auto *tabs = new Tabs;
  tabs->setModel({QStringLiteral("Shape"), QStringLiteral("Cursor"), QStringLiteral("Zoom")});
  tabs->setCurrentIndex(0);
  setFill(tabs);
  pane->addWidget(tabs);

  auto *frame = new SectionHeader(QStringLiteral("Frame"));
  setFill(frame);
  pane->addWidget(frame);

  auto *radius = new SettingRow(QStringLiteral("Radius"));
  auto *slider = new Slider;
  slider->setFixedWidth(110);
  slider->setValue(0.4);
  radius->addControl(slider);
  auto *border = new SettingRow(QStringLiteral("Border"));
  auto *dropdown = new Dropdown;
  dropdown->setFixedWidth(110);
  dropdown->setShowLabel(false);
  dropdown->setOptions({{QStringLiteral("solid"), QStringLiteral("Solid")}, {QStringLiteral("none"), QStringLiteral("None")}});
  dropdown->setValue(QStringLiteral("solid"));
  border->addControl(dropdown);
  auto *shadow = new SettingRow(QStringLiteral("Shadow"));
  shadow->setHasCursor(true);
  auto *shadowSwitch = new ToggleSwitch;
  shadowSwitch->setChecked(true);
  shadow->addControl(shadowSwitch);
  pane->addWidget(settings({radius, border, shadow}));

  auto *background = new SectionHeader(QStringLiteral("Background"));
  setFill(background);
  pane->addWidget(background);

  auto *fill = new SettingRow(QStringLiteral("Fill"));
  fill->addControl(new ColorWell);
  auto *padding = new SettingRow(QStringLiteral("Padding"));
  auto *number = new NumberField;
  number->setRange(0, 96);
  number->setValue(24);
  padding->addControl(number);
  pane->addWidget(settings({fill, padding}));
  return pane;
}

const QList<SearchItem> &searchItems() {
  static const QList<SearchItem> items = {
      {QStringLiteral("Terminal"), QStringLiteral("App"), g(0xF120), QString(), QStringLiteral("Open a new terminal window.")},
      {QStringLiteral("Terminal font"), QStringLiteral("Style"), g(0xF031), QString(), QStringLiteral("Pick the monospace font every terminal uses.")},
      {QStringLiteral("Terminal theme"), QStringLiteral("Style"), g(0xF1FC), QString(),
       QStringLiteral("Match the terminal colors to the desktop theme.")},
      {QStringLiteral("Intermediate CA"), QStringLiteral("Certificate"), g(0xF0A3), QString(),
       QStringLiteral("Trust a certificate authority for this machine.")},
  };
  return items;
}

void build(Column *body, QWidget *parent) {
  const int gap = Gallery::sp("huge") * 2;

  Row *top = Gallery::row(body, parent, gap);
  top->addWidget(wifiPanel(), Qt::AlignTop);
  top->addWidget(inspector(), Qt::AlignTop);

  Row *bottom = Gallery::row(body, parent, gap);

  // A type-ahead overlay with "term" typed: four results, the second under the cursor, and the
  // detail pane.
  auto *results = new SearchList;
  results->setFixedSize(360, 300);
  results->setQuery(QStringLiteral("term"));
  results->setItems(searchItems());
  results->setShowDetail(true);
  results->cursor()->set(QStringLiteral("results"), 1);
  bottom->addWidget(results, Qt::AlignTop);

  // Nothing matches "zzz".
  auto *none = new SearchList;
  none->setFixedSize(300, 200);
  none->setQuery(QStringLiteral("zzz"));
  QList<SearchItem> matching;
  for (const SearchItem &i : searchItems())
    if (SearchList::matches(i.title, QStringLiteral("zzz"))) matching << i;
  none->setItems(matching);
  bottom->addWidget(none, Qt::AlignTop);
}

Gallery::Register reg("composed", "Composed", build);

}  // namespace
