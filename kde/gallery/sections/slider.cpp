#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  auto *a = new Slider(parent);
  a->setValue(0.35);
  a->setFixedWidth(360);
  body->addWidget(a);
  auto *b = new Slider(parent);
  b->setValue(0.75);
  b->setTickCount(5);
  b->setFixedWidth(360);
  body->addWidget(b);
}

Gallery::Register reg("slider", "Slider", build);

}  // namespace
