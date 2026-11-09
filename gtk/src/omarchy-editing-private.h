/* omarchy-editing-private.h: what the editing components share (spec/components/editing.md). */
#pragma once

#include "omarchy-private.h"

G_BEGIN_DECLS

/* JS's Math.round (half up), so positions land on the pixels Qt's land on. */
double omarchy_editing_round (double x);
/* "Hot accent": the color with a quarter of the foreground mixed in (Qt.tint(c, alpha(fg, .25))). */
GdkRGBA omarchy_editing_hot (const GdkRGBA *c);
/* The Slider's knob: max(14, round(controlHeight × 0.38)). */
int omarchy_editing_knob (void);
/* A Slider knob centered on (cx, cy): `fill`, ringed max(1, space(2)) in `ring`, 1.15× when hot. */
void omarchy_editing_draw_knob (GtkSnapshot *s, float cx, float cy, gboolean hot, const GdkRGBA *fill,
                                const GdkRGBA *ring);
/* The background of a named surface: `popups` (default), `background`, `tooltip`, `menu`. */
const GdkRGBA *omarchy_editing_surface (const OmarchyTokens *p, const char *surface);

/* A drag that reports travel in the window's coordinates (Qt's mapToItem(null)), so a handle
 * the owner moves under the pointer doesn't feed back. */
typedef struct {
  gboolean active;
  double x0, y0;           /* the press, in the widget */
  graphene_point_t origin; /* the press, in the root */
} OmarchyEditingDrag;
void omarchy_editing_drag_begin (OmarchyEditingDrag *d, GtkWidget *w, double x, double y);
/* The travel since the press. */
void omarchy_editing_drag_travel (OmarchyEditingDrag *d, GtkWidget *w, double dx, double dy, double *tx, double *ty);
/* A drag gesture on `w` (any button) calling begin(x, y), update(dx, dy) and end(dx, dy). */
typedef void (*OmarchyDragFunc) (GtkWidget *w, double x, double y);
GtkGesture *omarchy_editing_add_drag (GtkWidget *w, OmarchyDragFunc begin, OmarchyDragFunc update, OmarchyDragFunc end);

G_END_DECLS
