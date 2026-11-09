/* Search: SearchList's keys case for case with qt/tests/tst_searchlist.qml (typing filters, j, k,
 * x and Space are text, Up/Down and the paging keys move the cursor, Return activates, Delete
 * asks to delete, Escape clears then asks to close, the empty state), a click on a row, and
 * matches(). */
#include <omarchy-gtk.h>

#include <string.h>

#include "drive.h"

static const struct {
  const char *title, *subtitle, *detail;
} all[] = {
  { "Terminal", "App", "Open a new terminal window." },
  { "Terminal font", "Style", "Pick the monospace font every terminal uses." },
  { "Terminal theme", "Style", "Match the terminal colors to the desktop theme." },
  { "Intermediate CA", "Certificate", "Trust a certificate authority for this machine." },
};

static int activated, deleted, closed, last_index;

static void
on_activated (OmarchySearchList *l, int index, gpointer data)
{
  activated++;
  last_index = index;
}

static void
on_deleted (OmarchySearchList *l, int index, gpointer data)
{
  deleted++;
  last_index = index;
}

static void
on_closed (OmarchySearchList *l, gpointer data)
{
  closed++;
}

/* The caller filters, as SearchList asks. */
static void
refilter (OmarchySearchList *list)
{
  omarchy_search_list_remove_all (list);
  const char *q = omarchy_search_list_get_query (list);
  for (guint i = 0; i < G_N_ELEMENTS (all); i++)
    if (omarchy_search_list_matches (all[i].title, q))
      omarchy_search_list_append (list, all[i].title, all[i].subtitle, NULL, NULL, all[i].detail);
}

typedef struct {
  GtkWidget *window, *widget;
  OmarchySearchList *list;
} Fixture;

static Fixture
make (const char *query)
{
  Fixture f;
  activated = deleted = closed = 0;
  last_index = -1;
  f.widget = omarchy_search_list_new ();
  f.list = OMARCHY_SEARCH_LIST (f.widget);
  gtk_widget_set_size_request (f.widget, 360, 300);
  omarchy_search_list_set_show_detail (f.list, TRUE);
  omarchy_search_list_set_query (f.list, query);
  refilter (f.list);
  g_signal_connect (f.widget, "notify::query", G_CALLBACK (refilter), NULL);
  g_signal_connect (f.widget, "activated", G_CALLBACK (on_activated), NULL);
  g_signal_connect (f.widget, "delete-requested", G_CALLBACK (on_deleted), NULL);
  g_signal_connect (f.widget, "close-requested", G_CALLBACK (on_closed), NULL);
  f.window = drive_window (f.widget, 600, 500);
  gtk_widget_grab_focus (f.widget);
  g_assert_true (gtk_widget_has_focus (f.widget)); /* the filter line holds focus */
  return f;
}

static void
key (Fixture *f, guint keyval)
{
  drive_key (f->window, NULL, keyval, 0);
}

static void
type (Fixture *f, const char *text)
{
  for (const char *p = text; *p; p++)
    key (f, gdk_unicode_to_keyval ((guchar) *p));
}

#define QUERY(f, q) g_assert_cmpstr (omarchy_search_list_get_query ((f).list), ==, (q))
#define N(f) omarchy_search_list_get_n_items ((f).list)
#define CURSOR(f) omarchy_search_list_get_cursor_index ((f).list)

TEST (test_typing_filters, "/search/typing-filters")
{
  Fixture f = make ("");
  g_assert_cmpuint (N (f), ==, 4);
  type (&f, "font");
  QUERY (f, "font");
  g_assert_cmpuint (N (f), ==, 1);
  g_assert_cmpstr (omarchy_search_item_get_title (omarchy_search_list_get_item (f.list, 0)), ==, "Terminal font");
  key (&f, GDK_KEY_BackSpace);
  QUERY (f, "fon");
  drive_close (f.window);
}

TEST (test_vim_keys_and_space_are_text, "/search/vim-keys-are-text")
{
  Fixture f = make ("");
  type (&f, "jk");
  QUERY (f, "jk"); /* j and k type, they don't move */
  g_assert_cmpint (CURSOR (f), ==, -1); /* nothing matches jk */
  key (&f, GDK_KEY_BackSpace);
  key (&f, GDK_KEY_BackSpace);
  type (&f, "term");
  key (&f, GDK_KEY_space);
  QUERY (f, "term ");
  g_assert_cmpuint (N (f), ==, 0);
  key (&f, GDK_KEY_BackSpace);
  type (&f, "x");
  QUERY (f, "termx"); /* x is text here, not delete */
  g_assert_cmpint (deleted, ==, 0);
  drive_close (f.window);
}

TEST (test_arrows_move_the_cursor, "/search/arrows")
{
  Fixture f = make ("term");
  g_assert_cmpint (CURSOR (f), ==, 0); /* the cursor shows on the first match from the start */
  key (&f, GDK_KEY_Down);
  g_assert_cmpint (CURSOR (f), ==, 1);
  key (&f, GDK_KEY_Down);
  key (&f, GDK_KEY_Down);
  g_assert_cmpint (CURSOR (f), ==, 3);
  key (&f, GDK_KEY_Down);
  g_assert_cmpint (CURSOR (f), ==, 3); /* it stops at the last row */
  key (&f, GDK_KEY_Up);
  g_assert_cmpint (CURSOR (f), ==, 2);
  key (&f, GDK_KEY_Home);
  g_assert_cmpint (CURSOR (f), ==, 0);
  key (&f, GDK_KEY_End);
  g_assert_cmpint (CURSOR (f), ==, 3);
  key (&f, GDK_KEY_Page_Up);
  g_assert_cmpint (CURSOR (f), ==, 0);
  key (&f, GDK_KEY_Page_Down);
  g_assert_cmpint (CURSOR (f), ==, 3);
  /* The detail pane follows the cursor; the rows show it. */
  g_assert_cmpstr (omarchy_search_item_get_title (omarchy_search_list_get_current_item (f.list)), ==, "Intermediate CA");
  GtkWidget *row = drive_find (f.widget, OMARCHY_TYPE_LIST_ROW, 3);
  g_assert_true (omarchy_list_row_get_has_cursor (OMARCHY_LIST_ROW (row)));
  g_assert_false (omarchy_list_row_get_has_cursor (OMARCHY_LIST_ROW (drive_find (f.widget, OMARCHY_TYPE_LIST_ROW, 0))));
  drive_close (f.window);
}

TEST (test_enter_activates_the_cursor_row, "/search/enter-activates")
{
  Fixture f = make ("term");
  key (&f, GDK_KEY_Down);
  key (&f, GDK_KEY_Down);
  key (&f, GDK_KEY_Return);
  g_assert_cmpint (activated, ==, 1);
  g_assert_cmpint (last_index, ==, 2);
  g_assert_cmpstr (omarchy_search_item_get_title (omarchy_search_list_get_item (f.list, 2)), ==, "Terminal theme");
  key (&f, GDK_KEY_KP_Enter);
  g_assert_cmpint (activated, ==, 2); /* keypad Enter too */
  drive_close (f.window);
}

TEST (test_new_query_starts_at_first_match, "/search/new-query")
{
  Fixture f = make ("term");
  key (&f, GDK_KEY_Down);
  key (&f, GDK_KEY_Down);
  g_assert_cmpint (CURSOR (f), ==, 2);
  type (&f, "i");
  QUERY (f, "termi");
  g_assert_cmpint (CURSOR (f), ==, 0);
  drive_close (f.window);
}

TEST (test_delete_asks_to_delete, "/search/delete")
{
  Fixture f = make ("term");
  key (&f, GDK_KEY_Down);
  key (&f, GDK_KEY_Delete);
  g_assert_cmpint (deleted, ==, 1);
  g_assert_cmpint (last_index, ==, 1);
  g_assert_cmpuint (N (f), ==, 4); /* asking only: the caller deletes */
  drive_close (f.window);
}

TEST (test_escape_clears_then_closes, "/search/escape")
{
  Fixture f = make ("termix");
  g_assert_cmpuint (N (f), ==, 0);
  key (&f, GDK_KEY_Escape);
  QUERY (f, "");
  g_assert_cmpuint (N (f), ==, 4); /* an empty query lists everything */
  g_assert_cmpint (closed, ==, 0);
  key (&f, GDK_KEY_Escape);
  g_assert_cmpint (closed, ==, 1);
  drive_close (f.window);
}

TEST (test_nothing_matches_shows_the_empty_state, "/search/empty")
{
  Fixture f = make ("zzz");
  g_assert_cmpuint (N (f), ==, 0);
  g_assert_cmpint (CURSOR (f), ==, -1);
  g_assert_null (omarchy_search_list_get_current_item (f.list));
  key (&f, GDK_KEY_Return);
  g_assert_cmpint (activated, ==, 0); /* Return with no rows does nothing */
  key (&f, GDK_KEY_Delete);
  g_assert_cmpint (deleted, ==, 0);
  GtkWidget *empty = drive_find (f.widget, OMARCHY_TYPE_EMPTY_STATE, 0);
  g_assert_nonnull (empty);
  g_assert_true (gtk_widget_get_visible (empty));
  /* The catalog's no-matches line, with the query in it. */
  const char *title = omarchy_empty_state_get_title (OMARCHY_EMPTY_STATE (empty));
  g_assert_nonnull (strstr (title, "zzz"));
  g_assert_null (strstr (title, "%1"));
  g_assert_cmpstr (omarchy_empty_state_get_glyph (OMARCHY_EMPTY_STATE (empty)), ==, "\U000F0349");
  g_assert_false (gtk_widget_get_visible (drive_find (f.widget, OMARCHY_TYPE_SCROLL_AREA, 0)));
  /* An empty query shows the caller's empty text instead. */
  omarchy_search_list_set_empty_text (f.list, "Clipboard is empty");
  key (&f, GDK_KEY_Escape);
  omarchy_search_list_remove_all (f.list);
  g_assert_cmpstr (omarchy_empty_state_get_title (OMARCHY_EMPTY_STATE (empty)), ==, "Clipboard is empty");
  drive_close (f.window);
}

TEST (test_click_activates_the_row, "/search/click")
{
  Fixture f = make ("term");
  GtkWidget *row = drive_find (f.widget, OMARCHY_TYPE_LIST_ROW, 1);
  g_assert_cmpstr (omarchy_list_row_get_title (OMARCHY_LIST_ROW (row)), ==, "Terminal font");
  drive_click (row, -1, -1);
  g_assert_cmpint (activated, ==, 1);
  g_assert_cmpint (last_index, ==, 1);
  g_assert_cmpint (CURSOR (f), ==, 1);
  g_assert_true (gtk_widget_has_focus (f.widget)); /* typing still goes to the filter line */
  drive_close (f.window);
}

TEST (test_chords_are_not_text, "/search/chords")
{
  Fixture f = make ("");
  drive_key (f.window, NULL, GDK_KEY_j, GDK_ALT_MASK);
  drive_key (f.window, NULL, GDK_KEY_k, GDK_CONTROL_MASK);
  QUERY (f, "");
  type (&f, "term");
  drive_key (f.window, NULL, GDK_KEY_u, GDK_CONTROL_MASK); /* Ctrl+U clears the line */
  QUERY (f, "");
  drive_close (f.window);
}

TEST (test_matches, "/search/matches")
{
  g_assert_true (omarchy_search_list_matches ("Terminal font", "FONT"));
  g_assert_true (omarchy_search_list_matches ("Terminal", ""));
  g_assert_true (omarchy_search_list_matches ("Émoji", "émo"));
  g_assert_false (omarchy_search_list_matches ("Terminal", "zzz"));
  g_assert_false (omarchy_search_list_matches (NULL, "a"));
}

TEST (test_defaults, "/search/defaults")
{
  GtkWidget *w = g_object_ref_sink (omarchy_search_list_new ());
  OmarchySearchList *l = OMARCHY_SEARCH_LIST (w);
  g_assert_cmpstr (omarchy_search_list_get_placeholder_text (l), !=, "");
  g_assert_nonnull (strstr (omarchy_search_list_get_no_matches_text (l), "%1"));
  g_assert_cmpint (omarchy_search_list_get_detail_width (l), ==, -1);
  OmarchyCursorModel *cursor = omarchy_search_list_get_cursor (l);
  g_assert_cmpstr (omarchy_cursor_model_get_focus_section (cursor), ==, "results");
  g_assert_true (omarchy_cursor_model_get_cursor_active (cursor)); /* showing from the start */
  g_object_unref (w);
}
