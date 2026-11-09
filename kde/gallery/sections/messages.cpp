#include "gallery.h"

using namespace Omarchy;

namespace {

// A 350 × 150 box with the empty state centered in it at the box's width (anchors.centerIn).
QWidget *boxed(EmptyState *state) {
  auto *box = new QWidget;
  box->setFixedSize(350, 150);
  state->setParent(box);
  const qreal h = state->implicitHeightFor(350);
  state->setGeometryF(QRectF(0, Paint::centered(150, h), 350, h));
  return box;
}

void build(Column *body, QWidget *parent) {
  const int huge = Gallery::sp("huge");

  Row *empty = Gallery::row(body, parent, huge);
  auto *networks = new EmptyState(QStringLiteral("No networks found"));
  networks->setGlyph(Gallery::glyph(0xF05AA));
  networks->setHint(QStringLiteral("Turn on Wi-Fi to scan"));
  networks->setActionText(QStringLiteral("Scan"));
  empty->addWidget(boxed(networks));
  auto *clipboard = new EmptyState(QStringLiteral("Clipboard is empty"));
  clipboard->setSize(QStringLiteral("overlay"));
  clipboard->setGlyph(Gallery::glyph(0xF014C));
  clipboard->setHint(QStringLiteral("Copy something to see it here"));
  empty->addWidget(boxed(clipboard));

  struct Spec {
    const char *tone, *text, *action;
  };
  for (const Spec &s : {Spec{"info", "Omarchy 3.2 is available", "Update"}, Spec{"success", "Backup finished", ""},
                        Spec{"warning", "Battery is low (12%)", ""}, Spec{"error", "Could not reach the update server", ""}}) {
    auto *b = new Banner(QString::fromUtf8(s.text), parent);
    b->setTone(QLatin1String(s.tone));
    b->setActionText(QString::fromUtf8(s.action));
    b->setDismissible(true);
    setFill(b);
    body->addWidget(b);
  }

  // A toast as it draws, held open (the app owns when it shows and the timer).
  auto *toast = new Toast(QStringLiteral("Export finished"), parent);
  toast->setGlyph(Gallery::glyph(0xF012C));
  toast->setText(QStringLiteral("omarchy-demo.mp4 · 42 MB"));
  toast->setActionText(QStringLiteral("Open"));
  toast->setCountdown(0.6);
  body->addWidget(toast);
}

Gallery::Register reg("messages", "Messages", build);

}  // namespace
