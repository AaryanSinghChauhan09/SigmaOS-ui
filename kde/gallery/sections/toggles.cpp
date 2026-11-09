#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  Row *r = Gallery::row(body, parent, Gallery::sp("xxl"));
  for (int i = 0; i < 4; ++i) {
    auto *s = new ToggleSwitch;
    s->setChecked(i % 2 == 1);
    s->setHasCursor(i >= 2);
    r->addWidget(s);
  }
  auto *wifi = new Toggle(QStringLiteral("Wi-Fi"), QStringLiteral("Connected to Basecamp"), parent);
  wifi->setChecked(true);
  wifi->setFixedWidth(360);
  body->addWidget(wifi);
  auto *bt = new Toggle(QStringLiteral("Bluetooth"), QStringLiteral("Off"), parent);
  bt->setHasCursor(true);
  bt->setFixedWidth(360);
  body->addWidget(bt);
}

Gallery::Register reg("toggles", "Toggles", build);

}  // namespace
