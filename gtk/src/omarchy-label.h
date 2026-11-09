/* omarchy-label.h: text in the Omarchy font at a token size and a palette role. */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_LABEL (omarchy_label_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyLabel, omarchy_label, OMARCHY, LABEL, GtkWidget)

GtkWidget *omarchy_label_new (const char *text);
const char *omarchy_label_get_text (OmarchyLabel *self);
void omarchy_label_set_text (OmarchyLabel *self, const char *text);
const char *omarchy_label_get_size (OmarchyLabel *self);
void omarchy_label_set_size (OmarchyLabel *self, const char *token);
void omarchy_label_set_pixel_size (OmarchyLabel *self, int px);
gboolean omarchy_label_get_bold (OmarchyLabel *self);
void omarchy_label_set_bold (OmarchyLabel *self, gboolean bold);
const char *omarchy_label_get_role (OmarchyLabel *self);
void omarchy_label_set_role (OmarchyLabel *self, const char *role);
void omarchy_label_set_color (OmarchyLabel *self, const GdkRGBA *color);
gboolean omarchy_label_get_wrap (OmarchyLabel *self);
void omarchy_label_set_wrap (OmarchyLabel *self, gboolean wrap);
void omarchy_label_set_ellipsize (OmarchyLabel *self, gboolean ellipsize);
void omarchy_label_set_xalign (OmarchyLabel *self, float xalign);
void omarchy_label_set_top_padding (OmarchyLabel *self, int px);

G_END_DECLS
