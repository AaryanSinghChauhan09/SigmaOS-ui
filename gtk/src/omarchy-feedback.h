/* omarchy-feedback.h: Meter, ProgressBar, Spinner, EmptyState, Banner and Toast
 * (spec/components/feedback.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Meter is derivable so ProgressBar can be what Qt's is: a Meter with an info fill. */
#define OMARCHY_TYPE_METER (omarchy_meter_get_type ())
G_DECLARE_DERIVABLE_TYPE (OmarchyMeter, omarchy_meter, OMARCHY, METER, GtkWidget)

struct _OmarchyMeterClass {
  GtkWidgetClass parent_class;
  /*< private >*/
  gpointer padding[4];
};

GtkWidget *omarchy_meter_new (double value);
double omarchy_meter_get_value (OmarchyMeter *self);
void omarchy_meter_set_value (OmarchyMeter *self, double value);
const char *omarchy_meter_get_tone (OmarchyMeter *self);
void omarchy_meter_set_tone (OmarchyMeter *self, const char *tone);
gboolean omarchy_meter_get_alarming (OmarchyMeter *self);
void omarchy_meter_set_alarming (OmarchyMeter *self, gboolean alarming);
void omarchy_meter_add_marker (OmarchyMeter *self, double fraction);
void omarchy_meter_clear_markers (OmarchyMeter *self);
guint omarchy_meter_get_n_markers (OmarchyMeter *self);
double omarchy_meter_get_marker (OmarchyMeter *self, guint index);
const char *omarchy_meter_get_label (OmarchyMeter *self);
void omarchy_meter_set_label (OmarchyMeter *self, const char *label);
const char *omarchy_meter_get_value_text (OmarchyMeter *self);
void omarchy_meter_set_value_text (OmarchyMeter *self, const char *value_text);
gboolean omarchy_meter_get_indeterminate (OmarchyMeter *self);
void omarchy_meter_set_indeterminate (OmarchyMeter *self, gboolean indeterminate);
double omarchy_meter_get_thickness (OmarchyMeter *self);
void omarchy_meter_set_thickness (OmarchyMeter *self, double thickness);

#define OMARCHY_TYPE_PROGRESS_BAR (omarchy_progress_bar_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyProgressBar, omarchy_progress_bar, OMARCHY, PROGRESS_BAR, OmarchyMeter)

GtkWidget *omarchy_progress_bar_new (double value);
GtkWidget *omarchy_progress_bar_new_indeterminate (void);

#define OMARCHY_TYPE_SPINNER (omarchy_spinner_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySpinner, omarchy_spinner, OMARCHY, SPINNER, GtkWidget)

GtkWidget *omarchy_spinner_new (void);
const char *omarchy_spinner_get_glyph (OmarchySpinner *self);
void omarchy_spinner_set_glyph (OmarchySpinner *self, const char *glyph);
const char *omarchy_spinner_get_size (OmarchySpinner *self);
void omarchy_spinner_set_size (OmarchySpinner *self, const char *token);
void omarchy_spinner_set_pixel_size (OmarchySpinner *self, int px);
gboolean omarchy_spinner_get_pulse (OmarchySpinner *self);
void omarchy_spinner_set_pulse (OmarchySpinner *self, gboolean pulse);
gboolean omarchy_spinner_get_running (OmarchySpinner *self);
void omarchy_spinner_set_running (OmarchySpinner *self, gboolean running);
void omarchy_spinner_set_color (OmarchySpinner *self, const GdkRGBA *color);

#define OMARCHY_TYPE_EMPTY_STATE (omarchy_empty_state_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyEmptyState, omarchy_empty_state, OMARCHY, EMPTY_STATE, GtkWidget)

GtkWidget *omarchy_empty_state_new (const char *title);
const char *omarchy_empty_state_get_title (OmarchyEmptyState *self);
void omarchy_empty_state_set_title (OmarchyEmptyState *self, const char *title);
const char *omarchy_empty_state_get_glyph (OmarchyEmptyState *self);
void omarchy_empty_state_set_glyph (OmarchyEmptyState *self, const char *glyph);
const char *omarchy_empty_state_get_hint (OmarchyEmptyState *self);
void omarchy_empty_state_set_hint (OmarchyEmptyState *self, const char *hint);
const char *omarchy_empty_state_get_action_text (OmarchyEmptyState *self);
void omarchy_empty_state_set_action_text (OmarchyEmptyState *self, const char *text);
const char *omarchy_empty_state_get_size (OmarchyEmptyState *self);
void omarchy_empty_state_set_size (OmarchyEmptyState *self, const char *size);
gboolean omarchy_empty_state_get_cursor (OmarchyEmptyState *self);
void omarchy_empty_state_set_cursor (OmarchyEmptyState *self, gboolean cursor);

#define OMARCHY_TYPE_BANNER (omarchy_banner_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyBanner, omarchy_banner, OMARCHY, BANNER, GtkWidget)

GtkWidget *omarchy_banner_new (const char *text);
const char *omarchy_banner_get_text (OmarchyBanner *self);
void omarchy_banner_set_text (OmarchyBanner *self, const char *text);
const char *omarchy_banner_get_tone (OmarchyBanner *self);
void omarchy_banner_set_tone (OmarchyBanner *self, const char *tone);
const char *omarchy_banner_get_glyph (OmarchyBanner *self);
void omarchy_banner_set_glyph (OmarchyBanner *self, const char *glyph);
const char *omarchy_banner_get_action_text (OmarchyBanner *self);
void omarchy_banner_set_action_text (OmarchyBanner *self, const char *text);
gboolean omarchy_banner_get_dismissible (OmarchyBanner *self);
void omarchy_banner_set_dismissible (OmarchyBanner *self, gboolean dismissible);
gboolean omarchy_banner_get_cursor (OmarchyBanner *self);
void omarchy_banner_set_cursor (OmarchyBanner *self, gboolean cursor);

#define OMARCHY_TYPE_TOAST (omarchy_toast_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyToast, omarchy_toast, OMARCHY, TOAST, GtkWidget)

GtkWidget *omarchy_toast_new (const char *title);
const char *omarchy_toast_get_title (OmarchyToast *self);
void omarchy_toast_set_title (OmarchyToast *self, const char *title);
const char *omarchy_toast_get_text (OmarchyToast *self);
void omarchy_toast_set_text (OmarchyToast *self, const char *text);
const char *omarchy_toast_get_glyph (OmarchyToast *self);
void omarchy_toast_set_glyph (OmarchyToast *self, const char *glyph);
const char *omarchy_toast_get_action_text (OmarchyToast *self);
void omarchy_toast_set_action_text (OmarchyToast *self, const char *text);
double omarchy_toast_get_countdown (OmarchyToast *self);
void omarchy_toast_set_countdown (OmarchyToast *self, double countdown);
gboolean omarchy_toast_get_cursor (OmarchyToast *self);
void omarchy_toast_set_cursor (OmarchyToast *self, gboolean cursor);

G_END_DECLS
