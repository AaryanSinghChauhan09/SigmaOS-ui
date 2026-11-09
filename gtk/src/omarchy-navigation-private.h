/* omarchy-navigation-private.h: what the navigation files share (Menu, Keycap, Tabs, IconRail),
 * private to the library. */
#pragma once

#include "omarchy-private.h"

G_BEGIN_DECLS

/* The middle of `text`'s ink, from the layout's origin, measured a hundred times larger so it
 * stays on the outline (Qt's OpticalGlyph). */
double omarchy_nav_ink_center (GtkWidget *w, const char *text, int px, gboolean bold);
/* Draws `text` centered by its ink across [x, x + width], and on its line box rounded to whole
 * points down [y, y + height]: Qt's OpticalGlyph with no paintedSize. */
void omarchy_nav_draw_glyph (GtkWidget *w, GtkSnapshot *s, const char *text, int px, float x, float y, float width,
                             float height, const GdkRGBA *color);
/* Where an `inner`-tall item centered in an `outer`-tall one starts: Qt's anchors with
 * alignWhenCentered (an odd size centers on its upper middle pixel). */
float omarchy_nav_center (float outer, float inner);
/* The layout's logical width, unrounded. */
double omarchy_nav_text_advance (GtkWidget *w, const char *text, int px, gboolean bold);
/* The layout's logical width, rounded up (Qt's Math.ceil(implicitWidth)). */
int omarchy_nav_text_width (GtkWidget *w, const char *text, int px, gboolean bold);
/* A value sliding to `to` over motion.press on the easeOut curve (none under reduced motion). */
typedef struct {
  double from, to, value;
  gint64 start;
  guint tick;
  gboolean set;
} OmarchyNavSlide;
void omarchy_nav_slide (GtkWidget *w, OmarchyNavSlide *slide, double to);
void omarchy_nav_slide_stop (GtkWidget *w, OmarchyNavSlide *slide);

G_END_DECLS
