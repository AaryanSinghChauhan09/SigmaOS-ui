#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  // A menu and a context menu held open, and a tooltip with its shortcut, huge apart.
  Row *top = Gallery::row(body, parent, Gallery::sp("huge"), Qt::AlignTop);
  auto *menu = new Menu;
  menu->setHeld(true);
  menu->addSection(QStringLiteral("File"));
  menu->addItem(QStringLiteral("New"), QStringLiteral("Ctrl+N"), Gallery::glyph(0xF15B));
  menu->addItem(QStringLiteral("Open…"), QStringLiteral("Ctrl+O"), Gallery::glyph(0xF07B))->setHasCursor(true);
  menu->addItem(QStringLiteral("Save"), QStringLiteral("Ctrl+S"), Gallery::glyph(0xF0C7));
  menu->addItem(QStringLiteral("Duplicate"), QString(), Gallery::glyph(0xF0C5))->setEnabled(false);
  menu->addSeparator();
  menu->addSection(QStringLiteral("View"));
  MenuItem *sidebar = menu->addItem(QStringLiteral("Sidebar"));
  sidebar->setCheckable(true);
  sidebar->setChecked(true);
  menu->addItem(QStringLiteral("Word wrap"))->setCheckable(true);
  menu->addSeparator();
  menu->addItem(QStringLiteral("Export"), QString(), Gallery::glyph(0xF064))->setHasSubmenu(true);
  top->addWidget(menu, Qt::AlignTop);

  auto *context = new ContextMenu;
  context->setHeld(true);
  context->addItem(QStringLiteral("Cut"), QStringLiteral("Ctrl+X"));
  context->addItem(QStringLiteral("Copy"), QStringLiteral("Ctrl+C"));
  context->addItem(QStringLiteral("Paste"), QStringLiteral("Ctrl+V"));
  context->addSeparator();
  for (const auto &[label, on] : {std::pair{QStringLiteral("By name"), true}, std::pair{QStringLiteral("By date"), false}}) {
    MenuItem *radio = context->addItem(label);
    radio->setCheckable(true);
    radio->setExclusive(true);
    radio->setChecked(on);
  }
  context->addSeparator();
  context->addItem(QStringLiteral("Delete"), QStringLiteral("Del"))->setDestructive(true);
  top->addWidget(context, Qt::AlignTop);

  auto *tip = new Tooltip(QStringLiteral("Save"));
  tip->setShortcut(QStringLiteral("Ctrl+S"));
  tip->setHeld(true);
  top->addWidget(tip, Qt::AlignTop);

  // Keycaps lg apart, then a combination.
  Row *keys = Gallery::row(body, parent, Gallery::sp("lg"));
  for (const char *k : {"Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K"})
    keys->addWidget(new Keycap(QString::fromLatin1(k)));
  keys->addWidget(new KeyCombo(QStringLiteral("Ctrl+Shift+P")));

  auto *sheet = new ShortcutSheet;
  sheet->addGroup(QStringLiteral("File"), {{QStringLiteral("Ctrl+N"), QStringLiteral("New file")},
                                           {QStringLiteral("Ctrl+O"), QStringLiteral("Open")},
                                           {QStringLiteral("Ctrl+S"), QStringLiteral("Save")}});
  sheet->addGroup(QStringLiteral("View"), {{QStringLiteral("Ctrl+B"), QStringLiteral("Sidebar")},
                                           {QStringLiteral("Ctrl+Shift+P"), QStringLiteral("Commands")},
                                           {QStringLiteral("F11"), QStringLiteral("Full screen")}});
  body->addWidget(sheet);
}

Gallery::Register reg("menus", "Menus", build);

}  // namespace
