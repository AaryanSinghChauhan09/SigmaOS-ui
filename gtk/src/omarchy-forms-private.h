/* omarchy-forms-private.h: what the forms files share. */
#pragma once

#include "omarchy-private.h"

G_BEGIN_DECLS

/* A bold ✓ `round(box × 0.85)` px, centered on `box` (Checkbox, MultiSelect rows). */
void omarchy_forms_draw_check (GtkWidget *w, GtkSnapshot *s, const graphene_rect_t *box, const GdkRGBA *color);
/* Text's height as Qt boxes it: one line is the font's line box, more are the layout's. */
int omarchy_forms_text_height (PangoLayout *l, int px);
/* Qt's Easing.OutCubic / OutQuad / InOutQuad. */
double omarchy_forms_out_cubic (double t);
double omarchy_forms_out_quad (double t);
double omarchy_forms_in_out_quad (double t);
/* Emit a widget's `hovered(bool)` signal on pointer enter and leave. */
void omarchy_forms_add_hover_signal (GtkWidget *w, guint signal_id);

G_END_DECLS
