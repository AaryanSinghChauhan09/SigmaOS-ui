#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  // Swatches: 64 × 32 with the caption under them, lg apart (a Flow).
  Row *swatches = Gallery::row(body, parent, Gallery::sp("lg"));
  const char *names[] = {"foreground", "background", "accent", "urgent", "muted"};
  for (const char *name : names) {
    const QString n = QString::fromLatin1(name);
    const QFont caption = shown().font("caption");
    const qreal tw = Paint::textWidth(caption, n);
    const qreal lh = Paint::lineHeight(caption);
    const int w = qMax(64, int(std::ceil(tw))), h = int(std::ceil(32 + Gallery::sp("xs") + lh));
    swatches->addWidget(Gallery::painted(QSize(w, h), [n](QPainter &p, const QRect &) {
      const Tokens &t = shown();
      Paint::surface(p, QRectF(0, 0, 64, 32), t.smallRadius(), t.color(QStringLiteral("palette.") + n), Border::flat(t.border(State::Normal).color, 1));
      Paint::text(p, QPointF(0, 32 + t.spacing("xs")), t.font("caption"), t.foreground(), n);
    }), Qt::AlignTop);
  }

  // State chips: 120 × controlHeight.
  Row *chips = Gallery::row(body, parent, Gallery::sp("lg"));
  const char *states[] = {"normal", "hover-cursor", "focus", "selected"};
  for (const char *name : states) {
    const QString n = QString::fromLatin1(name);
    chips->addWidget(Gallery::painted(QSize(120, Gallery::sp("controlHeight")), [n](QPainter &p, const QRect &r) {
      const Tokens &t = shown();
      const State s = n == QLatin1String("normal") ? State::Normal : n == QLatin1String("focus") ? State::Focus : n == QLatin1String("selected") ? State::Selected : State::Hover;
      Paint::surface(p, QRectF(r), t.radius(), t.fill(s), t.border(s));
      const bool sel = s == State::Selected;
      const QFont f = t.font("body", sel);
      const QSizeF ts = Paint::textSize(f, n);
      Paint::text(p, QPointF(Paint::centered(r.width(), ts.width()), Paint::centered(r.height(), ts.height())), f, sel ? t.selectedText() : t.foreground(), n);
    }));
  }

  // The type scale, xs apart.
  Column *scale = Gallery::column(body, parent, Gallery::sp("xs"));
  const char *sizes[] = {"displayLarge", "display", "heading", "title", "subtitle", "body", "bodySmall", "caption"};
  for (const char *name : sizes) {
    auto *l = new Label(QString::fromLatin1(name) + QLatin1Char(' ') + QString::number(shown().fontSize(name)), name);
    scale->addWidget(l);
  }
}

Gallery::Register reg("tokens", "Tokens", build);

}  // namespace
