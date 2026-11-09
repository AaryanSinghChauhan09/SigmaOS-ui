/* omarchy-pane.h: Heading, Pane (spec/components/surfaces.md) and SettingRow
 * (spec/components/forms.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_HEADING (omarchy_heading_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyHeading, omarchy_heading, OMARCHY, HEADING, GtkWidget)
GtkWidget *omarchy_heading_new (const char *text);
const char *omarchy_heading_get_text (OmarchyHeading *self);
void omarchy_heading_set_text (OmarchyHeading *self, const char *text);
const char *omarchy_heading_get_subtitle (OmarchyHeading *self);
void omarchy_heading_set_subtitle (OmarchyHeading *self, const char *subtitle);
const char *omarchy_heading_get_level (OmarchyHeading *self);
void omarchy_heading_set_level (OmarchyHeading *self, const char *level);
gboolean omarchy_heading_get_centered (OmarchyHeading *self);
void omarchy_heading_set_centered (OmarchyHeading *self, gboolean centered);
void omarchy_heading_add_trailing (OmarchyHeading *self, GtkWidget *child);

#define OMARCHY_TYPE_PANE (omarchy_pane_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyPane, omarchy_pane, OMARCHY, PANE, GtkWidget)
GtkWidget *omarchy_pane_new (const char *title);
void omarchy_pane_set_title (OmarchyPane *self, const char *title);
void omarchy_pane_set_subtitle (OmarchyPane *self, const char *subtitle);
void omarchy_pane_add_action (OmarchyPane *self, GtkWidget *action);
void omarchy_pane_append (OmarchyPane *self, GtkWidget *child);
gboolean omarchy_pane_get_bordered (OmarchyPane *self);
void omarchy_pane_set_bordered (OmarchyPane *self, gboolean bordered);
const char *omarchy_pane_get_divider (OmarchyPane *self);
void omarchy_pane_set_divider (OmarchyPane *self, const char *side);

#define OMARCHY_TYPE_SETTING_ROW (omarchy_setting_row_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySettingRow, omarchy_setting_row, OMARCHY, SETTING_ROW, GtkWidget)
GtkWidget *omarchy_setting_row_new (const char *label, const char *description, GtkWidget *control);
const char *omarchy_setting_row_get_label (OmarchySettingRow *self);
void omarchy_setting_row_set_label (OmarchySettingRow *self, const char *label);
const char *omarchy_setting_row_get_description (OmarchySettingRow *self);
void omarchy_setting_row_set_description (OmarchySettingRow *self, const char *description);
gboolean omarchy_setting_row_get_cursor (OmarchySettingRow *self);
void omarchy_setting_row_set_cursor (OmarchySettingRow *self, gboolean cursor);
void omarchy_setting_row_add_control (OmarchySettingRow *self, GtkWidget *control);

G_END_DECLS
