#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  // A fixed color, not the theme's accent, so the picker's content is the same in every theme.
  auto *picker = new ColorPicker(parent);
  picker->setFixedSize(300, 260);
  picker->setColor(QStringLiteral("#7aa2f7"));
  body->addWidget(picker);
}

Gallery::Register reg("color", "Color", build);

}  // namespace
