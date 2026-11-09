/* omarchy-navigation.h: Keycap, KeyCombo, ShortcutSheet, Toolbar, ToolbarSeparator, Tabs and
 * IconRail (spec/components/navigation.md). Menus are in omarchy-navigation-menu.h. */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_KEYCAP (omarchy_keycap_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyKeycap, omarchy_keycap, OMARCHY, KEYCAP, GtkWidget)
GtkWidget *omarchy_keycap_new (const char *key);
const char *omarchy_keycap_get_key (OmarchyKeycap *self);
void omarchy_keycap_set_key (OmarchyKeycap *self, const char *key);
const char *omarchy_keycap_get_label (OmarchyKeycap *self);
const char *omarchy_keycap_get_foreground (OmarchyKeycap *self);
void omarchy_keycap_set_foreground (OmarchyKeycap *self, const char *role);
char *omarchy_keycap_normalize (const char *key);

#define OMARCHY_TYPE_KEY_COMBO (omarchy_key_combo_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyKeyCombo, omarchy_key_combo, OMARCHY, KEY_COMBO, GtkWidget)
GtkWidget *omarchy_key_combo_new (const char *keys);
const char *omarchy_key_combo_get_keys (OmarchyKeyCombo *self);
void omarchy_key_combo_set_keys (OmarchyKeyCombo *self, const char *keys);
char **omarchy_key_combo_split (const char *combo);

#define OMARCHY_TYPE_SHORTCUT_SHEET (omarchy_shortcut_sheet_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyShortcutSheet, omarchy_shortcut_sheet, OMARCHY, SHORTCUT_SHEET, GtkWidget)
GtkWidget *omarchy_shortcut_sheet_new (void);
void omarchy_shortcut_sheet_add_group (OmarchyShortcutSheet *self, const char *title);
void omarchy_shortcut_sheet_add_shortcut (OmarchyShortcutSheet *self, const char *combo, const char *description);
void omarchy_shortcut_sheet_remove_all (OmarchyShortcutSheet *self);
int omarchy_shortcut_sheet_get_columns (OmarchyShortcutSheet *self);
void omarchy_shortcut_sheet_set_columns (OmarchyShortcutSheet *self, int columns);

#define OMARCHY_TYPE_TOOLBAR_SEPARATOR (omarchy_toolbar_separator_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyToolbarSeparator, omarchy_toolbar_separator, OMARCHY, TOOLBAR_SEPARATOR, GtkWidget)
GtkWidget *omarchy_toolbar_separator_new (void);

#define OMARCHY_TYPE_TOOLBAR (omarchy_toolbar_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyToolbar, omarchy_toolbar, OMARCHY, TOOLBAR, GtkWidget)
GtkWidget *omarchy_toolbar_new (void);
void omarchy_toolbar_append_leading (OmarchyToolbar *self, GtkWidget *child);
void omarchy_toolbar_append_center (OmarchyToolbar *self, GtkWidget *child);
void omarchy_toolbar_append_trailing (OmarchyToolbar *self, GtkWidget *child);
void omarchy_toolbar_append_separator (OmarchyToolbar *self);
void omarchy_toolbar_remove (OmarchyToolbar *self, GtkWidget *child);
gboolean omarchy_toolbar_get_overflow (OmarchyToolbar *self);
void omarchy_toolbar_set_overflow (OmarchyToolbar *self, gboolean overflow);
gboolean omarchy_toolbar_get_overflow_cursor (OmarchyToolbar *self);
void omarchy_toolbar_set_overflow_cursor (OmarchyToolbar *self, gboolean cursor);
gboolean omarchy_toolbar_get_divided (OmarchyToolbar *self);
void omarchy_toolbar_set_divided (OmarchyToolbar *self, gboolean divided);
GtkWidget *omarchy_toolbar_get_overflow_button (OmarchyToolbar *self);

#define OMARCHY_TYPE_TABS (omarchy_tabs_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTabs, omarchy_tabs, OMARCHY, TABS, GtkWidget)
GtkWidget *omarchy_tabs_new (const char *const *labels);
void omarchy_tabs_append (OmarchyTabs *self, const char *label);
void omarchy_tabs_remove_all (OmarchyTabs *self);
guint omarchy_tabs_get_n_tabs (OmarchyTabs *self);
const char *omarchy_tabs_get_label (OmarchyTabs *self, guint index);
int omarchy_tabs_get_current_index (OmarchyTabs *self);
void omarchy_tabs_set_current_index (OmarchyTabs *self, int index);
int omarchy_tabs_get_cursor_index (OmarchyTabs *self);
void omarchy_tabs_set_cursor_index (OmarchyTabs *self, int index);
void omarchy_tabs_select (OmarchyTabs *self, int index);

#define OMARCHY_TYPE_ICON_RAIL (omarchy_icon_rail_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyIconRail, omarchy_icon_rail, OMARCHY, ICON_RAIL, GtkWidget)
GtkWidget *omarchy_icon_rail_new (void);
void omarchy_icon_rail_append (OmarchyIconRail *self, const char *glyph, const char *label);
void omarchy_icon_rail_remove_all (OmarchyIconRail *self);
guint omarchy_icon_rail_get_n_items (OmarchyIconRail *self);
const char *omarchy_icon_rail_get_label (OmarchyIconRail *self, guint index);
int omarchy_icon_rail_get_current_index (OmarchyIconRail *self);
void omarchy_icon_rail_set_current_index (OmarchyIconRail *self, int index);
int omarchy_icon_rail_get_cursor_index (OmarchyIconRail *self);
void omarchy_icon_rail_set_cursor_index (OmarchyIconRail *self, int index);
int omarchy_icon_rail_get_item_size (OmarchyIconRail *self);
void omarchy_icon_rail_select (OmarchyIconRail *self, int index);

G_END_DECLS
