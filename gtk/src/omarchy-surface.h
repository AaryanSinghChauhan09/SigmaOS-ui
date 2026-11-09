/* omarchy-surface.h: Card, SectionHeader, Separator, MenuRow and Tooltip (spec/components.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_SECTION_HEADER (omarchy_section_header_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySectionHeader, omarchy_section_header, OMARCHY, SECTION_HEADER, GtkWidget)
GtkWidget *omarchy_section_header_new (const char *text);
const char *omarchy_section_header_get_text (OmarchySectionHeader *self);
void omarchy_section_header_set_text (OmarchySectionHeader *self, const char *text);

#define OMARCHY_TYPE_SEPARATOR (omarchy_separator_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySeparator, omarchy_separator, OMARCHY, SEPARATOR, GtkWidget)
GtkWidget *omarchy_separator_new (void);
void omarchy_separator_set_vertical (OmarchySeparator *self, gboolean vertical);

#define OMARCHY_TYPE_CARD (omarchy_card_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyCard, omarchy_card, OMARCHY, CARD, GtkWidget)
GtkWidget *omarchy_card_new (const char *title);
const char *omarchy_card_get_title (OmarchyCard *self);
void omarchy_card_set_title (OmarchyCard *self, const char *title);
void omarchy_card_append (OmarchyCard *self, GtkWidget *child);
void omarchy_card_remove (OmarchyCard *self, GtkWidget *child);
void omarchy_card_set_spacing (OmarchyCard *self, int spacing);
void omarchy_card_set_padding (OmarchyCard *self, int padding);

#define OMARCHY_TYPE_MENU_ROW (omarchy_menu_row_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyMenuRow, omarchy_menu_row, OMARCHY, MENU_ROW, GtkWidget)
GtkWidget *omarchy_menu_row_new (const char *text);
const char *omarchy_menu_row_get_text (OmarchyMenuRow *self);
void omarchy_menu_row_set_text (OmarchyMenuRow *self, const char *text);
gboolean omarchy_menu_row_get_checked (OmarchyMenuRow *self);
void omarchy_menu_row_set_checked (OmarchyMenuRow *self, gboolean checked);
gboolean omarchy_menu_row_get_highlighted (OmarchyMenuRow *self);
void omarchy_menu_row_set_highlighted (OmarchyMenuRow *self, gboolean highlighted);

#define OMARCHY_TYPE_TOOLTIP (omarchy_tooltip_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTooltip, omarchy_tooltip, OMARCHY, TOOLTIP, GtkWidget)
GtkWidget *omarchy_tooltip_new (const char *text);
const char *omarchy_tooltip_get_text (OmarchyTooltip *self);
void omarchy_tooltip_set_text (OmarchyTooltip *self, const char *text);
const char *omarchy_tooltip_get_shortcut (OmarchyTooltip *self);
void omarchy_tooltip_set_shortcut (OmarchyTooltip *self, const char *shortcut);

#define OMARCHY_TYPE_SURFACE (omarchy_surface_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySurface, omarchy_surface, OMARCHY, SURFACE, GtkWidget)
GtkWidget *omarchy_surface_new (const char *fill, const char *border);
void omarchy_surface_set_fill (OmarchySurface *self, const char *fill);
void omarchy_surface_set_border (OmarchySurface *self, const char *border);
void omarchy_surface_set_border_width (OmarchySurface *self, int width);
void omarchy_surface_set_small (OmarchySurface *self, gboolean small);
void omarchy_surface_set_padding (OmarchySurface *self, int x, int y);
void omarchy_surface_set_child (OmarchySurface *self, GtkWidget *child);
GtkWidget *omarchy_surface_get_child (OmarchySurface *self);

G_END_DECLS
