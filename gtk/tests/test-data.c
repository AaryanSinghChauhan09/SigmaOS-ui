/* Table, Tree, Sidebar, Calendar and SplitView: clicks and keys (qt/tests/tst_data.qml, and a
 * click and a key test for each). */
#include <omarchy-gtk.h>

#include "drive.h"

static int
sp (const char *token)
{
  return omarchy_theme_get_spacing (omarchy_theme_get_default (), token);
}

static GtkWidget *
top_left (GtkWidget *w, int width)
{
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  gtk_widget_set_valign (w, GTK_ALIGN_START);
  if (width > 0)
    gtk_widget_set_size_request (w, width, -1);
  return w;
}

static int fired;
static int last_int;
static char *last_str;

static void
on_int (GtkWidget *w, int v, gpointer data)
{
  fired++;
  last_int = v;
}

static void
on_str (GtkWidget *w, const char *v, gpointer data)
{
  fired++;
  g_free (last_str);
  last_str = g_strdup (v);
}

/* ================================================================================= Table === */

static GtkWidget *
table_new (void)
{
  GtkWidget *w = omarchy_table_new ();
  OmarchyTable *t = OMARCHY_TABLE (w);
  omarchy_table_append_column (t, "Name", 0, FALSE);
  omarchy_table_append_column (t, "Kind", 90, FALSE);
  omarchy_table_append_column (t, "Size", 70, TRUE);
  const char *rows[][4] = {
    { "assets", "Folder", "—", NULL }, { "Cargo.toml", "TOML", "1.2 KB", NULL }, { "README.md", "Markdown", "4.8 KB", NULL },
    { "src", "Folder", "—", NULL },    { "target", "Folder", "—", NULL },
  };
  for (guint i = 0; i < G_N_ELEMENTS (rows); i++)
    omarchy_table_append_row (t, rows[i]);
  omarchy_table_set_sort_column (t, 0);
  return top_left (w, 480);
}

TEST (test_table_selects_on_click_and_sorts_on_header, "/data/table-click")
{
  GtkWidget *w = table_new ();
  OmarchyTable *t = OMARCHY_TABLE (w);
  GtkWidget *window = drive_window (w, 600, 500);
  int rh = sp ("popupRowHeight");
  fired = 0;
  g_signal_connect (w, "row-activated", G_CALLBACK (on_int), NULL);
  /* "src": the fourth body row, under the header. */
  drive_click (w, 20, rh + 3 * rh + rh / 2.0);
  g_assert_cmpint (omarchy_table_get_selected_row (t), ==, 3);
  g_assert_cmpint (last_int, ==, 3);
  g_assert_true (gtk_widget_has_focus (w));
  g_signal_connect (w, "sort-requested", G_CALLBACK (on_int), NULL);
  drive_click (w, 20, rh / 2.0);
  g_assert_false (omarchy_table_get_sort_ascending (t)); /* the sorted column's header flips the order */
  g_assert_cmpint (last_int, ==, 0);
  drive_click (w, omarchy_table_get_column_width (t, 0) + 20, rh / 2.0);
  g_assert_cmpint (omarchy_table_get_sort_column (t), ==, 1);
  g_assert_true (omarchy_table_get_sort_ascending (t));
  g_assert_cmpint (fired, ==, 3);
  drive_close (window);
}

TEST (test_table_keys_move_the_cursor_and_choose, "/data/table-keys")
{
  GtkWidget *w = table_new ();
  OmarchyTable *t = OMARCHY_TABLE (w);
  GtkWidget *window = drive_window (w, 600, 500);
  fired = 0;
  g_signal_connect (w, "row-activated", G_CALLBACK (on_int), NULL);
  gtk_widget_grab_focus (w);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpint (omarchy_table_get_cursor_row (t), ==, 0); /* the first key lands on the first row */
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpint (omarchy_table_get_selected_row (t), ==, 1);
  drive_key (window, NULL, GDK_KEY_End, 0);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpint (omarchy_table_get_cursor_row (t), ==, 4); /* stops at the last row */
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_cmpint (omarchy_table_get_selected_row (t), ==, 4);
  drive_key (window, NULL, GDK_KEY_Home, 0);
  drive_key (window, NULL, GDK_KEY_Up, 0);
  g_assert_cmpint (omarchy_table_get_cursor_row (t), ==, 0); /* stops at the first row */
  drive_key (window, NULL, GDK_KEY_KP_Enter, 0);
  g_assert_cmpint (omarchy_table_get_selected_row (t), ==, 0);
  drive_key (window, NULL, GDK_KEY_Page_Down, 0);
  g_assert_cmpint (omarchy_table_get_cursor_row (t), ==, 4);
  g_assert_cmpint (fired, ==, 3);
  drive_close (window);
}

/* ================================================================================== Tree === */

static void
tree_app (GtkWidget *w)
{
  OmarchyTree *t = OMARCHY_TREE (w);
  omarchy_tree_append (t, NULL, "root", "omarchy-ui", NULL);
  omarchy_tree_append (t, "root", "qt", "qt", NULL);
  omarchy_tree_append (t, "qt", "qt/button", "Button.qml", NULL);
  omarchy_tree_append (t, "qt", "qt/table", "Table.qml", NULL);
  omarchy_tree_append (t, "root", "rune", "rune", NULL);
  omarchy_tree_append (t, "rune", "rune/lib", "lib.rs", NULL);
  omarchy_tree_append (t, "root", "spec", "spec", NULL);
  omarchy_tree_append (t, "spec", "spec/tokens", "tokens.md", NULL);
  const char *expanded[] = { "root", "qt", "spec", NULL };
  omarchy_tree_set_expanded (t, expanded);
}

static int
tree_row_of (OmarchyTree *t, const char *id)
{
  for (int i = 0; i < omarchy_tree_get_n_visible_rows (t); i++)
    if (!g_strcmp0 (omarchy_tree_get_visible_id (t, i), id))
      return i;
  return -1;
}

TEST (test_tree_folds_and_selects_on_click, "/data/tree-click")
{
  GtkWidget *w = top_left (omarchy_tree_new (), 220);
  OmarchyTree *t = OMARCHY_TREE (w);
  tree_app (w);
  GtkWidget *window = drive_window (w, 600, 500);
  int rh = sp ("popupRowHeight");
  fired = 0;
  g_signal_connect (w, "toggled", G_CALLBACK (on_str), NULL);
  /* The chevron sits in the slot before the label: md + depth × huge. */
  drive_click (w, sp ("md") + sp ("huge") + 4, tree_row_of (t, "qt") * rh + rh / 2.0);
  g_assert_false (omarchy_tree_is_expanded (t, "qt")); /* the chevron folds qt */
  g_assert_cmpstr (last_str, ==, "qt");
  g_assert_cmpint (tree_row_of (t, "qt/button"), ==, -1);
  g_assert_cmpstr (omarchy_tree_get_selected_id (t), ==, ""); /* folding doesn't choose */
  g_signal_connect (w, "activated", G_CALLBACK (on_str), NULL);
  drive_click (w, 150, tree_row_of (t, "spec/tokens") * rh + rh / 2.0);
  g_assert_cmpstr (omarchy_tree_get_selected_id (t), ==, "spec/tokens");
  g_assert_cmpstr (last_str, ==, "spec/tokens");
  g_assert_cmpint (fired, ==, 2);
  drive_close (window);
}

TEST (test_tree_keys_fold_unfold_and_choose, "/data/tree-keys")
{
  GtkWidget *w = top_left (omarchy_tree_new (), 220);
  OmarchyTree *t = OMARCHY_TREE (w);
  tree_app (w);
  GtkWidget *window = drive_window (w, 600, 500);
  gtk_widget_grab_focus (w);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpstr (omarchy_tree_get_cursor_id (t), ==, "root");
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpstr (omarchy_tree_get_cursor_id (t), ==, "qt");
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_false (omarchy_tree_is_expanded (t, "qt")); /* Left folds an open branch */
  drive_key (window, NULL, GDK_KEY_h, 0);
  g_assert_false (omarchy_tree_is_expanded (t, "qt")); /* and does nothing to a folded one */
  drive_key (window, NULL, GDK_KEY_Right, 0);
  g_assert_true (omarchy_tree_is_expanded (t, "qt")); /* Right unfolds it */
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpstr (omarchy_tree_get_cursor_id (t), ==, "qt/button");
  drive_key (window, NULL, GDK_KEY_Right, 0);
  g_auto (GStrv) expanded = omarchy_tree_get_expanded (t);
  g_assert_cmpint (g_strv_length (expanded), ==, 3); /* a leaf doesn't fold */
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpstr (omarchy_tree_get_selected_id (t), ==, "qt/button");
  drive_close (window);
}

/* =============================================================================== Sidebar === */

static GtkWidget *
sidebar_new (void)
{
  GtkWidget *w = omarchy_sidebar_new ();
  OmarchySidebar *s = OMARCHY_SIDEBAR (w);
  omarchy_sidebar_append_section (s, "Library");
  omarchy_sidebar_append_item (s, "all", "All files", NULL);
  omarchy_sidebar_append_item (s, "recent", "Recent", NULL);
  omarchy_sidebar_append_section (s, "Places");
  omarchy_sidebar_append_item (s, "documents", "Documents", NULL);
  omarchy_sidebar_set_selected_id (s, "recent");
  return top_left (w, 200);
}

TEST (test_sidebar_selects_on_click, "/data/sidebar-click")
{
  GtkWidget *w = sidebar_new ();
  OmarchySidebar *s = OMARCHY_SIDEBAR (w);
  GtkWidget *window = drive_window (w, 300, 400);
  fired = 0;
  g_signal_connect (w, "activated", G_CALLBACK (on_str), NULL);
  /* The last row: the bottom padding (sm) and half a row up from the bottom. */
  int h = gtk_widget_get_height (w);
  drive_click (w, 40, h - sp ("sm") - sp ("popupRowHeight") / 2.0);
  g_assert_cmpstr (omarchy_sidebar_get_selected_id (s), ==, "documents");
  g_assert_cmpstr (last_str, ==, "documents");
  /* A header is no row. */
  drive_click (w, 40, sp ("sm") + 2);
  g_assert_cmpint (fired, ==, 1);
  /* Collapsed, it is a rail. */
  omarchy_sidebar_set_collapsed (s, TRUE);
  gtk_widget_set_size_request (w, -1, -1);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (w), ==, omarchy_sidebar_get_rail_width (s));
  drive_close (window);
}

TEST (test_sidebar_keys_walk_across_sections, "/data/sidebar-keys")
{
  GtkWidget *w = sidebar_new ();
  OmarchySidebar *s = OMARCHY_SIDEBAR (w);
  GtkWidget *window = drive_window (w, 300, 400);
  gtk_widget_grab_focus (w);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpstr (omarchy_sidebar_get_cursor_id (s), ==, "all");
  drive_key (window, NULL, GDK_KEY_Down, 0);
  drive_key (window, NULL, GDK_KEY_j, 0);
  g_assert_cmpstr (omarchy_sidebar_get_cursor_id (s), ==, "documents"); /* into the next section */
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpstr (omarchy_sidebar_get_cursor_id (s), ==, "documents"); /* stops at the last */
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_cmpstr (omarchy_sidebar_get_selected_id (s), ==, "documents");
  drive_key (window, NULL, GDK_KEY_Up, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpstr (omarchy_sidebar_get_selected_id (s), ==, "recent");
  drive_close (window);
}

/* ============================================================================== Calendar === */

TEST (test_calendar_picks_on_click_and_steps, "/data/calendar-click")
{
  GtkWidget *w = top_left (omarchy_calendar_new (2026, 11), -1);
  OmarchyCalendar *c = OMARCHY_CALENDAR (w);
  omarchy_calendar_set_today (c, "2026-11-08");
  GtkWidget *window = drive_window (w, 400, 500);
  /* Weeks from Monday: November 2026 starts on a Sunday. */
  g_autofree char *first = omarchy_calendar_get_day_at (c, 0);
  g_assert_cmpstr (first, ==, "2026-10-26");
  fired = 0;
  g_signal_connect (w, "picked", G_CALLBACK (on_str), NULL);
  GtkWidget *prev = drive_find (w, OMARCHY_TYPE_ICON_BUTTON, 0);
  GtkWidget *next = drive_find (w, OMARCHY_TYPE_ICON_BUTTON, 1);
  int cw = omarchy_theme_space (omarchy_theme_get_default (), 36), ch = sp ("controlHeight"), gap = sp ("xxs");
  int days_y = gtk_widget_get_height (prev) + sp ("sm") + omarchy_theme_space (omarchy_theme_get_default (), 16) + gap;
  /* 2026-11-20: Friday of the fourth week. */
  drive_click (w, 4 * (cw + gap) + cw / 2.0, days_y + 3 * (ch + gap) + ch / 2.0);
  g_assert_cmpstr (omarchy_calendar_get_selected (c), ==, "2026-11-20");
  g_assert_cmpstr (last_str, ==, "2026-11-20");
  g_assert_cmpint (fired, ==, 1);
  drive_click (next, -1, -1);
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 12);
  drive_click (next, -1, -1);
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 1);
  g_assert_cmpint (omarchy_calendar_get_year (c), ==, 2027);
  drive_click (prev, -1, -1);
  g_assert_cmpint (omarchy_calendar_get_year (c), ==, 2026);
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 12);
  g_assert_cmpint (fired, ==, 1); /* stepping picks nothing */
  drive_close (window);
}

TEST (test_calendar_keys_move_the_cursor_across_months, "/data/calendar-keys")
{
  GtkWidget *w = top_left (omarchy_calendar_new (2026, 11), -1);
  OmarchyCalendar *c = OMARCHY_CALENDAR (w);
  omarchy_calendar_set_today (c, "2026-11-08");
  omarchy_calendar_set_selected (c, "2026-11-28");
  GtkWidget *window = drive_window (w, 400, 500);
  gtk_widget_grab_focus (w);
  drive_key (window, NULL, GDK_KEY_Right, 0); /* from the selected day */
  g_assert_cmpstr (omarchy_calendar_get_cursor_date (c), ==, "2026-11-29");
  drive_key (window, NULL, GDK_KEY_Down, 0); /* a week on, into December */
  g_assert_cmpstr (omarchy_calendar_get_cursor_date (c), ==, "2026-12-06");
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 12);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  drive_key (window, NULL, GDK_KEY_Up, 0);
  g_assert_cmpstr (omarchy_calendar_get_cursor_date (c), ==, "2026-11-28");
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 11);
  drive_key (window, NULL, GDK_KEY_h, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpstr (omarchy_calendar_get_selected (c), ==, "2026-11-27");
  drive_key (window, NULL, GDK_KEY_Page_Down, 0);
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 12);
  drive_key (window, NULL, GDK_KEY_Page_Up, 0);
  drive_key (window, NULL, GDK_KEY_Page_Up, 0);
  g_assert_cmpint (omarchy_calendar_get_month (c), ==, 10);
  drive_close (window);
}

/* ============================================================================= SplitView === */

static GtkWidget *
split_new (GtkWidget **side)
{
  GtkWidget *w = omarchy_split_view_new ();
  *side = gtk_label_new ("side");
  omarchy_split_view_append_sized (OMARCHY_SPLIT_VIEW (w), *side, 200, 120, 300);
  omarchy_split_view_append (OMARCHY_SPLIT_VIEW (w), gtk_label_new ("content"));
  return w;
}

static void
on_resized (GtkWidget *w, int pane, int size, gpointer data)
{
  fired++;
  g_assert_cmpint (pane, ==, 0);
  last_int = size;
}

static GtkGesture *
find_drag (GtkWidget *w)
{
  GListModel *list = gtk_widget_observe_controllers (w);
  GtkGesture *hit = NULL;
  for (guint i = 0; i < g_list_model_get_n_items (list) && !hit; i++) {
    g_autoptr (GtkEventController) c = g_list_model_get_item (list, i);
    if (GTK_IS_GESTURE_DRAG (c))
      hit = GTK_GESTURE (c);
  }
  g_object_unref (list);
  return hit;
}

TEST (test_split_view_drag_resizes_within_limits, "/data/split-drag")
{
  GtkWidget *side;
  GtkWidget *w = split_new (&side);
  OmarchySplitView *v = OMARCHY_SPLIT_VIEW (w);
  GtkWidget *window = drive_window (w, 720, 220);
  int md = sp ("md");
  g_assert_cmpint (omarchy_split_view_get_handle_thickness (v), ==, md);
  g_assert_cmpint (gtk_widget_get_width (side), ==, 200);
  GtkWidget *handle = omarchy_split_view_get_handle (v, 0);
  graphene_point_t p;
  g_assert_true (gtk_widget_compute_point (handle, w, &GRAPHENE_POINT_INIT (0, 0), &p));
  g_assert_cmpfloat (p.x, ==, 200);
  fired = 0;
  g_signal_connect (w, "resized", G_CALLBACK (on_resized), NULL);
  GtkGesture *drag = find_drag (handle);
  g_assert_nonnull (drag);
  g_signal_emit_by_name (drag, "drag-begin", 3.0, 100.0);
  g_signal_emit_by_name (drag, "drag-update", 40.0, 0.0);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (side), ==, 240);
  g_assert_cmpint (last_int, ==, 240);
  g_signal_emit_by_name (drag, "drag-update", -400.0, 0.0);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (side), ==, 120); /* its minimum */
  g_signal_emit_by_name (drag, "drag-update", 400.0, 0.0);
  g_signal_emit_by_name (drag, "drag-end", 400.0, 0.0);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (side), ==, 300); /* its maximum */
  /* A hidden pane takes its handle with it. */
  gtk_widget_set_visible (side, FALSE);
  drive_wait (20);
  g_assert_false (gtk_widget_get_child_visible (handle));
  drive_close (window);
}

TEST (test_split_view_handle_keys, "/data/split-keys")
{
  GtkWidget *side;
  GtkWidget *w = split_new (&side);
  OmarchySplitView *v = OMARCHY_SPLIT_VIEW (w);
  GtkWidget *window = drive_window (w, 720, 220);
  int md = sp ("md");
  GtkWidget *handle = omarchy_split_view_get_handle (v, 0);
  g_assert_true (gtk_widget_grab_focus (handle));
  drive_key (window, NULL, GDK_KEY_Right, 0);
  drive_wait (20);
  g_assert_cmpint (omarchy_split_view_get_pane_size (v, side), ==, 200 + md);
  drive_key (window, NULL, GDK_KEY_h, 0);
  drive_key (window, NULL, GDK_KEY_h, 0);
  drive_wait (20);
  g_assert_cmpint (omarchy_split_view_get_pane_size (v, side), ==, 200 - md);
  drive_key (window, NULL, GDK_KEY_Home, 0);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (side), ==, 120);
  drive_key (window, NULL, GDK_KEY_End, 0);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (side), ==, 300);
  drive_close (window);
}
