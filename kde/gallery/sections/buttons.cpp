#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  Row *r = Gallery::row(body, parent, Gallery::sp("controlGap"));
  r->addWidget(new Button(QStringLiteral("Plain")));
  auto *bordered = new Button(QStringLiteral("Bordered"));
  bordered->setBordered(true);
  r->addWidget(bordered);
  auto *hover = new Button(QStringLiteral("Hover"));
  hover->setBordered(true);
  hover->setHasCursor(true);
  r->addWidget(hover);
  auto *selected = new Button(QStringLiteral("Selected"));
  selected->setBordered(true);
  selected->setSelected(true);
  r->addWidget(selected);
  auto *icon = new Button(QStringLiteral("Icon"));
  icon->setIconText(Gallery::glyph(0xF013));
  icon->setBordered(true);
  r->addWidget(icon);
  auto *power = new IconButton(Gallery::glyph(0xF011));
  power->setBordered(true);
  r->addWidget(power);
  auto *refresh = new IconButton(Gallery::glyph(0xF021));
  refresh->setBordered(true);
  refresh->setHasCursor(true);
  r->addWidget(refresh);

  auto *group = new ButtonGroup(parent);
  group->setOptions({{QStringLiteral("left"), QStringLiteral("Left"), {}, {}},
                     {QStringLiteral("center"), QStringLiteral("Center"), {}, {}},
                     {QStringLiteral("right"), QStringLiteral("Right"), {}, {}}});
  group->setValue(QStringLiteral("center"));
  body->addWidget(group);
}

Gallery::Register reg("buttons", "Buttons", build);

}  // namespace
