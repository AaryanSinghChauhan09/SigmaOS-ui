/* omarchy-slider.h: Slider (spec/components.md; Qt ValueSlider / PanelSlider). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_SLIDER (omarchy_slider_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySlider, omarchy_slider, OMARCHY, SLIDER, GtkWidget)
GtkWidget *omarchy_slider_new (double value);
double omarchy_slider_get_value (OmarchySlider *self);
void omarchy_slider_set_value (OmarchySlider *self, double value);
void omarchy_slider_set_range (OmarchySlider *self, double minimum, double maximum);
double omarchy_slider_get_minimum (OmarchySlider *self);
double omarchy_slider_get_maximum (OmarchySlider *self);
void omarchy_slider_set_step (OmarchySlider *self, double step);
void omarchy_slider_set_integer (OmarchySlider *self, gboolean integer);
int omarchy_slider_get_tick_count (OmarchySlider *self);
void omarchy_slider_set_tick_count (OmarchySlider *self, int ticks);
gboolean omarchy_slider_get_dragging (OmarchySlider *self);
/* The surface it sits on: `popups` (default), `background`, `tooltip`, `menu`. */
void omarchy_slider_set_surface (OmarchySlider *self, const char *surface);

G_END_DECLS
