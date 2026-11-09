/* Lists: CursorModel's keys case for case with qt/tests/tst_cursormodel.qml (and the Rune
 * CursorModel's unit tests), its pointer gate, and a click and a key for ListRow, ScrollArea and
 * TileGrid; Avatar's fallbacks. */
#include <omarchy-gtk.h>

#include "drive.h"

/* ---- CursorModel ------------------------------------------------------------------------- */

static int activated, deleted, closed, typed, last_index;
static char *typed_text[4];

static void
on_activated (OmarchyCursorModel *m, const char *section, int index, gpointer data)
{
  activated++;
  last_index = index;
}

static void
on_deleted (OmarchyCursorModel *m, const char *section, int index, gpointer data)
{
  deleted++;
}

static void
on_closed (OmarchyCursorModel *m, gpointer data)
{
  closed++;
}

static void
on_typed (OmarchyCursorModel *m, const char *text, gpointer data)
{
  if (typed < 4) {
    g_free (typed_text[typed]);
    typed_text[typed] = g_strdup (text);
  }
  typed++;
}

/* A focused panel that hands every key to the model, as a panel's Keys.onPressed does. */
typedef struct {
  GtkWidget *window, *panel;
  OmarchyCursorModel *model;
} Panel;

static Panel
make (OmarchyCursorModel *model)
{
  Panel p;
  activated = deleted = closed = typed = 0;
  p.model = model;
  p.panel = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_size_request (p.panel, 100, 100);
  omarchy_cursor_model_attach (model, p.panel);
  g_signal_connect (model, "activated", G_CALLBACK (on_activated), NULL);
  g_signal_connect (model, "delete-requested", G_CALLBACK (on_deleted), NULL);
  g_signal_connect (model, "close-requested", G_CALLBACK (on_closed), NULL);
  g_signal_connect (model, "text-typed", G_CALLBACK (on_typed), NULL);
  p.window = drive_window (p.panel, 200, 200);
  gtk_widget_grab_focus (p.panel);
  return p;
}

static void
done (Panel *p)
{
  drive_close (p->window);
  g_object_unref (p->model);
}

static void
key (Panel *p, guint keyval)
{
  drive_key (p->window, NULL, keyval, 0);
}

static OmarchyCursorModel *
panel_model (void)
{
  OmarchyCursorModel *m = omarchy_cursor_model_new ();
  omarchy_cursor_model_add_section (m, "header", 1);
  omarchy_cursor_model_add_section (m, "connected", 2);
  omarchy_cursor_model_add_section (m, "known", 0);
  omarchy_cursor_model_add_section (m, "discovered", 3);
  return m;
}

#define AT(m, s, i) g_assert_true (omarchy_cursor_model_is_at ((m), (s), (i)))

TEST (test_first_key_only_shows_the_cursor, "/lists/cursor/first-key")
{
  Panel p = make (panel_model ());
  g_assert_false (omarchy_cursor_model_is_at (p.model, "header", 0));
  key (&p, GDK_KEY_j);
  AT (p.model, "header", 0);
  key (&p, GDK_KEY_j);
  AT (p.model, "connected", 0);
  done (&p);
}

TEST (test_rows_cross_sections, "/lists/cursor/cross-sections")
{
  Panel p = make (panel_model ());
  omarchy_cursor_model_set (p.model, "connected", 1);
  key (&p, GDK_KEY_Down);
  AT (p.model, "discovered", 0); /* skips the empty known section */
  key (&p, GDK_KEY_k);
  AT (p.model, "connected", 1); /* back to the previous section's last row */
  key (&p, GDK_KEY_End);
  AT (p.model, "discovered", 2);
  key (&p, GDK_KEY_Down);
  AT (p.model, "discovered", 2); /* stops at the last row */
  key (&p, GDK_KEY_Home);
  AT (p.model, "header", 0);
  key (&p, GDK_KEY_Up);
  AT (p.model, "header", 0); /* stops at the first row */
  done (&p);
}

TEST (test_tab_jumps_sections, "/lists/cursor/tab")
{
  Panel p = make (panel_model ());
  omarchy_cursor_model_set (p.model, "header", 0);
  key (&p, GDK_KEY_Tab);
  AT (p.model, "connected", 0);
  key (&p, GDK_KEY_Tab);
  AT (p.model, "discovered", 0); /* skips the empty section */
  key (&p, GDK_KEY_ISO_Left_Tab);
  AT (p.model, "connected", 0);
  drive_key (p.window, NULL, GDK_KEY_Tab, GDK_SHIFT_MASK);
  AT (p.model, "header", 0);
  done (&p);
}

TEST (test_pages_stay_in_their_section, "/lists/cursor/pages")
{
  OmarchyCursorModel *m = omarchy_cursor_model_new ();
  omarchy_cursor_model_add_section (m, "a", 12);
  omarchy_cursor_model_add_section (m, "b", 4);
  Panel p = make (m);
  omarchy_cursor_model_set (m, "a", 0);
  key (&p, GDK_KEY_Page_Down);
  AT (m, "a", 5);
  key (&p, GDK_KEY_Page_Down);
  key (&p, GDK_KEY_Page_Down);
  AT (m, "a", 11);
  key (&p, GDK_KEY_Page_Up);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 6);
  done (&p);
}

TEST (test_grids_move_by_columns, "/lists/cursor/grid")
{
  OmarchyCursorModel *m = omarchy_cursor_model_new_list (8);
  omarchy_cursor_model_set_columns (m, 4);
  Panel p = make (m);
  omarchy_cursor_model_set (m, "", 1);
  key (&p, GDK_KEY_l);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 2);
  key (&p, GDK_KEY_Down);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 6);
  key (&p, GDK_KEY_Right);
  key (&p, GDK_KEY_Right);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 7); /* stays on its line */
  key (&p, GDK_KEY_k);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 3);
  key (&p, GDK_KEY_h);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 2);
  done (&p);

  OmarchyCursorModel *l = omarchy_cursor_model_new_list (3);
  Panel q = make (l);
  omarchy_cursor_model_set (l, "", 1);
  key (&q, GDK_KEY_Right);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (l), ==, 1); /* a list ignores column moves */
  done (&q);
}

TEST (test_short_last_line, "/lists/cursor/short-last-line")
{
  OmarchyCursorModel *m = omarchy_cursor_model_new_list (6);
  omarchy_cursor_model_set_columns (m, 4);
  Panel p = make (m);
  omarchy_cursor_model_set (m, "", 3);
  key (&p, GDK_KEY_Down);
  g_assert_cmpint (omarchy_cursor_model_get_selected_index (m), ==, 5); /* the short line's last tile */
  done (&p);
}

TEST (test_actions_and_type_ahead, "/lists/cursor/actions")
{
  Panel p = make (omarchy_cursor_model_new_list (3));
  omarchy_cursor_model_set (p.model, "", 1);
  key (&p, GDK_KEY_Return);
  key (&p, GDK_KEY_space);
  g_assert_cmpint (activated, ==, 2);
  g_assert_cmpint (last_index, ==, 1);
  key (&p, GDK_KEY_Escape);
  g_assert_cmpint (closed, ==, 1);
  key (&p, GDK_KEY_x);
  key (&p, GDK_KEY_Delete);
  g_assert_cmpint (deleted, ==, 2);
  key (&p, GDK_KEY_q);
  key (&p, GDK_KEY_7);
  key (&p, GDK_KEY_eacute); /* not ASCII: still text */
  g_assert_cmpint (typed, ==, 3);
  g_assert_cmpstr (typed_text[0], ==, "q");
  g_assert_cmpstr (typed_text[1], ==, "7");
  g_assert_cmpstr (typed_text[2], ==, "é");
  g_assert_false (drive_key (p.window, NULL, GDK_KEY_q, GDK_CONTROL_MASK)); /* Ctrl+Q is not text */
  g_assert_cmpint (typed, ==, 3);
  g_assert_null (omarchy_cursor_model_printable (GDK_KEY_space, 0));
  g_assert_null (omarchy_cursor_model_printable (GDK_KEY_Return, 0));
  done (&p);
}

TEST (test_counts_keep_the_cursor_on_a_row, "/lists/cursor/set-count")
{
  OmarchyCursorModel *m = panel_model ();
  omarchy_cursor_model_set (m, "discovered", 2);
  omarchy_cursor_model_set_count (m, "discovered", 1);
  g_assert_true (omarchy_cursor_model_is_at (m, "discovered", 0));
  omarchy_cursor_model_set_count (m, "discovered", 0);
  g_assert_true (omarchy_cursor_model_is_at (m, "connected", 0)); /* off the emptied section */
  omarchy_cursor_model_reset (m);
  g_assert_false (omarchy_cursor_model_get_cursor_active (m));
  g_object_unref (m);
}

/* ---- the pointer gate -------------------------------------------------------------------- */

static int moved_by_pointer;

static void
on_moved (OmarchyCursorModel *m, const char *section, int index, gboolean by_key, gpointer data)
{
  if (!by_key)
    moved_by_pointer++;
}

TEST (test_pointer_moves_only_when_it_travels, "/lists/cursor/pointer")
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *a = omarchy_list_row_new ("A"), *b = omarchy_list_row_new ("B");
  gtk_box_append (GTK_BOX (box), a);
  gtk_box_append (GTK_BOX (box), b);
  GtkWidget *window = drive_window (box, 300, 200);
  OmarchyCursorModel *m = omarchy_cursor_model_new_list (2);
  omarchy_cursor_model_set_reference_item (m, box);
  omarchy_cursor_model_bind (m, a, "", 0);
  omarchy_cursor_model_bind (m, b, "", 1);
  moved_by_pointer = 0;
  g_signal_connect (m, "moved", G_CALLBACK (on_moved), NULL);

  /* The first sample only primes the gate; a still pointer (rows moving under it) never counts. */
  g_signal_emit_by_name (b, "pointer-moved", 10.0, 10.0);
  g_assert_false (omarchy_cursor_model_get_cursor_active (m));
  g_signal_emit_by_name (b, "pointer-moved", 10.5, 10.5);
  g_assert_false (omarchy_cursor_model_get_cursor_active (m));
  /* A real move does, and the row's cursor flag follows. */
  g_signal_emit_by_name (b, "pointer-moved", 14.0, 10.0);
  g_assert_true (omarchy_cursor_model_is_at (m, "", 1));
  g_assert_true (omarchy_list_row_get_has_cursor (OMARCHY_LIST_ROW (b)));
  g_assert_false (omarchy_list_row_get_has_cursor (OMARCHY_LIST_ROW (a)));
  g_assert_cmpint (moved_by_pointer, ==, 1);
  /* A key resets the gate: the next sample only primes it again. */
  omarchy_cursor_model_move (m, 0, -1);
  g_assert_true (omarchy_list_row_get_has_cursor (OMARCHY_LIST_ROW (a)));
  g_signal_emit_by_name (b, "pointer-moved", 30.0, 10.0);
  g_assert_true (omarchy_cursor_model_is_at (m, "", 0));
  /* A view opened by the pointer keeps the row under it. */
  omarchy_cursor_model_allow_initial_sample (m);
  g_signal_emit_by_name (b, "pointer-moved", 30.0, 10.0);
  g_assert_true (omarchy_cursor_model_is_at (m, "", 1));
  g_object_unref (m);
  drive_close (window);
}

/* ---- ListRow ----------------------------------------------------------------------------- */

static int clicked, action_clicked;

static void
on_clicked (GtkWidget *w, gpointer data)
{
  clicked++;
}

static void
on_action (GtkWidget *w, gpointer data)
{
  action_clicked++;
}

TEST (test_list_row_clicks_and_keys, "/lists/list-row")
{
  GtkWidget *r = omarchy_list_row_new ("Office");
  OmarchyListRow *row = OMARCHY_LIST_ROW (r);
  omarchy_list_row_set_glyph (row, "󰤢");
  omarchy_list_row_set_subtitle (row, "Saved");
  omarchy_list_row_set_trailing_glyph (row, "󰌾");
  omarchy_list_row_set_action_glyph (row, "󰅙");
  gtk_widget_set_valign (r, GTK_ALIGN_START);
  GtkWidget *window = drive_window (r, 420, 100);
  clicked = action_clicked = 0;
  g_signal_connect (r, "clicked", G_CALLBACK (on_clicked), NULL);
  g_signal_connect (r, "action-clicked", G_CALLBACK (on_action), NULL);

  /* At rest the lock shows; the action shows under the cursor or the action cursor. */
  g_assert_false (omarchy_list_row_get_action_shown (row));
  omarchy_list_row_set_has_cursor (row, TRUE);
  g_assert_true (omarchy_list_row_get_action_shown (row));

  drive_click (r, 40, gtk_widget_get_height (r) / 2.0);
  g_assert_cmpint (clicked, ==, 1);
  /* The end slot: `controlPaddingX` in from the right, `space(22)` square. */
  drive_click (r, gtk_widget_get_width (r) - 10 - 11, gtk_widget_get_height (r) / 2.0);
  g_assert_cmpint (action_clicked, ==, 1);
  g_assert_cmpint (clicked, ==, 1);

  gtk_widget_grab_focus (r);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_cmpint (clicked, ==, 3);
  drive_key (window, NULL, GDK_KEY_Delete, 0);
  g_assert_cmpint (action_clicked, ==, 2);

  /* Disabled: no clicks, no keys, no action. */
  omarchy_list_row_set_disabled (row, TRUE);
  g_assert_false (omarchy_list_row_get_action_shown (row));
  drive_click (r, 40, gtk_widget_get_height (r) / 2.0);
  drive_key (window, r, GDK_KEY_Return, 0);
  g_assert_cmpint (clicked, ==, 3);
  g_assert_false (gtk_widget_get_focusable (r));
  drive_close (window);
}

TEST (test_list_row_metrics, "/lists/list-row-metrics")
{
  OmarchyTheme *t = omarchy_theme_get_default ();
  GtkWidget *r = omarchy_list_row_new ("Basecamp");
  omarchy_list_row_set_glyph (OMARCHY_LIST_ROW (r), "󰤨");
  GtkWidget *window = drive_window (r, 420, 200);
  int one, two;
  gtk_widget_measure (r, GTK_ORIENTATION_VERTICAL, 420, &one, NULL, NULL, NULL);
  omarchy_list_row_set_status (OMARCHY_LIST_ROW (r), "Connected", "success");
  gtk_widget_measure (r, GTK_ORIENTATION_VERTICAL, 420, &two, NULL, NULL, NULL);
  /* height = content + rowPaddingX; a status line adds a caption line `space(1)` down. */
  g_assert_cmpint (two, >, one);
  g_assert_cmpstr (omarchy_list_row_get_status_tone (OMARCHY_LIST_ROW (r)), ==, "success");
  int pad = omarchy_theme_get_spacing (t, "rowPaddingX");
  g_assert_cmpint (one, >=, pad + 1);
  /* Revealing the action never reflows the row: the slot is reserved. */
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (r), "󰌾");
  int nat1, nat2;
  gtk_widget_measure (r, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &nat1, NULL, NULL);
  omarchy_list_row_set_action_glyph (OMARCHY_LIST_ROW (r), "󰅙");
  omarchy_list_row_set_has_cursor (OMARCHY_LIST_ROW (r), TRUE);
  gtk_widget_measure (r, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &nat2, NULL, NULL);
  g_assert_cmpint (nat1, ==, nat2);
  drive_close (window);
}

/* ---- ScrollArea -------------------------------------------------------------------------- */

TEST (test_scroll_area_caps_scrolls_and_reveals, "/lists/scroll-area")
{
  GtkWidget *s = omarchy_scroll_area_new ();
  OmarchyScrollArea *sa = OMARCHY_SCROLL_AREA (s);
  omarchy_scroll_area_set_max_height (sa, 120);
  GtkWidget *rows[10];
  for (int i = 0; i < 10; i++) {
    g_autofree char *t = g_strdup_printf ("Row %d", i);
    rows[i] = omarchy_list_row_new (t);
    omarchy_scroll_area_append (sa, rows[i]);
  }
  gtk_widget_set_valign (s, GTK_ALIGN_START);
  GtkWidget *window = drive_window (s, 300, 400);
  g_assert_cmpint (gtk_widget_get_height (s), ==, 120);
  g_assert_true (omarchy_scroll_area_get_overflowing (sa));
  GtkAdjustment *adj = omarchy_scroll_area_get_vadjustment (sa);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), ==, 0);

  /* Reveal the last row: just far enough. */
  omarchy_scroll_area_ensure_visible (sa, rows[9]);
  drive_wait (10);
  double bottom = gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), ==, bottom);
  omarchy_scroll_area_ensure_visible (sa, rows[0]);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), ==, 0);

  /* Keys scroll it (no cursor model attached), stopping at the ends. */
  gtk_widget_set_focusable (s, TRUE);
  gtk_widget_grab_focus (s);
  drive_key (window, NULL, GDK_KEY_End, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), ==, bottom);
  drive_key (window, NULL, GDK_KEY_Page_Down, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), ==, bottom);
  drive_key (window, NULL, GDK_KEY_Home, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), ==, 0);
  drive_key (window, NULL, GDK_KEY_Page_Down, 0);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), >, 0);
  omarchy_scroll_area_ensure_visible (sa, rows[0]);
  drive_wait (10);

  /* A click on a row reaches the row, under the bar too (the bar takes drags, not clicks). */
  clicked = 0;
  g_signal_connect (rows[1], "clicked", G_CALLBACK (on_clicked), NULL);
  drive_click (rows[1], 40, 10);
  g_assert_cmpint (clicked, ==, 1);

  /* Dragging the thumb scrolls the content. */
  GListModel *list = gtk_widget_observe_controllers (s);
  for (guint i = 0; i < g_list_model_get_n_items (list); i++) {
    g_autoptr (GtkEventController) c = g_list_model_get_item (list, i);
    if (!GTK_IS_GESTURE_DRAG (c))
      continue;
    g_signal_emit_by_name (c, "drag-begin", gtk_widget_get_width (s) - 2.0, 5.0);
    g_signal_emit_by_name (c, "drag-update", 0.0, 40.0);
    g_signal_emit_by_name (c, "drag-end", 0.0, 40.0);
  }
  g_object_unref (list);
  g_assert_cmpfloat (gtk_adjustment_get_value (adj), >, 0);

  /* A short list doesn't overflow and shows no bar. */
  for (int i = 2; i < 10; i++)
    omarchy_scroll_area_remove (sa, rows[i]);
  drive_wait (20);
  g_assert_false (omarchy_scroll_area_get_overflowing (sa));
  g_assert_cmpint (gtk_widget_get_height (s), <, 120);
  drive_close (window);
}

TEST (test_scroll_area_with_cursor_model, "/lists/scroll-area-cursor")
{
  GtkWidget *s = omarchy_scroll_area_new ();
  OmarchyScrollArea *sa = OMARCHY_SCROLL_AREA (s);
  omarchy_scroll_area_set_max_height (sa, 100);
  OmarchyCursorModel *m = omarchy_cursor_model_new_list (8);
  for (int i = 0; i < 8; i++) {
    GtkWidget *r = omarchy_list_row_new ("Row");
    gtk_widget_set_focusable (r, FALSE);
    omarchy_scroll_area_append (sa, r);
    omarchy_cursor_model_bind (m, r, "", i);
  }
  omarchy_cursor_model_attach (m, s);
  gtk_widget_set_valign (s, GTK_ALIGN_START);
  GtkWidget *window = drive_window (s, 300, 300);
  gtk_widget_grab_focus (s);
  /* The model takes the keys: End moves the cursor, not the view. */
  drive_key (window, NULL, GDK_KEY_End, 0);
  g_assert_true (omarchy_cursor_model_is_at (m, "", 7));
  GtkWidget *last = gtk_widget_get_last_child (omarchy_scroll_area_get_content (sa));
  g_assert_true (omarchy_list_row_get_has_cursor (OMARCHY_LIST_ROW (last)));
  g_object_unref (m);
  drive_close (window);
}

/* ---- TileGrid ---------------------------------------------------------------------------- */

static int tile_activated;

static void
on_tile (OmarchyTileGrid *g, int index, gpointer data)
{
  tile_activated = index;
}

TEST (test_tile_grid_clicks_and_keys, "/lists/tile-grid")
{
  GtkWidget *grid = omarchy_tile_grid_new ();
  OmarchyTileGrid *g = OMARCHY_TILE_GRID (grid);
  const char *labels[] = { "Terminal", "Files", "Browser", "Music", "Settings", "Calendar" };
  for (int i = 0; i < 6; i++)
    omarchy_tile_grid_append (g, labels[i], "");
  omarchy_tile_grid_set_columns (g, 4);
  gtk_widget_set_halign (grid, GTK_ALIGN_START);
  gtk_widget_set_valign (grid, GTK_ALIGN_START);
  GtkWidget *window = drive_window (grid, 500, 400);
  g_signal_connect (grid, "activated", G_CALLBACK (on_tile), NULL);
  int cw = gtk_widget_get_width (grid) / 4, ch = gtk_widget_get_height (grid) / 2;

  tile_activated = -1;
  drive_click (grid, cw * 2 + cw / 2.0, ch / 2.0);
  g_assert_cmpint (tile_activated, ==, 2);
  drive_click (grid, cw / 2.0, ch + ch / 2.0);
  g_assert_cmpint (tile_activated, ==, 4);

  omarchy_tile_grid_set_cursor_index (g, 1);
  gtk_widget_grab_focus (grid);
  drive_key (window, NULL, GDK_KEY_l, 0);
  g_assert_cmpint (omarchy_tile_grid_get_cursor_index (g), ==, 2);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpint (omarchy_tile_grid_get_cursor_index (g), ==, 5); /* the short line's last tile */
  drive_key (window, NULL, GDK_KEY_k, 0);
  g_assert_cmpint (omarchy_tile_grid_get_cursor_index (g), ==, 1);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpint (tile_activated, ==, 1);

  /* RTL mirrors Left and Right. */
  gtk_widget_set_direction (grid, GTK_TEXT_DIR_RTL);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_cmpint (omarchy_tile_grid_get_cursor_index (g), ==, 2);
  omarchy_tile_grid_set_cursor_index (g, -1);
  g_assert_cmpint (omarchy_tile_grid_get_cursor_index (g), ==, -1);
  drive_close (window);
}

/* ---- the small pieces -------------------------------------------------------------------- */

TEST (test_avatar_falls_back, "/lists/avatar")
{
  g_autofree char *a = omarchy_avatar_initial ("  ada");
  g_assert_cmpstr (a, ==, "A");
  g_autofree char *e = omarchy_avatar_initial ("élan");
  g_assert_cmpstr (e, ==, "É");
  g_autofree char *none = omarchy_avatar_initial ("");
  g_assert_cmpstr (none, ==, "");

  GtkWidget *av = omarchy_avatar_new ("Missing");
  OmarchyAvatar *avatar = OMARCHY_AVATAR (av);
  omarchy_avatar_set_source (avatar, "/nonexistent/missing.png");
  omarchy_avatar_set_icon_name (avatar, "no-such-app-anywhere");
  GtkWidget *window = drive_window (av, 100, 100);
  g_assert_false (omarchy_avatar_get_pictured (avatar)); /* a source that fails gives way */
  g_assert_false (omarchy_avatar_get_iconed (avatar));
  OmarchyTheme *t = omarchy_theme_get_default ();
  int w;
  gtk_widget_measure (av, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &w, NULL, NULL);
  g_assert_cmpint (w, ==, omarchy_theme_get_font_size (t, "displayLarge"));
  omarchy_avatar_set_size (avatar, "small");
  gtk_widget_measure (av, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &w, NULL, NULL);
  g_assert_cmpint (w, ==, omarchy_theme_get_font_size (t, "iconLarge"));
  omarchy_avatar_set_size (avatar, "large");
  gtk_widget_measure (av, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &w, NULL, NULL);
  g_assert_cmpint (w, ==, omarchy_theme_get_font_size (t, "display") * 2);
  if (g_file_test ("qt/gallery/assets/avatar.png", G_FILE_TEST_EXISTS)) {
    omarchy_avatar_set_source (avatar, "qt/gallery/assets/avatar.png");
    g_assert_true (omarchy_avatar_get_pictured (avatar));
  }
  drive_close (window);
}

TEST (test_badge_dot_key_value_hero_sizes, "/lists/pieces")
{
  OmarchyTheme *t = omarchy_theme_get_default ();
  GtkWidget *b = omarchy_badge_new ("3");
  int w, h;
  gtk_widget_measure (b, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &w, NULL, NULL);
  gtk_widget_measure (b, GTK_ORIENTATION_VERTICAL, -1, NULL, &h, NULL, NULL);
  g_assert_cmpint (w, >=, h); /* never narrower than tall */
  omarchy_badge_set_text (OMARCHY_BADGE (b), "");
  gtk_widget_measure (b, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &w, NULL, NULL);
  g_assert_cmpint (w, ==, h);
  g_object_ref_sink (b);
  g_object_unref (b);

  GtkWidget *d = omarchy_status_dot_new ("urgent");
  gtk_widget_measure (d, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &w, NULL, NULL);
  int ring = MAX (1, omarchy_theme_space (t, 2));
  g_assert_cmpint (w, ==, omarchy_theme_space (t, 8) + ring * 2);
  g_object_ref_sink (d);
  g_object_unref (d);

  GtkWidget *kv = omarchy_key_value_new ();
  omarchy_key_value_set_columns (OMARCHY_KEY_VALUE (kv), 2);
  omarchy_key_value_append (OMARCHY_KEY_VALUE (kv), "Ping", "18 ms");
  omarchy_key_value_append_toned (OMARCHY_KEY_VALUE (kv), "Packet loss", "4%", "urgent");
  omarchy_key_value_append (OMARCHY_KEY_VALUE (kv), "Receiving", "1.2 MB/s");
  gtk_widget_measure (kv, GTK_ORIENTATION_VERTICAL, -1, NULL, &h, NULL, NULL);
  int one = h;
  omarchy_key_value_set_columns (OMARCHY_KEY_VALUE (kv), 1);
  gtk_widget_measure (kv, GTK_ORIENTATION_VERTICAL, -1, NULL, &h, NULL, NULL);
  g_assert_cmpint (h, >, one); /* three lines instead of two */
  g_assert_cmpuint (omarchy_key_value_get_n_pairs (OMARCHY_KEY_VALUE (kv)), ==, 3);
  g_object_ref_sink (kv);
  g_object_unref (kv);

  GtkWidget *hero = omarchy_hero_new ("󰂯", "Bluetooth");
  omarchy_hero_set_status (OMARCHY_HERO (hero), "No adapter found");
  GtkWidget *sw = omarchy_toggle_switch_new ();
  omarchy_hero_set_trailing (OMARCHY_HERO (hero), sw);
  int with, without;
  gtk_widget_measure (hero, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &with, NULL, NULL);
  omarchy_hero_set_trailing (OMARCHY_HERO (hero), NULL);
  gtk_widget_measure (hero, GTK_ORIENTATION_HORIZONTAL, -1, NULL, &without, NULL, NULL);
  g_assert_cmpint (with, >, without);
  g_object_ref_sink (hero);
  g_object_unref (hero);
}
