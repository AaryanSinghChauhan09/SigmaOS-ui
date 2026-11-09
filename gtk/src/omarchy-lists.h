/* omarchy-lists.h: ListRow, ScrollArea, KeyValue and Hero (spec/components/lists.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_LIST_ROW (omarchy_list_row_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyListRow, omarchy_list_row, OMARCHY, LIST_ROW, GtkWidget)

GtkWidget *omarchy_list_row_new (const char *title);
const char *omarchy_list_row_get_title (OmarchyListRow *self);
void omarchy_list_row_set_title (OmarchyListRow *self, const char *title);
const char *omarchy_list_row_get_subtitle (OmarchyListRow *self);
void omarchy_list_row_set_subtitle (OmarchyListRow *self, const char *subtitle);
const char *omarchy_list_row_get_status (OmarchyListRow *self);
void omarchy_list_row_set_status (OmarchyListRow *self, const char *text, const char *tone);
const char *omarchy_list_row_get_status_tone (OmarchyListRow *self);
void omarchy_list_row_set_status_tone (OmarchyListRow *self, const char *tone);
const char *omarchy_list_row_get_glyph (OmarchyListRow *self);
void omarchy_list_row_set_glyph (OmarchyListRow *self, const char *glyph);
int omarchy_list_row_get_glyph_size (OmarchyListRow *self);
void omarchy_list_row_set_glyph_size (OmarchyListRow *self, int px);
void omarchy_list_row_set_glyph_color (OmarchyListRow *self, const GdkRGBA *color);
void omarchy_list_row_set_leading (OmarchyListRow *self, GtkWidget *leading);
GtkWidget *omarchy_list_row_get_leading (OmarchyListRow *self);
const char *omarchy_list_row_get_value (OmarchyListRow *self);
void omarchy_list_row_set_value (OmarchyListRow *self, const char *value);
void omarchy_list_row_set_trailing (OmarchyListRow *self, GtkWidget *trailing);
GtkWidget *omarchy_list_row_get_trailing (OmarchyListRow *self);
const char *omarchy_list_row_get_trailing_glyph (OmarchyListRow *self);
void omarchy_list_row_set_trailing_glyph (OmarchyListRow *self, const char *glyph);
const char *omarchy_list_row_get_trailing_tone (OmarchyListRow *self);
void omarchy_list_row_set_trailing_tone (OmarchyListRow *self, const char *tone);
const char *omarchy_list_row_get_action_glyph (OmarchyListRow *self);
void omarchy_list_row_set_action_glyph (OmarchyListRow *self, const char *glyph);
gboolean omarchy_list_row_get_action_urgent (OmarchyListRow *self);
void omarchy_list_row_set_action_urgent (OmarchyListRow *self, gboolean urgent);
gboolean omarchy_list_row_get_action_cursor (OmarchyListRow *self);
void omarchy_list_row_set_action_cursor (OmarchyListRow *self, gboolean action_cursor);
const char *omarchy_list_row_get_action_tip (OmarchyListRow *self);
void omarchy_list_row_set_action_tip (OmarchyListRow *self, const char *tip);
gboolean omarchy_list_row_get_action_shown (OmarchyListRow *self);
void omarchy_list_row_set_expansion (OmarchyListRow *self, GtkWidget *expansion);
GtkWidget *omarchy_list_row_get_expansion (OmarchyListRow *self);
gboolean omarchy_list_row_get_has_cursor (OmarchyListRow *self);
void omarchy_list_row_set_has_cursor (OmarchyListRow *self, gboolean has_cursor);
gboolean omarchy_list_row_get_current (OmarchyListRow *self);
void omarchy_list_row_set_current (OmarchyListRow *self, gboolean current);
gboolean omarchy_list_row_get_disabled (OmarchyListRow *self);
void omarchy_list_row_set_disabled (OmarchyListRow *self, gboolean disabled);

#define OMARCHY_TYPE_SCROLL_AREA (omarchy_scroll_area_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyScrollArea, omarchy_scroll_area, OMARCHY, SCROLL_AREA, GtkWidget)

GtkWidget *omarchy_scroll_area_new (void);
void omarchy_scroll_area_append (OmarchyScrollArea *self, GtkWidget *child);
void omarchy_scroll_area_remove (OmarchyScrollArea *self, GtkWidget *child);
GtkWidget *omarchy_scroll_area_get_content (OmarchyScrollArea *self);
int omarchy_scroll_area_get_max_height (OmarchyScrollArea *self);
void omarchy_scroll_area_set_max_height (OmarchyScrollArea *self, int max_height);
int omarchy_scroll_area_get_spacing (OmarchyScrollArea *self);
void omarchy_scroll_area_set_spacing (OmarchyScrollArea *self, int spacing);
void omarchy_scroll_area_set_spacing_token (OmarchyScrollArea *self, const char *token);
GtkAdjustment *omarchy_scroll_area_get_vadjustment (OmarchyScrollArea *self);
gboolean omarchy_scroll_area_get_overflowing (OmarchyScrollArea *self);
void omarchy_scroll_area_ensure_visible (OmarchyScrollArea *self, GtkWidget *child);
void omarchy_scroll_area_ensure_visible_range (OmarchyScrollArea *self, double y, double height);

#define OMARCHY_TYPE_KEY_VALUE (omarchy_key_value_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyKeyValue, omarchy_key_value, OMARCHY, KEY_VALUE, GtkWidget)

GtkWidget *omarchy_key_value_new (void);
void omarchy_key_value_append (OmarchyKeyValue *self, const char *label, const char *value);
void omarchy_key_value_append_toned (OmarchyKeyValue *self, const char *label, const char *value, const char *tone);
void omarchy_key_value_remove_all (OmarchyKeyValue *self);
guint omarchy_key_value_get_n_pairs (OmarchyKeyValue *self);
int omarchy_key_value_get_columns (OmarchyKeyValue *self);
void omarchy_key_value_set_columns (OmarchyKeyValue *self, int columns);

#define OMARCHY_TYPE_HERO (omarchy_hero_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyHero, omarchy_hero, OMARCHY, HERO, GtkWidget)

GtkWidget *omarchy_hero_new (const char *glyph, const char *title);
const char *omarchy_hero_get_glyph (OmarchyHero *self);
void omarchy_hero_set_glyph (OmarchyHero *self, const char *glyph);
const char *omarchy_hero_get_title (OmarchyHero *self);
void omarchy_hero_set_title (OmarchyHero *self, const char *title);
const char *omarchy_hero_get_status (OmarchyHero *self);
void omarchy_hero_set_status (OmarchyHero *self, const char *status);
gboolean omarchy_hero_get_error (OmarchyHero *self);
void omarchy_hero_set_error (OmarchyHero *self, gboolean error);
double omarchy_hero_get_glyph_opacity (OmarchyHero *self);
void omarchy_hero_set_glyph_opacity (OmarchyHero *self, double opacity);
void omarchy_hero_set_trailing (OmarchyHero *self, GtkWidget *trailing);
GtkWidget *omarchy_hero_get_trailing (OmarchyHero *self);

G_END_DECLS
