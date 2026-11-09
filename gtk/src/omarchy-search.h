/* omarchy-search.h: SearchList and SearchItem (spec/components/composed.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

#include "omarchy-lists-cursor.h"

G_BEGIN_DECLS

#define OMARCHY_TYPE_SEARCH_ITEM (omarchy_search_item_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySearchItem, omarchy_search_item, OMARCHY, SEARCH_ITEM, GObject)

OmarchySearchItem *omarchy_search_item_new (const char *title);
const char *omarchy_search_item_get_title (OmarchySearchItem *self);
void omarchy_search_item_set_title (OmarchySearchItem *self, const char *title);
const char *omarchy_search_item_get_subtitle (OmarchySearchItem *self);
void omarchy_search_item_set_subtitle (OmarchySearchItem *self, const char *subtitle);
const char *omarchy_search_item_get_glyph (OmarchySearchItem *self);
void omarchy_search_item_set_glyph (OmarchySearchItem *self, const char *glyph);
const char *omarchy_search_item_get_value (OmarchySearchItem *self);
void omarchy_search_item_set_value (OmarchySearchItem *self, const char *value);
const char *omarchy_search_item_get_detail (OmarchySearchItem *self);
void omarchy_search_item_set_detail (OmarchySearchItem *self, const char *detail);

#define OMARCHY_TYPE_SEARCH_LIST (omarchy_search_list_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySearchList, omarchy_search_list, OMARCHY, SEARCH_LIST, GtkWidget)

GtkWidget *omarchy_search_list_new (void);
gboolean omarchy_search_list_matches (const char *text, const char *query);

const char *omarchy_search_list_get_query (OmarchySearchList *self);
void omarchy_search_list_set_query (OmarchySearchList *self, const char *query);
const char *omarchy_search_list_get_placeholder_text (OmarchySearchList *self);
void omarchy_search_list_set_placeholder_text (OmarchySearchList *self, const char *text);

void omarchy_search_list_append (OmarchySearchList *self, const char *title, const char *subtitle, const char *glyph,
                                 const char *value, const char *detail);
void omarchy_search_list_append_item (OmarchySearchList *self, OmarchySearchItem *item);
void omarchy_search_list_remove_all (OmarchySearchList *self);
guint omarchy_search_list_get_n_items (OmarchySearchList *self);
OmarchySearchItem *omarchy_search_list_get_item (OmarchySearchList *self, guint index);

gboolean omarchy_search_list_get_show_detail (OmarchySearchList *self);
void omarchy_search_list_set_show_detail (OmarchySearchList *self, gboolean show_detail);
GtkWidget *omarchy_search_list_get_detail (OmarchySearchList *self);
void omarchy_search_list_set_detail (OmarchySearchList *self, GtkWidget *detail);
int omarchy_search_list_get_detail_width (OmarchySearchList *self);
void omarchy_search_list_set_detail_width (OmarchySearchList *self, int width);

const char *omarchy_search_list_get_empty_glyph (OmarchySearchList *self);
void omarchy_search_list_set_empty_glyph (OmarchySearchList *self, const char *glyph);
const char *omarchy_search_list_get_empty_text (OmarchySearchList *self);
void omarchy_search_list_set_empty_text (OmarchySearchList *self, const char *text);
const char *omarchy_search_list_get_no_matches_text (OmarchySearchList *self);
void omarchy_search_list_set_no_matches_text (OmarchySearchList *self, const char *text);

OmarchyCursorModel *omarchy_search_list_get_cursor (OmarchySearchList *self);
int omarchy_search_list_get_cursor_index (OmarchySearchList *self);
OmarchySearchItem *omarchy_search_list_get_current_item (OmarchySearchList *self);

G_END_DECLS
