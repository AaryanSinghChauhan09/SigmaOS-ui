#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  const int huge = Gallery::sp("huge");

  // A Card, and beside it a tooltip as it draws and a ConfirmDialog in a 340 × 170 box.
  Row *top = Gallery::row(body, parent, huge, Qt::AlignTop);
  auto *card = new Card(QStringLiteral("Network"));
  card->setFixedWidth(300);
  auto *wifi = new Toggle(QStringLiteral("Wi-Fi"), QStringLiteral("Basecamp"));
  wifi->setChecked(true);
  setFill(wifi);
  card->addWidget(wifi);
  card->addWidget(new Separator);
  auto *rows = new QWidget;
  setFill(rows);
  auto *rc = new Column(rows);
  auto *basecamp = new MenuRow(QStringLiteral("Basecamp"));
  basecamp->setChecked(true);
  rc->addWidget(basecamp);
  auto *guest = new MenuRow(QStringLiteral("Basecamp Guest"));
  guest->setHighlighted(true);
  rc->addWidget(guest);
  rc->addWidget(new MenuRow(QStringLiteral("Coffee Shop")));
  card->addWidget(rows);
  top->addWidget(card, Qt::AlignTop);

  Column *side = Gallery::column(top, parent, huge, Qt::AlignTop);
  side->addWidget(new TooltipFrame(QStringLiteral("Restart Wi-Fi")));
  auto *box = new QWidget;
  box->setFixedSize(340, 170);
  auto *dialog = new ConfirmDialog(QStringLiteral("Remove the Coffee Shop network?"), box);
  dialog->setGeometry(box->rect());
  side->addWidget(box);

  // Panes: a side panel with a rule against its neighbour, and a bordered settings group.
  Row *panes = Gallery::row(body, parent, huge, Qt::AlignTop);
  auto *layers = new Pane(QStringLiteral("Layers"), QStringLiteral("3 items"));
  layers->setDivider(QStringLiteral("left"));
  layers->addAction(new IconButton(Gallery::glyph(0xF067)));
  layers->setFixedWidth(240);
  auto *lrows = new QWidget;
  setFill(lrows);
  auto *lc = new Column(lrows);
  lc->addWidget(new MenuRow(QStringLiteral("Background")));
  auto *title = new MenuRow(QStringLiteral("Title"));
  title->setChecked(true);
  lc->addWidget(title);
  lc->addWidget(new MenuRow(QStringLiteral("Logo")));
  layers->addWidget(lrows);
  panes->addWidget(layers, Qt::AlignTop);

  auto *night = new Pane(QStringLiteral("Night light"), QStringLiteral("Sunset to sunrise"));
  night->setBordered(true);
  auto *nightSwitch = new ToggleSwitch;
  nightSwitch->setChecked(true);
  night->addAction(nightSwitch);
  night->setFixedWidth(240);
  auto *strength = new SettingRow(QStringLiteral("Strength"));
  setFill(strength);
  auto *slider = new Slider;
  slider->setValue(0.6);
  slider->setFixedWidth(100);
  strength->addControl(slider);
  night->addWidget(strength);
  panes->addWidget(night, Qt::AlignTop);

  // Headings: the three levels, a trailing action, and a toolbar's centered title.
  Row *headings = Gallery::row(body, parent, huge, Qt::AlignTop);
  Column *left = Gallery::column(headings, parent, huge, Qt::AlignTop);
  left->parentWidget()->setFixedWidth(240);
  auto *settings = new Heading(QStringLiteral("Settings"), QStringLiteral("Omarchy"));
  settings->setLevel(QStringLiteral("display"));
  setFill(settings);
  left->addWidget(settings);
  auto *appearance = new Heading(QStringLiteral("Appearance"), QStringLiteral("Theme, font and rounding"));
  appearance->setLevel(QStringLiteral("heading"));
  setFill(appearance);
  left->addWidget(appearance);
  Column *right = Gallery::column(headings, parent, huge, Qt::AlignTop);
  right->parentWidget()->setFixedWidth(240);
  auto *frame = new Heading(QStringLiteral("Frame"));
  frame->addTrailing(new IconButton(Gallery::glyph(0xF141)));
  setFill(frame);
  right->addWidget(frame);
  auto *notes = new Heading(QStringLiteral("notes.md"), QStringLiteral("~/Documents"));
  notes->setCentered(true);
  setFill(notes);
  right->addWidget(notes);
}

Gallery::Register reg("surfaces", "Surfaces", build);

}  // namespace
