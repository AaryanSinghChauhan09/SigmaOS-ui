/* omarchy-button.h: Button, IconButton and ButtonGroup (spec/components.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_BUTTON (omarchy_button_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyButton, omarchy_button, OMARCHY, BUTTON, GtkWidget)

GtkWidget *omarchy_button_new (const char *label);
const char *omarchy_button_get_label (OmarchyButton *self);
void omarchy_button_set_label (OmarchyButton *self, const char *label);
const char *omarchy_button_get_icon (OmarchyButton *self);
void omarchy_button_set_icon (OmarchyButton *self, const char *glyph);
gboolean omarchy_button_get_bordered (OmarchyButton *self);
void omarchy_button_set_bordered (OmarchyButton *self, gboolean bordered);
gboolean omarchy_button_get_selected (OmarchyButton *self);
void omarchy_button_set_selected (OmarchyButton *self, gboolean selected);
gboolean omarchy_button_get_active (OmarchyButton *self);
void omarchy_button_set_active (OmarchyButton *self, gboolean active);
gboolean omarchy_button_get_cursor (OmarchyButton *self);
void omarchy_button_set_cursor (OmarchyButton *self, gboolean cursor);
gboolean omarchy_button_get_destructive (OmarchyButton *self);
void omarchy_button_set_destructive (OmarchyButton *self, gboolean destructive);
void omarchy_button_set_foreground (OmarchyButton *self, const GdkRGBA *color);
gboolean omarchy_button_get_left_align (OmarchyButton *self);
void omarchy_button_set_left_align (OmarchyButton *self, gboolean left_align);

#define OMARCHY_TYPE_ICON_BUTTON (omarchy_icon_button_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyIconButton, omarchy_icon_button, OMARCHY, ICON_BUTTON, GtkWidget)

GtkWidget *omarchy_icon_button_new (const char *glyph);
const char *omarchy_icon_button_get_icon (OmarchyIconButton *self);
void omarchy_icon_button_set_icon (OmarchyIconButton *self, const char *glyph);
gboolean omarchy_icon_button_get_bordered (OmarchyIconButton *self);
void omarchy_icon_button_set_bordered (OmarchyIconButton *self, gboolean bordered);
gboolean omarchy_icon_button_get_cursor (OmarchyIconButton *self);
void omarchy_icon_button_set_cursor (OmarchyIconButton *self, gboolean cursor);
gboolean omarchy_icon_button_get_destructive (OmarchyIconButton *self);
void omarchy_icon_button_set_destructive (OmarchyIconButton *self, gboolean destructive);

#define OMARCHY_TYPE_BUTTON_GROUP (omarchy_button_group_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyButtonGroup, omarchy_button_group, OMARCHY, BUTTON_GROUP, GtkWidget)

GtkWidget *omarchy_button_group_new (void);
void omarchy_button_group_append (OmarchyButtonGroup *self, const char *value, const char *label);
void omarchy_button_group_append_icon (OmarchyButtonGroup *self, const char *value, const char *label, const char *glyph);
void omarchy_button_group_remove_all (OmarchyButtonGroup *self);
const char *omarchy_button_group_get_value (OmarchyButtonGroup *self);
void omarchy_button_group_set_value (OmarchyButtonGroup *self, const char *value);
int omarchy_button_group_get_cursor_index (OmarchyButtonGroup *self);
void omarchy_button_group_set_cursor_index (OmarchyButtonGroup *self, int index);
int omarchy_button_group_get_focused_index (OmarchyButtonGroup *self);

G_END_DECLS
