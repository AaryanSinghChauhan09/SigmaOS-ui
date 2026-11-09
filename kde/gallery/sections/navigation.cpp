#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  Q_UNUSED(parent);
  // Three groups split by separators, a centered title, a trailing button and the overflow.
  auto *bar = new Toolbar;
  setFill(bar);
  for (uint cp : {0xF032u, 0xF033u, 0xF0CDu}) {
    auto *b = new IconButton(Gallery::glyph(cp));
    b->setHasCursor(cp == 0xF033u);
    bar->addLeading(b);
  }
  bar->addSeparator();
  for (uint cp : {0xF0CAu, 0xF0CBu}) bar->addLeading(new IconButton(Gallery::glyph(cp)));
  bar->addSeparator();
  for (uint cp : {0xF03Eu, 0xF0C1u}) bar->addLeading(new IconButton(Gallery::glyph(cp)));
  auto *title = new Heading(QStringLiteral("notes.md"));
  title->setCentered(true);
  bar->addCenter(title);
  auto *share = new Button(QStringLiteral("Share"));
  share->setBordered(true);
  bar->addTrailing(share);
  bar->setOverflow(true);
  body->addWidget(bar);

  auto *tabs = new Tabs;
  setFill(tabs);
  tabs->setModel({QStringLiteral("General"), QStringLiteral("Display"), QStringLiteral("Audio"), QStringLiteral("Network")});
  tabs->setCurrentIndex(1);
  tabs->setCursorIndex(2);
  body->addWidget(tabs);

  // The rail beside a placeholder for its panel, the selected item's name centered in it.
  auto *row = new QWidget;
  auto *rl = new Row(row);
  auto *rail = new IconRail;
  rail->setModel({{Gallery::glyph(0xF07B), QStringLiteral("Files")},
                  {Gallery::glyph(0xF002), QStringLiteral("Search")},
                  {Gallery::glyph(0xF126), QStringLiteral("Source control")},
                  {Gallery::glyph(0xF188), QStringLiteral("Debug")},
                  {Gallery::glyph(0xF013), QStringLiteral("Settings")}});
  rail->setCurrentIndex(1);
  rl->addWidget(rail);
  auto *panel = Gallery::painted(QSize(shown().space(320), rail->sizeHint().height()), [rail](QPainter &p, const QRect &r) {
    const Tokens &tk = shown();
    const QFont f = tk.font("body");
    const QString name = rail->model().value(rail->currentIndex()).label;
    const QSizeF s = Paint::textSize(f, name);
    Paint::text(p, QPointF(Paint::centered(r.width(), s.width()), Paint::centered(r.height(), s.height())), f, tk.textDim(), name);
  });
  rl->addWidget(panel);
  body->addWidget(row);
}

Gallery::Register reg("navigation", "Navigation", build);

}  // namespace
