/* omarchy-navigation-menu.h: Menu, MenuItem, MenuSection, MenuSeparator and ContextMenu
 * (spec/components/navigation.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_MENU_ITEM (omarchy_menu_item_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyMenuItem, omarchy_menu_item, OMARCHY, MENU_ITEM, GtkWidget)

#define OMARCHY_TYPE_MENU (omarchy_menu_get_type ())
G_DECLARE_DERIVABLE_TYPE (OmarchyMenu, omarchy_menu, OMARCHY, MENU, GtkWidget)

struct _OmarchyMenuClass {
  GtkWidgetClass parent_class;
  gpointer padding[4];
};

GtkWidget *omarchy_menu_new (void);
void omarchy_menu_append (OmarchyMenu *self, GtkWidget *row);
void omarchy_menu_remove (OmarchyMenu *self, GtkWidget *row);
OmarchyMenuItem *omarchy_menu_append_item (OmarchyMenu *self, const char *text);
void omarchy_menu_append_separator (OmarchyMenu *self);
void omarchy_menu_append_section (OmarchyMenu *self, const char *text);
guint omarchy_menu_get_n_rows (OmarchyMenu *self);
OmarchyMenuItem *omarchy_menu_get_row (OmarchyMenu *self, guint index);
OmarchyMenuItem *omarchy_menu_get_current_item (OmarchyMenu *self);
int omarchy_menu_get_current_index (OmarchyMenu *self);
void omarchy_menu_set_current_index (OmarchyMenu *self, int index);
gboolean omarchy_menu_get_opened (OmarchyMenu *self);
gboolean omarchy_menu_get_held (OmarchyMenu *self);
void omarchy_menu_set_held (OmarchyMenu *self, gboolean held);
GtkWidget *omarchy_menu_get_trigger (OmarchyMenu *self);
void omarchy_menu_set_trigger (OmarchyMenu *self, GtkWidget *trigger);
int omarchy_menu_get_gap (OmarchyMenu *self);
void omarchy_menu_set_gap (OmarchyMenu *self, int gap);
int omarchy_menu_get_minimum_width (OmarchyMenu *self);
void omarchy_menu_set_minimum_width (OmarchyMenu *self, int width);
gboolean omarchy_menu_get_has_icons (OmarchyMenu *self);
gboolean omarchy_menu_get_has_marks (OmarchyMenu *self);
OmarchyMenu *omarchy_menu_get_parent_menu (OmarchyMenu *self);
void omarchy_menu_open (OmarchyMenu *self);
void omarchy_menu_popup (OmarchyMenu *self, GtkWidget *parent, double x, double y);
void omarchy_menu_close (OmarchyMenu *self);
gboolean omarchy_menu_handle_key (OmarchyMenu *self, guint keyval, GdkModifierType state);

GtkWidget *omarchy_menu_item_new (const char *text);
const char *omarchy_menu_item_get_text (OmarchyMenuItem *self);
void omarchy_menu_item_set_text (OmarchyMenuItem *self, const char *text);
const char *omarchy_menu_item_get_icon (OmarchyMenuItem *self);
void omarchy_menu_item_set_icon (OmarchyMenuItem *self, const char *glyph);
const char *omarchy_menu_item_get_shortcut (OmarchyMenuItem *self);
void omarchy_menu_item_set_shortcut (OmarchyMenuItem *self, const char *shortcut);
gboolean omarchy_menu_item_get_checkable (OmarchyMenuItem *self);
void omarchy_menu_item_set_checkable (OmarchyMenuItem *self, gboolean checkable);
gboolean omarchy_menu_item_get_checked (OmarchyMenuItem *self);
void omarchy_menu_item_set_checked (OmarchyMenuItem *self, gboolean checked);
gboolean omarchy_menu_item_get_exclusive (OmarchyMenuItem *self);
void omarchy_menu_item_set_exclusive (OmarchyMenuItem *self, gboolean exclusive);
gboolean omarchy_menu_item_get_destructive (OmarchyMenuItem *self);
void omarchy_menu_item_set_destructive (OmarchyMenuItem *self, gboolean destructive);
gboolean omarchy_menu_item_get_cursor (OmarchyMenuItem *self);
void omarchy_menu_item_set_cursor (OmarchyMenuItem *self, gboolean cursor);
gboolean omarchy_menu_item_get_has_submenu (OmarchyMenuItem *self);
void omarchy_menu_item_set_has_submenu (OmarchyMenuItem *self, gboolean has_submenu);
OmarchyMenu *omarchy_menu_item_get_submenu (OmarchyMenuItem *self);
void omarchy_menu_item_set_submenu (OmarchyMenuItem *self, OmarchyMenu *submenu);
OmarchyMenu *omarchy_menu_item_get_menu (OmarchyMenuItem *self);
gboolean omarchy_menu_item_get_highlighted (OmarchyMenuItem *self);
gboolean omarchy_menu_item_get_hot (OmarchyMenuItem *self);
void omarchy_menu_item_activate (OmarchyMenuItem *self);

#define OMARCHY_TYPE_MENU_SEPARATOR (omarchy_menu_separator_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyMenuSeparator, omarchy_menu_separator, OMARCHY, MENU_SEPARATOR, GtkWidget)
GtkWidget *omarchy_menu_separator_new (void);

#define OMARCHY_TYPE_MENU_SECTION (omarchy_menu_section_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyMenuSection, omarchy_menu_section, OMARCHY, MENU_SECTION, GtkWidget)
GtkWidget *omarchy_menu_section_new (const char *text);
const char *omarchy_menu_section_get_text (OmarchyMenuSection *self);
void omarchy_menu_section_set_text (OmarchyMenuSection *self, const char *text);

#define OMARCHY_TYPE_CONTEXT_MENU (omarchy_context_menu_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyContextMenu, omarchy_context_menu, OMARCHY, CONTEXT_MENU, OmarchyMenu)
GtkWidget *omarchy_context_menu_new (void);
GtkWidget *omarchy_context_menu_get_target (OmarchyContextMenu *self);
void omarchy_context_menu_set_target (OmarchyContextMenu *self, GtkWidget *target);

G_END_DECLS
