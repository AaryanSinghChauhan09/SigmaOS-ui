#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  const int huge = Gallery::sp("huge");

  Row *meters = Gallery::row(body, parent, huge);
  for (qreal v : {0.2, 0.65, 0.92}) {
    auto *m = new Meter(v);
    m->setMarkers({0.5, 0.8});
    m->setAlarming(v > 0.9);
    m->setFixedWidth(220);
    meters->addWidget(m);
  }

  auto *battery = new Meter(0.65, parent);
  battery->setLabel(QStringLiteral("Battery"));
  battery->setValueText(QStringLiteral("65%"));
  battery->setFixedWidth(360);
  body->addWidget(battery);

  Row *bars = Gallery::row(body, parent, huge);
  auto *determinate = new ProgressBar(0.4);
  determinate->setFixedWidth(340);
  bars->addWidget(determinate);
  auto *busy = new ProgressBar;
  busy->setIndeterminate(true);
  busy->setFixedWidth(340);
  bars->addWidget(busy);

  Row *spinners = Gallery::row(body, parent, huge);
  for (int i = 0; i < 4; ++i) {
    auto *s = new Spinner;
    s->setPulse(i >= 2);
    if (i % 2) s->setSize(shown().fontSize("iconLarge"));
    spinners->addWidget(s);
  }
}

Gallery::Register reg("progress", "Progress", build);

}  // namespace
