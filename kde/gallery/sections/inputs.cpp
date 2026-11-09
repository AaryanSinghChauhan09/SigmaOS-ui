#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  Row *r = Gallery::row(body, parent, Gallery::sp("controlGap"));
  auto *search = new TextField;
  search->setPlaceholderText(QStringLiteral("Search…"));
  search->setFixedWidth(220);
  r->addWidget(search);
  auto *text = new TextField(QStringLiteral("Omarchy"));
  text->setFixedWidth(220);
  r->addWidget(text);
  auto *hovered = new TextField(QStringLiteral("Hovered"));
  hovered->setHasCursor(true);
  hovered->setFixedWidth(220);
  r->addWidget(hovered);

  Row *r2 = Gallery::row(body, parent, Gallery::sp("xxl"));
  auto *gaps = new NumberField(QStringLiteral("Gaps"));
  gaps->setRange(0, 40);
  gaps->setValue(10);
  r2->addWidget(gaps);
  auto *theme = new Dropdown(QStringLiteral("Theme"));
  theme->setOptions({{QStringLiteral("tokyo-night"), QStringLiteral("Tokyo Night")}, {QStringLiteral("nord"), QStringLiteral("Nord")}});
  theme->setValue(QStringLiteral("tokyo-night"));
  r2->addWidget(theme);
}

Gallery::Register reg("inputs", "Inputs", build);

}  // namespace
