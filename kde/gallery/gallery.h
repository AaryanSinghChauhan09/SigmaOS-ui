#pragma once

// The gallery's sections (spec/gallery.md): each builds its specimens into `body`, a Column
// xxl apart, under the section header the gallery draws. One file per section in sections/.

#include <Omarchy/Widgets.h>

namespace Gallery {

using Build = void (*)(Omarchy::Column *body, QWidget *parent);

struct Section {
  const char *name;
  const char *title;
  Build build;
};

// Every registered section, in spec/gallery.md's order (main.cpp has the order). A section file
// registers itself: `static Gallery::Register reg("buttons", "Buttons", build);`
QList<Section> sections();

struct Register {
  Register(const char *name, const char *title, Build build);
};

// A row of specimens `spacing` apart, added to `into`.
Omarchy::Row *row(Omarchy::BoxLayout *into, QWidget *parent, int spacing, Qt::Alignment align = {});
// A nested column.
Omarchy::Column *column(Omarchy::BoxLayout *into, QWidget *parent, int spacing, Qt::Alignment align = {});
// A Nerd Font glyph by codepoint.
QString glyph(uint cp);
// Spacing and font tokens now.
int sp(const char *name);

}  // namespace Gallery
