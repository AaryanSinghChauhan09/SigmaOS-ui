/* omarchy-forms.h: Checkbox, Radio, RadioGroup, PasswordField, TextArea, MultiSelect, ColorWell,
 * SwatchStrip and Dialog (spec/components/forms.md). SettingRow is in omarchy-pane.h. */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* ---- Checkbox, Radio, RadioGroup ---- */

#define OMARCHY_TYPE_CHECKBOX (omarchy_checkbox_get_type ())
G_DECLARE_DERIVABLE_TYPE (OmarchyCheckbox, omarchy_checkbox, OMARCHY, CHECKBOX, GtkWidget)

struct _OmarchyCheckboxClass {
  GtkWidgetClass parent_class;
  /*< private >*/
  gpointer padding[4];
};

GtkWidget *omarchy_checkbox_new (const char *label);
const char *omarchy_checkbox_get_label (OmarchyCheckbox *self);
void omarchy_checkbox_set_label (OmarchyCheckbox *self, const char *label);
const char *omarchy_checkbox_get_description (OmarchyCheckbox *self);
void omarchy_checkbox_set_description (OmarchyCheckbox *self, const char *description);
gboolean omarchy_checkbox_get_checked (OmarchyCheckbox *self);
void omarchy_checkbox_set_checked (OmarchyCheckbox *self, gboolean checked);
gboolean omarchy_checkbox_get_exclusive (OmarchyCheckbox *self);
void omarchy_checkbox_set_exclusive (OmarchyCheckbox *self, gboolean exclusive);
gboolean omarchy_checkbox_get_cursor (OmarchyCheckbox *self);
void omarchy_checkbox_set_cursor (OmarchyCheckbox *self, gboolean cursor);

#define OMARCHY_TYPE_RADIO (omarchy_radio_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyRadio, omarchy_radio, OMARCHY, RADIO, OmarchyCheckbox)
GtkWidget *omarchy_radio_new (const char *label);

#define OMARCHY_TYPE_RADIO_GROUP (omarchy_radio_group_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyRadioGroup, omarchy_radio_group, OMARCHY, RADIO_GROUP, GtkWidget)
GtkWidget *omarchy_radio_group_new (void);
void omarchy_radio_group_append (OmarchyRadioGroup *self, const char *value, const char *label, const char *description);
void omarchy_radio_group_remove_all (OmarchyRadioGroup *self);
const char *omarchy_radio_group_get_value (OmarchyRadioGroup *self);
void omarchy_radio_group_set_value (OmarchyRadioGroup *self, const char *value);
int omarchy_radio_group_get_cursor_index (OmarchyRadioGroup *self);
void omarchy_radio_group_set_cursor_index (OmarchyRadioGroup *self, int index);

/* ---- PasswordField, TextArea ---- */

#define OMARCHY_TYPE_PASSWORD_FIELD (omarchy_password_field_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyPasswordField, omarchy_password_field, OMARCHY, PASSWORD_FIELD, GtkWidget)
GtkWidget *omarchy_password_field_new (void);
const char *omarchy_password_field_get_placeholder_text (OmarchyPasswordField *self);
void omarchy_password_field_set_placeholder_text (OmarchyPasswordField *self, const char *text);
gboolean omarchy_password_field_get_revealable (OmarchyPasswordField *self);
void omarchy_password_field_set_revealable (OmarchyPasswordField *self, gboolean revealable);
gboolean omarchy_password_field_get_revealed (OmarchyPasswordField *self);
void omarchy_password_field_set_revealed (OmarchyPasswordField *self, gboolean revealed);
const char *omarchy_password_field_get_error (OmarchyPasswordField *self);
void omarchy_password_field_set_error (OmarchyPasswordField *self, const char *error);
gboolean omarchy_password_field_get_cursor (OmarchyPasswordField *self);
void omarchy_password_field_set_cursor (OmarchyPasswordField *self, gboolean cursor);
void omarchy_password_field_shake (OmarchyPasswordField *self);
GtkText *omarchy_password_field_get_text_widget (OmarchyPasswordField *self);

#define OMARCHY_TYPE_TEXT_AREA (omarchy_text_area_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTextArea, omarchy_text_area, OMARCHY, TEXT_AREA, GtkWidget)
GtkWidget *omarchy_text_area_new (void);
char *omarchy_text_area_get_text (OmarchyTextArea *self);
void omarchy_text_area_set_text (OmarchyTextArea *self, const char *text);
const char *omarchy_text_area_get_placeholder_text (OmarchyTextArea *self);
void omarchy_text_area_set_placeholder_text (OmarchyTextArea *self, const char *text);
int omarchy_text_area_get_min_rows (OmarchyTextArea *self);
void omarchy_text_area_set_min_rows (OmarchyTextArea *self, int rows);
int omarchy_text_area_get_max_rows (OmarchyTextArea *self);
void omarchy_text_area_set_max_rows (OmarchyTextArea *self, int rows);
gboolean omarchy_text_area_get_cursor (OmarchyTextArea *self);
void omarchy_text_area_set_cursor (OmarchyTextArea *self, gboolean cursor);
GtkTextBuffer *omarchy_text_area_get_buffer (OmarchyTextArea *self);
GtkTextView *omarchy_text_area_get_text_view (OmarchyTextArea *self);

/* ---- MultiSelect ---- */

#define OMARCHY_TYPE_MULTI_SELECT (omarchy_multi_select_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyMultiSelect, omarchy_multi_select, OMARCHY, MULTI_SELECT, GtkWidget)
GtkWidget *omarchy_multi_select_new (const char *label);
const char *omarchy_multi_select_get_label (OmarchyMultiSelect *self);
void omarchy_multi_select_set_label (OmarchyMultiSelect *self, const char *label);
void omarchy_multi_select_append (OmarchyMultiSelect *self, const char *value, const char *label, const char *description);
void omarchy_multi_select_remove_all (OmarchyMultiSelect *self);
char **omarchy_multi_select_get_values (OmarchyMultiSelect *self);
void omarchy_multi_select_set_values (OmarchyMultiSelect *self, const char *const *values);
gboolean omarchy_multi_select_is_selected (OmarchyMultiSelect *self, const char *value);
void omarchy_multi_select_toggle_value (OmarchyMultiSelect *self, const char *value);
const char *omarchy_multi_select_get_placeholder_text (OmarchyMultiSelect *self);
void omarchy_multi_select_set_placeholder_text (OmarchyMultiSelect *self, const char *text);
const char *omarchy_multi_select_get_empty_text (OmarchyMultiSelect *self);
void omarchy_multi_select_set_empty_text (OmarchyMultiSelect *self, const char *text);
const char *omarchy_multi_select_get_no_selection_text (OmarchyMultiSelect *self);
void omarchy_multi_select_set_no_selection_text (OmarchyMultiSelect *self, const char *text);
const char *omarchy_multi_select_get_trigger_label (OmarchyMultiSelect *self);
void omarchy_multi_select_set_trigger_label (OmarchyMultiSelect *self, const char *text);
gboolean omarchy_multi_select_get_show_label (OmarchyMultiSelect *self);
void omarchy_multi_select_set_show_label (OmarchyMultiSelect *self, gboolean show);
gboolean omarchy_multi_select_get_cursor (OmarchyMultiSelect *self);
void omarchy_multi_select_set_cursor (OmarchyMultiSelect *self, gboolean cursor);
gboolean omarchy_multi_select_get_refreshable (OmarchyMultiSelect *self);
void omarchy_multi_select_set_refreshable (OmarchyMultiSelect *self, gboolean refreshable);
gboolean omarchy_multi_select_get_loading_options (OmarchyMultiSelect *self);
void omarchy_multi_select_set_loading_options (OmarchyMultiSelect *self, gboolean loading);
const char *omarchy_multi_select_get_options_error (OmarchyMultiSelect *self);
void omarchy_multi_select_set_options_error (OmarchyMultiSelect *self, const char *error);
gboolean omarchy_multi_select_get_inline_popup (OmarchyMultiSelect *self);
void omarchy_multi_select_set_inline_popup (OmarchyMultiSelect *self, gboolean inline_popup);
gboolean omarchy_multi_select_get_popup_open (OmarchyMultiSelect *self);
void omarchy_multi_select_open (OmarchyMultiSelect *self);
void omarchy_multi_select_close (OmarchyMultiSelect *self);
void omarchy_multi_select_toggle (OmarchyMultiSelect *self);
int omarchy_multi_select_get_current_index (OmarchyMultiSelect *self);
GtkWidget *omarchy_multi_select_get_search_field (OmarchyMultiSelect *self);

/* ---- ColorWell, SwatchStrip ---- */

#define OMARCHY_TYPE_COLOR_WELL (omarchy_color_well_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyColorWell, omarchy_color_well, OMARCHY, COLOR_WELL, GtkWidget)
GtkWidget *omarchy_color_well_new (const GdkRGBA *swatch);
GtkWidget *omarchy_color_well_new_for_role (const char *role);
void omarchy_color_well_get_swatch (OmarchyColorWell *self, GdkRGBA *swatch);
void omarchy_color_well_set_swatch (OmarchyColorWell *self, const GdkRGBA *swatch);
const char *omarchy_color_well_get_swatch_role (OmarchyColorWell *self);
void omarchy_color_well_set_swatch_role (OmarchyColorWell *self, const char *role);
gboolean omarchy_color_well_get_cursor (OmarchyColorWell *self);
void omarchy_color_well_set_cursor (OmarchyColorWell *self, gboolean cursor);

#define OMARCHY_TYPE_SWATCH_STRIP (omarchy_swatch_strip_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySwatchStrip, omarchy_swatch_strip, OMARCHY, SWATCH_STRIP, GtkWidget)
GtkWidget *omarchy_swatch_strip_new (void);
void omarchy_swatch_strip_append (OmarchySwatchStrip *self, const GdkRGBA *color);
void omarchy_swatch_strip_append_role (OmarchySwatchStrip *self, const char *role);
void omarchy_swatch_strip_remove_all (OmarchySwatchStrip *self);
guint omarchy_swatch_strip_get_n_colors (OmarchySwatchStrip *self);
void omarchy_swatch_strip_get_color (OmarchySwatchStrip *self, guint index, GdkRGBA *color);
int omarchy_swatch_strip_get_current_index (OmarchySwatchStrip *self);
void omarchy_swatch_strip_set_current_index (OmarchySwatchStrip *self, int index);
int omarchy_swatch_strip_get_cursor_index (OmarchySwatchStrip *self);
void omarchy_swatch_strip_set_cursor_index (OmarchySwatchStrip *self, int index);

/* ---- Dialog ---- */

#define OMARCHY_TYPE_DIALOG (omarchy_dialog_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyDialog, omarchy_dialog, OMARCHY, DIALOG, GtkWidget)
GtkWidget *omarchy_dialog_new (const char *title, const char *message);
gboolean omarchy_dialog_get_opened (OmarchyDialog *self);
void omarchy_dialog_set_opened (OmarchyDialog *self, gboolean opened);
const char *omarchy_dialog_get_title (OmarchyDialog *self);
void omarchy_dialog_set_title (OmarchyDialog *self, const char *title);
const char *omarchy_dialog_get_message (OmarchyDialog *self);
void omarchy_dialog_set_message (OmarchyDialog *self, const char *message);
const char *omarchy_dialog_get_secondary_text (OmarchyDialog *self);
void omarchy_dialog_set_secondary_text (OmarchyDialog *self, const char *text);
const char *omarchy_dialog_get_primary_text (OmarchyDialog *self);
void omarchy_dialog_set_primary_text (OmarchyDialog *self, const char *text);
gboolean omarchy_dialog_get_destructive (OmarchyDialog *self);
void omarchy_dialog_set_destructive (OmarchyDialog *self, gboolean destructive);
int omarchy_dialog_get_selected_index (OmarchyDialog *self);
void omarchy_dialog_set_selected_index (OmarchyDialog *self, int index);
void omarchy_dialog_append (OmarchyDialog *self, GtkWidget *child);
void omarchy_dialog_remove (OmarchyDialog *self, GtkWidget *child);
gboolean omarchy_dialog_handle_key (OmarchyDialog *self, guint keyval, GdkModifierType state);

G_END_DECLS
