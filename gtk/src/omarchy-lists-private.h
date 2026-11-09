/* omarchy-lists-private.h: what the Lists files share. */
#pragma once

#include "omarchy-private.h"

G_BEGIN_DECLS

/* A tone's color on screen: "accent" (or "info"), "success", "warning", "urgent" (or "error"),
 * "muted"; anything else (neutral, "") is `neutral`. */
GdkRGBA omarchy_lists_tone (const char *tone, const GdkRGBA *neutral);
/* TRUE for a tone other than neutral. */
gboolean omarchy_lists_toned (const char *tone);
/* A layout of one line, elided at `width` (no limit when width < 0). */
PangoLayout *omarchy_lists_line (GtkWidget *w, const char *text, int px, gboolean bold, int width);
/* Draw a glyph centered (anchors.centerIn) by its fractional box in (x, y, w, h). */
void omarchy_lists_draw_glyph (GtkSnapshot *s, GtkWidget *w, const char *glyph, int px, float x, float y, float bw,
                               float bh, const GdkRGBA *color);
/* Draw a glyph at x (its box's left edge), y. */
void omarchy_lists_draw_glyph_at (GtkSnapshot *s, GtkWidget *w, const char *glyph, int px, float x, float y,
                                  const GdkRGBA *color);
/* Where Qt's anchors center a child in a parent (alignWhenCentered). */
float omarchy_lists_center (float parent, float child);
/* A glyph's box (advance plus overhang) and line height at `px`. */
void omarchy_lists_glyph_size (GtkWidget *w, const char *glyph, int px, int *width, int *height);
/* The same box at its fractional width (Qt's implicitWidth). */
float omarchy_lists_glyph_width_f (GtkWidget *w, const char *glyph, int px);

static inline gboolean
child_shown (GtkWidget *c)
{
  return c && gtk_widget_get_visible (c);
}

G_END_DECLS
