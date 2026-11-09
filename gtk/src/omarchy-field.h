/* omarchy-field.h: TextField, NumberField and Dropdown (spec/components.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_TEXT_FIELD (omarchy_text_field_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTextField, omarchy_text_field, OMARCHY, TEXT_FIELD, GtkWidget)
GtkWidget *omarchy_text_field_new (void);
const char *omarchy_text_field_get_placeholder_text (OmarchyTextField *self);
void omarchy_text_field_set_placeholder_text (OmarchyTextField *self, const char *text);
gboolean omarchy_text_field_get_cursor (OmarchyTextField *self);
void omarchy_text_field_set_cursor (OmarchyTextField *self, gboolean cursor);
gboolean omarchy_text_field_get_password (OmarchyTextField *self);
void omarchy_text_field_set_password (OmarchyTextField *self, gboolean password);
GtkText *omarchy_text_field_get_text_widget (OmarchyTextField *self);

#define OMARCHY_TYPE_NUMBER_FIELD (omarchy_number_field_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyNumberField, omarchy_number_field, OMARCHY, NUMBER_FIELD, GtkWidget)
GtkWidget *omarchy_number_field_new (const char *label, int from, int to);
const char *omarchy_number_field_get_label (OmarchyNumberField *self);
void omarchy_number_field_set_label (OmarchyNumberField *self, const char *label);
int omarchy_number_field_get_value (OmarchyNumberField *self);
void omarchy_number_field_set_value (OmarchyNumberField *self, int value);
void omarchy_number_field_set_range (OmarchyNumberField *self, int from, int to);
void omarchy_number_field_set_step (OmarchyNumberField *self, int step);
gboolean omarchy_number_field_get_cursor (OmarchyNumberField *self);
void omarchy_number_field_set_cursor (OmarchyNumberField *self, gboolean cursor);

#define OMARCHY_TYPE_DROPDOWN (omarchy_dropdown_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyDropdown, omarchy_dropdown, OMARCHY, DROPDOWN, GtkWidget)
GtkWidget *omarchy_dropdown_new (const char *label);
const char *omarchy_dropdown_get_label (OmarchyDropdown *self);
void omarchy_dropdown_set_label (OmarchyDropdown *self, const char *label);
void omarchy_dropdown_append (OmarchyDropdown *self, const char *value, const char *label);
void omarchy_dropdown_remove_all (OmarchyDropdown *self);
const char *omarchy_dropdown_get_value (OmarchyDropdown *self);
void omarchy_dropdown_set_value (OmarchyDropdown *self, const char *value);
gboolean omarchy_dropdown_get_cursor (OmarchyDropdown *self);
void omarchy_dropdown_set_cursor (OmarchyDropdown *self, gboolean cursor);
gboolean omarchy_dropdown_get_popup_open (OmarchyDropdown *self);
void omarchy_dropdown_open (OmarchyDropdown *self);
void omarchy_dropdown_close (OmarchyDropdown *self);
int omarchy_dropdown_get_current_index (OmarchyDropdown *self);

G_END_DECLS
