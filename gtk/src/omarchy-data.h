/* omarchy-data.h: Table, Tree, SplitView, Sidebar and Calendar (spec/components/data.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_TABLE (omarchy_table_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTable, omarchy_table, OMARCHY, TABLE, GtkWidget)

GtkWidget *omarchy_table_new (void);
void omarchy_table_append_column (OmarchyTable *self, const char *title, int width, gboolean align_right);
void omarchy_table_remove_all_columns (OmarchyTable *self);
int omarchy_table_get_n_columns (OmarchyTable *self);
void omarchy_table_append_row (OmarchyTable *self, const char *const *cells);
void omarchy_table_remove_all_rows (OmarchyTable *self);
int omarchy_table_get_n_rows (OmarchyTable *self);
const char *omarchy_table_get_cell (OmarchyTable *self, int row, int column);
int omarchy_table_get_column_width (OmarchyTable *self, int column);
int omarchy_table_get_sort_column (OmarchyTable *self);
void omarchy_table_set_sort_column (OmarchyTable *self, int column);
gboolean omarchy_table_get_sort_ascending (OmarchyTable *self);
void omarchy_table_set_sort_ascending (OmarchyTable *self, gboolean ascending);
int omarchy_table_get_selected_row (OmarchyTable *self);
void omarchy_table_set_selected_row (OmarchyTable *self, int row);
int omarchy_table_get_cursor_row (OmarchyTable *self);
void omarchy_table_set_cursor_row (OmarchyTable *self, int row);
gboolean omarchy_table_get_has_cursor (OmarchyTable *self);
void omarchy_table_set_has_cursor (OmarchyTable *self, gboolean has_cursor);
gboolean omarchy_table_get_separators (OmarchyTable *self);
void omarchy_table_set_separators (OmarchyTable *self, gboolean separators);

#define OMARCHY_TYPE_TREE (omarchy_tree_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTree, omarchy_tree, OMARCHY, TREE, GtkWidget)

GtkWidget *omarchy_tree_new (void);
void omarchy_tree_append (OmarchyTree *self, const char *parent_id, const char *id, const char *label, const char *glyph);
void omarchy_tree_remove_all (OmarchyTree *self);
char **omarchy_tree_get_expanded (OmarchyTree *self);
void omarchy_tree_set_expanded (OmarchyTree *self, const char *const *ids);
gboolean omarchy_tree_is_expanded (OmarchyTree *self, const char *id);
void omarchy_tree_set_node_expanded (OmarchyTree *self, const char *id, gboolean expanded);
const char *omarchy_tree_get_selected_id (OmarchyTree *self);
void omarchy_tree_set_selected_id (OmarchyTree *self, const char *id);
const char *omarchy_tree_get_cursor_id (OmarchyTree *self);
void omarchy_tree_set_cursor_id (OmarchyTree *self, const char *id);
gboolean omarchy_tree_get_has_cursor (OmarchyTree *self);
void omarchy_tree_set_has_cursor (OmarchyTree *self, gboolean has_cursor);
int omarchy_tree_get_n_visible_rows (OmarchyTree *self);
const char *omarchy_tree_get_visible_id (OmarchyTree *self, int index);

#define OMARCHY_TYPE_SPLIT_VIEW (omarchy_split_view_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySplitView, omarchy_split_view, OMARCHY, SPLIT_VIEW, GtkWidget)

GtkWidget *omarchy_split_view_new (void);
void omarchy_split_view_append (OmarchySplitView *self, GtkWidget *child);
void omarchy_split_view_append_sized (OmarchySplitView *self, GtkWidget *child, int size, int minimum, int maximum);
void omarchy_split_view_remove (OmarchySplitView *self, GtkWidget *child);
int omarchy_split_view_get_pane_size (OmarchySplitView *self, GtkWidget *child);
void omarchy_split_view_set_pane_size (OmarchySplitView *self, GtkWidget *child, int size);
gboolean omarchy_split_view_get_has_cursor (OmarchySplitView *self);
void omarchy_split_view_set_has_cursor (OmarchySplitView *self, gboolean has_cursor);
int omarchy_split_view_get_handle_thickness (OmarchySplitView *self);
GtkWidget *omarchy_split_view_get_handle (OmarchySplitView *self, int index);

#define OMARCHY_TYPE_SIDEBAR (omarchy_sidebar_get_type ())
G_DECLARE_FINAL_TYPE (OmarchySidebar, omarchy_sidebar, OMARCHY, SIDEBAR, GtkWidget)

GtkWidget *omarchy_sidebar_new (void);
void omarchy_sidebar_append_section (OmarchySidebar *self, const char *title);
void omarchy_sidebar_append_item (OmarchySidebar *self, const char *id, const char *label, const char *glyph);
void omarchy_sidebar_remove_all (OmarchySidebar *self);
const char *omarchy_sidebar_get_selected_id (OmarchySidebar *self);
void omarchy_sidebar_set_selected_id (OmarchySidebar *self, const char *id);
const char *omarchy_sidebar_get_cursor_id (OmarchySidebar *self);
void omarchy_sidebar_set_cursor_id (OmarchySidebar *self, const char *id);
gboolean omarchy_sidebar_get_has_cursor (OmarchySidebar *self);
void omarchy_sidebar_set_has_cursor (OmarchySidebar *self, gboolean has_cursor);
gboolean omarchy_sidebar_get_collapsed (OmarchySidebar *self);
void omarchy_sidebar_set_collapsed (OmarchySidebar *self, gboolean collapsed);
int omarchy_sidebar_get_rail_width (OmarchySidebar *self);

#define OMARCHY_TYPE_CALENDAR (omarchy_calendar_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyCalendar, omarchy_calendar, OMARCHY, CALENDAR, GtkWidget)

GtkWidget *omarchy_calendar_new (int year, int month);
int omarchy_calendar_get_year (OmarchyCalendar *self);
void omarchy_calendar_set_year (OmarchyCalendar *self, int year);
int omarchy_calendar_get_month (OmarchyCalendar *self);
void omarchy_calendar_set_month (OmarchyCalendar *self, int month);
void omarchy_calendar_step_month (OmarchyCalendar *self, int delta);
const char *omarchy_calendar_get_today (OmarchyCalendar *self);
void omarchy_calendar_set_today (OmarchyCalendar *self, const char *date);
const char *omarchy_calendar_get_selected (OmarchyCalendar *self);
void omarchy_calendar_set_selected (OmarchyCalendar *self, const char *date);
const char *omarchy_calendar_get_cursor_date (OmarchyCalendar *self);
void omarchy_calendar_set_cursor_date (OmarchyCalendar *self, const char *date);
gboolean omarchy_calendar_get_has_cursor (OmarchyCalendar *self);
void omarchy_calendar_set_has_cursor (OmarchyCalendar *self, gboolean has_cursor);
void omarchy_calendar_set_month_names (OmarchyCalendar *self, const char *const *names);
void omarchy_calendar_set_weekday_names (OmarchyCalendar *self, const char *const *names);
char *omarchy_calendar_get_day_at (OmarchyCalendar *self, int index);

G_END_DECLS
