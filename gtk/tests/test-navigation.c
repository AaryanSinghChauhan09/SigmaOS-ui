/* Menu, ContextMenu, Keycap, KeyCombo, ShortcutSheet, Tooltip, Toolbar, Tabs and IconRail:
 * clicks and keys (qt/tests/tst_navigation.qml and more). */
#include <omarchy-gtk.h>

#include "drive.h"

static GString *ran;
static gboolean sidebar;

static void
on_triggered (OmarchyMenu *menu, OmarchyMenuItem *item, gpointer data)
{
  const char *text = omarchy_menu_item_get_text (item);
  if (!g_strcmp0 (text, "Sidebar"))
    return;
  if (ran->len)
    g_string_append_c (ran, ',');
  g_string_append (ran, text);
}

static void
on_sidebar (OmarchyMenuItem *item, gpointer data)
{
  sidebar = omarchy_menu_item_get_checked (item);
}

/* The navigation test menu, held open: a disabled row and a submenu row among plain ones. */
static GtkWidget *
test_menu (OmarchyMenu **sub_out)
{
  GtkWidget *m = omarchy_menu_new ();
  OmarchyMenu *menu = OMARCHY_MENU (m);
  omarchy_menu_set_held (menu, TRUE);
  omarchy_menu_item_set_icon (omarchy_menu_append_item (menu, "New"), "+");
  omarchy_menu_append_item (menu, "Open…");
  omarchy_menu_append_item (menu, "Save");
  gtk_widget_set_sensitive (GTK_WIDGET (omarchy_menu_append_item (menu, "Duplicate")), FALSE);
  omarchy_menu_append_separator (menu);
  OmarchyMenuItem *side = omarchy_menu_append_item (menu, "Sidebar");
  omarchy_menu_item_set_checkable (side, TRUE);
  omarchy_menu_item_set_checked (side, TRUE);
  sidebar = TRUE;
  g_signal_connect (side, "triggered", G_CALLBACK (on_sidebar), NULL);
  OmarchyMenuItem *export = omarchy_menu_append_item (menu, "Export");
  GtkWidget *sub = omarchy_menu_new ();
  omarchy_menu_append_item (OMARCHY_MENU (sub), "PDF");
  omarchy_menu_append_item (OMARCHY_MENU (sub), "PNG");
  omarchy_menu_item_set_submenu (export, OMARCHY_MENU (sub));
  if (sub_out)
    *sub_out = OMARCHY_MENU (sub);
  gtk_widget_set_halign (m, GTK_ALIGN_START);
  gtk_widget_set_valign (m, GTK_ALIGN_START);
  if (ran)
    g_string_free (ran, TRUE);
  ran = g_string_new ("");
  g_signal_connect (m, "triggered", G_CALLBACK (on_triggered), NULL);
  return m;
}

static OmarchyMenuItem *
row (OmarchyMenu *m, const char *text)
{
  for (guint i = 0; i < omarchy_menu_get_n_rows (m); i++)
    if (!g_strcmp0 (omarchy_menu_item_get_text (omarchy_menu_get_row (m, i)), text))
      return omarchy_menu_get_row (m, i);
  return NULL;
}

static const char *
current (OmarchyMenu *m)
{
  OmarchyMenuItem *i = omarchy_menu_get_current_item (m);
  return i ? omarchy_menu_item_get_text (i) : NULL;
}

TEST (test_menu_rows_run_on_click, "/navigation/menu-click")
{
  GtkWidget *m = test_menu (NULL);
  OmarchyMenu *menu = OMARCHY_MENU (m);
  GtkWidget *window = drive_window (m, 600, 500);
  /* Every row reserves the glyph and mark columns, so labels line up. */
  g_assert_true (omarchy_menu_get_has_icons (menu));
  g_assert_true (omarchy_menu_get_has_marks (menu));
  g_assert_cmpint (gtk_widget_get_width (m), >=, 160);
  drive_click (GTK_WIDGET (row (menu, "New")), -1, -1);
  g_assert_cmpstr (ran->str, ==, "New");
  drive_click (GTK_WIDGET (row (menu, "Sidebar")), -1, -1);
  g_assert_false (sidebar); /* a check row flips */
  drive_click (GTK_WIDGET (row (menu, "Duplicate")), -1, -1);
  g_assert_cmpstr (ran->str, ==, "New"); /* a disabled row doesn't run */
  drive_close (window);
}

TEST (test_menu_walks_with_vim_keys_and_arrows, "/navigation/menu-keys")
{
  GtkWidget *m = test_menu (NULL);
  OmarchyMenu *menu = OMARCHY_MENU (m);
  GtkWidget *window = drive_window (m, 600, 500);
  gtk_widget_grab_focus (m);
  drive_key (window, NULL, GDK_KEY_j, 0);
  g_assert_cmpstr (current (menu), ==, "New"); /* the first key lands on the first row */
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpstr (ran->str, ==, "Open…");
  drive_key (window, NULL, GDK_KEY_Down, 0);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_cmpstr (ran->str, ==, "Open…,Save");
  drive_key (window, NULL, GDK_KEY_k, 0);
  drive_key (window, NULL, GDK_KEY_KP_Enter, 0);
  g_assert_cmpstr (ran->str, ==, "Open…,Save,Open…");
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_key (window, NULL, GDK_KEY_j, 0);
  g_assert_cmpstr (current (menu), ==, "Sidebar"); /* skips the disabled row */
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_false (sidebar);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpstr (current (menu), ==, "New"); /* wraps past the last row */
  drive_key (window, NULL, GDK_KEY_Up, 0);
  g_assert_cmpstr (current (menu), ==, "Export"); /* and back past the first */
  drive_key (window, NULL, GDK_KEY_Home, 0);
  g_assert_cmpstr (current (menu), ==, "New");
  drive_key (window, NULL, GDK_KEY_End, 0);
  g_assert_cmpstr (current (menu), ==, "Export");
  /* Hovering a row puts the cursor on it. */
  drive_hover (GTK_WIDGET (row (menu, "Save")), TRUE);
  g_assert_true (omarchy_menu_item_get_hot (row (menu, "Save")));
  drive_close (window);
}

TEST (test_menu_submenu_opens_right_and_closes_left, "/navigation/menu-submenu")
{
  OmarchyMenu *sub;
  GtkWidget *m = test_menu (&sub);
  GtkWidget *window = drive_window (m, 600, 500);
  gtk_widget_grab_focus (m);
  drive_key (window, NULL, GDK_KEY_End, 0);
  drive_key (window, NULL, GDK_KEY_l, 0);
  drive_wait (30);
  g_assert_true (omarchy_menu_get_opened (sub)); /* l opens the submenu */
  g_assert_true (gtk_widget_has_focus (GTK_WIDGET (sub)));
  g_assert_cmpstr (current (sub), ==, "PDF");
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  drive_wait (30);
  g_assert_cmpstr (ran->str, ==, "PNG");
  g_assert_false (omarchy_menu_get_opened (sub)); /* running a row closes the submenu */
  g_assert_true (gtk_widget_has_focus (m));       /* and the held menu has the keys again */
  drive_key (window, NULL, GDK_KEY_End, 0);
  drive_key (window, NULL, GDK_KEY_Right, 0);
  g_assert_true (omarchy_menu_get_opened (sub));
  drive_key (window, NULL, GDK_KEY_h, 0);
  g_assert_false (omarchy_menu_get_opened (sub)); /* h goes back out */
  g_assert_true (gtk_widget_has_focus (m));
  drive_key (window, NULL, GDK_KEY_Right, 0);
  g_assert_true (omarchy_menu_get_opened (sub));
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_false (omarchy_menu_get_opened (sub)); /* so does Left */
  drive_key (window, NULL, GDK_KEY_Right, 0);
  drive_key (window, NULL, GDK_KEY_Escape, 0);
  g_assert_false (omarchy_menu_get_opened (sub)); /* and Escape */
  /* A click on the submenu row opens it too. */
  drive_click (GTK_WIDGET (row (OMARCHY_MENU (m), "Export")), -1, -1);
  g_assert_true (omarchy_menu_get_opened (sub));
  drive_click (GTK_WIDGET (row (sub, "PDF")), -1, -1);
  g_assert_cmpstr (ran->str, ==, "PNG,PDF");
  g_assert_false (omarchy_menu_get_opened (sub));
  drive_close (window);
}

static int closed;

static void
count_closed (gpointer a, gpointer b)
{
  closed++;
}

TEST (test_menu_opens_below_its_trigger_and_escape_closes, "/navigation/menu-open")
{
  GtkWidget *button = omarchy_button_new ("File");
  gtk_widget_set_halign (button, GTK_ALIGN_START);
  gtk_widget_set_valign (button, GTK_ALIGN_START);
  GtkWidget *window = drive_window (button, 400, 300);
  GtkWidget *m = omarchy_menu_new ();
  OmarchyMenu *menu = OMARCHY_MENU (m);
  omarchy_menu_append_item (menu, "Open…");
  omarchy_menu_append_item (menu, "Save");
  omarchy_menu_set_trigger (menu, button);
  closed = 0;
  g_signal_connect (m, "closed", G_CALLBACK (count_closed), NULL);
  omarchy_menu_open (menu);
  drive_wait (30);
  g_assert_true (omarchy_menu_get_opened (menu));
  g_assert_true (gtk_widget_get_mapped (m));
  g_assert_true (gtk_widget_has_focus (m));
  drive_key (window, NULL, GDK_KEY_Escape, 0);
  drive_wait (30);
  g_assert_false (omarchy_menu_get_opened (menu));
  g_assert_false (gtk_widget_get_mapped (m));
  g_assert_cmpint (closed, ==, 1);
  /* Running a row closes it. */
  omarchy_menu_open (menu);
  drive_wait (30);
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  drive_wait (30);
  g_assert_false (omarchy_menu_get_opened (menu));
  g_assert_cmpint (closed, ==, 2);
  drive_close (window);
}

TEST (test_menu_exclusive_rows_check_one, "/navigation/menu-radio")
{
  GtkWidget *m = omarchy_menu_new ();
  OmarchyMenu *menu = OMARCHY_MENU (m);
  omarchy_menu_set_held (menu, TRUE);
  OmarchyMenuItem *a = omarchy_menu_append_item (menu, "By name");
  OmarchyMenuItem *b = omarchy_menu_append_item (menu, "By date");
  for (OmarchyMenuItem **i = (OmarchyMenuItem *[]) { a, b, NULL }; *i; i++) {
    omarchy_menu_item_set_checkable (*i, TRUE);
    omarchy_menu_item_set_exclusive (*i, TRUE);
  }
  omarchy_menu_item_set_checked (a, TRUE);
  GtkWidget *window = drive_window (m, 300, 200);
  drive_click (GTK_WIDGET (b), -1, -1);
  g_assert_false (omarchy_menu_item_get_checked (a));
  g_assert_true (omarchy_menu_item_get_checked (b));
  gtk_widget_grab_focus (m);
  drive_key (window, NULL, GDK_KEY_Home, 0);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_true (omarchy_menu_item_get_checked (a));
  g_assert_false (omarchy_menu_item_get_checked (b));
  drive_close (window);
}

TEST (test_context_menu_opens_on_a_secondary_press, "/navigation/context-menu")
{
  GtkWidget *canvas = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request (canvas, 300, 200);
  gtk_widget_set_focusable (canvas, TRUE);
  GtkWidget *window = drive_window (canvas, 300, 200);
  GtkWidget *m = omarchy_context_menu_new ();
  OmarchyMenu *menu = OMARCHY_MENU (m);
  omarchy_menu_append_item (menu, "Copy");
  omarchy_context_menu_set_target (OMARCHY_CONTEXT_MENU (m), canvas);
  if (ran)
    g_string_free (ran, TRUE);
  ran = g_string_new ("");
  g_signal_connect (m, "triggered", G_CALLBACK (on_triggered), NULL);
  drive_click (canvas, 40, 40);
  g_assert_false (omarchy_menu_get_opened (menu)); /* a primary press doesn't */
  drive_right_click (canvas, 40, 40);
  drive_wait (30);
  g_assert_true (omarchy_menu_get_opened (menu));
  drive_click (GTK_WIDGET (omarchy_menu_get_row (menu, 0)), -1, -1);
  drive_wait (30);
  g_assert_cmpstr (ran->str, ==, "Copy");
  g_assert_false (omarchy_menu_get_opened (menu));
  /* The Menu key opens it from the keyboard; Escape closes it. */
  gtk_widget_grab_focus (canvas);
  g_assert_true (drive_key (window, canvas, GDK_KEY_Menu, 0));
  drive_wait (30);
  g_assert_true (omarchy_menu_get_opened (menu));
  drive_key (window, NULL, GDK_KEY_Escape, 0);
  drive_wait (30);
  g_assert_false (omarchy_menu_get_opened (menu));
  drive_close (window);
}

TEST (test_keycap_names_and_combos, "/navigation/keycap")
{
  static const char *const cases[][2] = {
    { "super", "Super" }, { "Meta", "Super" }, { "CMD", "Super" }, { "control", "Ctrl" }, { "option", "Alt" },
    { "Shift", "⇧" }, { "Return", "⏎" }, { "backspace", "⌫" }, { "escape", "Esc" }, { "del", "Del" },
    { "up", "↑" }, { "PageDown", "PgDn" }, { "space", "Space" }, { "k", "K" }, { "F11", "F11" }, { " tab ", "Tab" },
  };
  for (gsize i = 0; i < G_N_ELEMENTS (cases); i++) {
    g_autofree char *got = omarchy_keycap_normalize (cases[i][0]);
    g_assert_cmpstr (got, ==, cases[i][1]);
  }
  g_auto (GStrv) a = omarchy_key_combo_split ("Ctrl+Shift+S");
  g_assert_cmpuint (g_strv_length (a), ==, 3);
  g_assert_cmpstr (a[1], ==, "Shift");
  g_auto (GStrv) b = omarchy_key_combo_split ("Ctrl++");
  g_assert_cmpuint (g_strv_length (b), ==, 2);
  g_assert_cmpstr (b[1], ==, "+");

  GtkWidget *combo = omarchy_key_combo_new ("Ctrl+Shift+P");
  gtk_widget_set_halign (combo, GTK_ALIGN_START);
  gtk_widget_set_valign (combo, GTK_ALIGN_START);
  GtkWidget *window = drive_window (combo, 300, 100);
  GtkWidget *shift = drive_find (combo, OMARCHY_TYPE_KEYCAP, 1);
  g_assert_cmpstr (omarchy_keycap_get_label (OMARCHY_KEYCAP (shift)), ==, "⇧");
  /* At least square, and as tall as space(18). */
  g_assert_cmpint (gtk_widget_get_width (shift), >=, gtk_widget_get_height (shift));
  g_assert_null (drive_find (combo, OMARCHY_TYPE_KEYCAP, 3));
  drive_close (window);
}

TEST (test_tooltip_shortcut_adds_keycaps, "/navigation/tooltip")
{
  GtkWidget *tip = omarchy_tooltip_new ("Save");
  gtk_widget_set_halign (tip, GTK_ALIGN_START);
  gtk_widget_set_valign (tip, GTK_ALIGN_START);
  GtkWidget *window = drive_window (tip, 300, 100);
  int plain = gtk_widget_get_width (tip);
  omarchy_tooltip_set_shortcut (OMARCHY_TOOLTIP (tip), "Ctrl+S");
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (tip), >, plain);
  g_assert_nonnull (drive_find (tip, OMARCHY_TYPE_KEY_COMBO, 0));
  omarchy_tooltip_set_shortcut (OMARCHY_TOOLTIP (tip), NULL);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_width (tip), ==, plain);
  g_assert_null (drive_find (tip, OMARCHY_TYPE_KEY_COMBO, 0));
  drive_close (window);
}

TEST (test_shortcut_sheet_fills_columns, "/navigation/shortcut-sheet")
{
  GtkWidget *sheet = omarchy_shortcut_sheet_new ();
  OmarchyShortcutSheet *s = OMARCHY_SHORTCUT_SHEET (sheet);
  omarchy_shortcut_sheet_add_group (s, "File");
  omarchy_shortcut_sheet_add_shortcut (s, "Ctrl+N", "New file");
  omarchy_shortcut_sheet_add_group (s, "View");
  omarchy_shortcut_sheet_add_shortcut (s, "F11", "Full screen");
  omarchy_shortcut_sheet_add_group (s, "Go");
  omarchy_shortcut_sheet_add_shortcut (s, "Ctrl+P", "Go to file");
  GtkWidget *window = drive_window (sheet, 700, 300);
  /* Two columns: File and Go in the first, View in the second. */
  GtkWidget *first = gtk_widget_get_first_child (sheet);
  g_assert_nonnull (gtk_widget_get_next_sibling (first));
  g_assert_null (gtk_widget_get_next_sibling (gtk_widget_get_next_sibling (first)));
  g_assert_cmpstr (omarchy_section_header_get_text (OMARCHY_SECTION_HEADER (drive_find (first, OMARCHY_TYPE_SECTION_HEADER, 1))), ==, "Go");
  omarchy_shortcut_sheet_set_columns (s, 3);
  g_assert_nonnull (gtk_widget_get_next_sibling (gtk_widget_get_next_sibling (gtk_widget_get_first_child (sheet))));
  drive_close (window);
}

static int overflowed;

static void
count_overflow (gpointer a, gpointer b)
{
  overflowed++;
}

TEST (test_toolbar_overflow_clicks_and_keys, "/navigation/toolbar")
{
  GtkWidget *bar = omarchy_toolbar_new ();
  OmarchyToolbar *tb = OMARCHY_TOOLBAR (bar);
  GtkWidget *bold = omarchy_icon_button_new ("B");
  omarchy_toolbar_append_leading (tb, bold);
  omarchy_toolbar_append_separator (tb);
  omarchy_toolbar_append_center (tb, omarchy_heading_new ("notes.md"));
  GtkWidget *share = omarchy_button_new ("Share");
  omarchy_toolbar_append_trailing (tb, share);
  gtk_widget_set_valign (bar, GTK_ALIGN_START);
  GtkWidget *window = drive_window (bar, 600, 100);
  GtkWidget *more = omarchy_toolbar_get_overflow_button (tb);
  g_assert_false (gtk_widget_get_visible (more));
  omarchy_toolbar_set_overflow (tb, TRUE);
  drive_wait (20);
  overflowed = 0;
  g_signal_connect (bar, "overflow-triggered", G_CALLBACK (count_overflow), NULL);
  drive_click (more, -1, -1);
  gtk_widget_grab_focus (more);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpint (overflowed, ==, 2);
  /* Slots: leading at the left edge, trailing ending at the right, sm inside. */
  graphene_point_t p;
  g_assert_true (gtk_widget_compute_point (bold, bar, &GRAPHENE_POINT_INIT (0, 0), &p));
  g_assert_cmpfloat (p.x, ==, omarchy_theme_get_spacing (omarchy_theme_get_default (), "sm"));
  g_assert_true (gtk_widget_compute_point (more, bar, &GRAPHENE_POINT_INIT (gtk_widget_get_width (more), 0), &p));
  g_assert_cmpfloat (p.x, ==, 600 - omarchy_theme_get_spacing (omarchy_theme_get_default (), "sm"));
  drive_close (window);
}

static int activated, activated_index;

static void
on_activated (GtkWidget *w, int index, gpointer data)
{
  activated++;
  activated_index = index;
}

TEST (test_tabs_choose_on_click_and_keys, "/navigation/tabs")
{
  static const char *const labels[] = { "General", "Display", "Audio", "Network", NULL };
  GtkWidget *t = omarchy_tabs_new (labels);
  OmarchyTabs *tabs = OMARCHY_TABS (t);
  omarchy_tabs_set_current_index (tabs, 1);
  gtk_widget_set_valign (t, GTK_ALIGN_START);
  GtkWidget *window = drive_window (t, 600, 100);
  activated = 0;
  g_signal_connect (t, "activated", G_CALLBACK (on_activated), NULL);
  /* A click on the last tab (the tabs sit from the left; the last ends at their natural width). */
  int nat;
  gtk_widget_measure (t, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &nat, NULL, NULL);
  drive_click (t, nat - 10, gtk_widget_get_height (t) / 2.0);
  g_assert_cmpint (omarchy_tabs_get_current_index (tabs), ==, 3);
  g_assert_cmpint (activated, ==, 1);
  g_assert_cmpint (activated_index, ==, 3);
  /* A click past the tabs, on the rule, chooses nothing. */
  drive_click (t, 590, gtk_widget_get_height (t) / 2.0);
  g_assert_cmpint (activated, ==, 1);
  g_assert_true (gtk_widget_has_focus (t));
  omarchy_tabs_set_current_index (tabs, 1);
  static const struct {
    guint key;
    int want;
  } steps[] = { { GDK_KEY_Right, 2 }, { GDK_KEY_l, 3 }, { GDK_KEY_Right, 3 }, { GDK_KEY_h, 2 }, { GDK_KEY_Left, 1 },
                { GDK_KEY_Home, 0 }, { GDK_KEY_Left, 0 }, { GDK_KEY_End, 3 }, { GDK_KEY_Up, 3 }, { GDK_KEY_Down, 3 } };
  activated = 0;
  for (gsize i = 0; i < G_N_ELEMENTS (steps); i++) {
    drive_key (window, NULL, steps[i].key, 0);
    g_assert_cmpint (omarchy_tabs_get_current_index (tabs), ==, steps[i].want);
  }
  g_assert_cmpint (activated, ==, 6);
  drive_close (window);
}

TEST (test_rail_chooses_on_click_and_keys, "/navigation/icon-rail")
{
  GtkWidget *r = omarchy_icon_rail_new ();
  OmarchyIconRail *rail = OMARCHY_ICON_RAIL (r);
  const char *names[] = { "Files", "Search", "Source control", "Debug", "Settings" };
  for (int i = 0; i < 5; i++)
    omarchy_icon_rail_append (rail, "x", names[i]);
  gtk_widget_set_halign (r, GTK_ALIGN_START);
  gtk_widget_set_valign (r, GTK_ALIGN_START);
  GtkWidget *window = drive_window (r, 200, 400);
  activated = 0;
  g_signal_connect (r, "activated", G_CALLBACK (on_activated), NULL);
  int item = omarchy_icon_rail_get_item_size (rail);
  drive_click (r, gtk_widget_get_width (r) / 2.0, gtk_widget_get_height (r) - item / 2.0 - 4);
  g_assert_cmpint (omarchy_icon_rail_get_current_index (rail), ==, 4); /* a press on the last item */
  g_assert_cmpint (activated, ==, 1);
  g_assert_true (gtk_widget_has_focus (r));
  omarchy_icon_rail_set_current_index (rail, 0);
  static const struct {
    guint key;
    int want;
  } steps[] = { { GDK_KEY_Down, 1 }, { GDK_KEY_j, 2 }, { GDK_KEY_k, 1 }, { GDK_KEY_Up, 0 }, { GDK_KEY_Up, 0 },
                { GDK_KEY_End, 4 }, { GDK_KEY_Down, 4 }, { GDK_KEY_Home, 0 }, { GDK_KEY_Right, 0 }, { GDK_KEY_Left, 0 } };
  for (gsize i = 0; i < G_N_ELEMENTS (steps); i++) {
    drive_key (window, NULL, steps[i].key, 0);
    g_assert_cmpint (omarchy_icon_rail_get_current_index (rail), ==, steps[i].want);
  }
  drive_close (window);
}
