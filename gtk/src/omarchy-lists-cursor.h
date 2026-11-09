/* omarchy-lists-cursor.h: CursorModel, the keyboard cursor across a panel's sections
 * (spec/components/lists.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_CURSOR_MODEL (omarchy_cursor_model_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyCursorModel, omarchy_cursor_model, OMARCHY, CURSOR_MODEL, GObject)

OmarchyCursorModel *omarchy_cursor_model_new (void);
OmarchyCursorModel *omarchy_cursor_model_new_list (int count);

void omarchy_cursor_model_add_section (OmarchyCursorModel *self, const char *name, int count);
void omarchy_cursor_model_clear_sections (OmarchyCursorModel *self);
void omarchy_cursor_model_set_count (OmarchyCursorModel *self, const char *section, int count);
int omarchy_cursor_model_get_count (OmarchyCursorModel *self, const char *section);
guint omarchy_cursor_model_get_n_sections (OmarchyCursorModel *self);

const char *omarchy_cursor_model_get_focus_section (OmarchyCursorModel *self);
int omarchy_cursor_model_get_selected_index (OmarchyCursorModel *self);
gboolean omarchy_cursor_model_get_cursor_active (OmarchyCursorModel *self);
int omarchy_cursor_model_get_columns (OmarchyCursorModel *self);
void omarchy_cursor_model_set_columns (OmarchyCursorModel *self, int columns);
int omarchy_cursor_model_get_page_size (OmarchyCursorModel *self);
void omarchy_cursor_model_set_page_size (OmarchyCursorModel *self, int page_size);
GtkWidget *omarchy_cursor_model_get_reference_item (OmarchyCursorModel *self);
void omarchy_cursor_model_set_reference_item (OmarchyCursorModel *self, GtkWidget *item);

gboolean omarchy_cursor_model_is_at (OmarchyCursorModel *self, const char *section, int index);
void omarchy_cursor_model_set (OmarchyCursorModel *self, const char *section, int index);
void omarchy_cursor_model_reset (OmarchyCursorModel *self);
void omarchy_cursor_model_move (OmarchyCursorModel *self, int dx, int dy);
void omarchy_cursor_model_move_section (OmarchyCursorModel *self, int delta);
void omarchy_cursor_model_page (OmarchyCursorModel *self, int delta);
void omarchy_cursor_model_home (OmarchyCursorModel *self);
void omarchy_cursor_model_end (OmarchyCursorModel *self);

gboolean omarchy_cursor_model_handle_key (OmarchyCursorModel *self, guint keyval, GdkModifierType state);
char *omarchy_cursor_model_printable (guint keyval, GdkModifierType state);
gboolean omarchy_cursor_model_pointer_moved (OmarchyCursorModel *self, GtkWidget *item, double x, double y,
                                             const char *section, int index);
void omarchy_cursor_model_allow_initial_sample (OmarchyCursorModel *self);

void omarchy_cursor_model_attach (OmarchyCursorModel *self, GtkWidget *widget);
void omarchy_cursor_model_bind (OmarchyCursorModel *self, GtkWidget *row, const char *section, int index);

G_END_DECLS
