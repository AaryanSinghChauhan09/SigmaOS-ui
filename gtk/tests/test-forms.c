/* Forms and pickers: Checkbox, Radio, RadioGroup, PasswordField, TextArea, MultiSelect,
 * ColorWell, SwatchStrip and Dialog, by click and by key (qt/tests/tst_forms.qml and more). */
#include <omarchy-gtk.h>

#include "drive.h"

static int fired;
static char *last_value;
static int last_index = -1;

static void
count (gpointer a, gpointer b)
{
  fired++;
}

static int
spacing (const char *token)
{
  return omarchy_theme_get_spacing (omarchy_theme_get_default (), token);
}

static void
assert_values (OmarchyMultiSelect *ms, const char *const *want, const char *what)
{
  g_auto (GStrv) got = omarchy_multi_select_get_values (ms);
  g_autofree char *a = g_strjoinv (",", got);
  g_autofree char *b = g_strjoinv (",", (char **) want);
  if (g_strcmp0 (a, b))
    g_error ("%s: values [%s], want [%s]", what, a, b);
}

/* ---- Checkbox, Radio, RadioGroup ---- */

TEST (test_checkbox_flips_on_click_and_keys_unless_disabled, "/forms/checkbox")
{
  GtkWidget *box = omarchy_checkbox_new ("Show hidden files");
  gtk_widget_set_halign (box, GTK_ALIGN_START);
  GtkWidget *window = drive_window (box, 300, 80);
  fired = 0;
  g_signal_connect (box, "toggled", G_CALLBACK (count), NULL);
  drive_click (box, -1, -1);
  g_assert_true (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (box)));
  gtk_widget_grab_focus (box);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_false (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (box)));
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_true (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (box)));
  g_assert_cmpint (fired, ==, 3);
  gtk_widget_set_sensitive (box, FALSE);
  drive_click (box, -1, -1);
  g_assert_true (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (box)));
  g_assert_cmpint (fired, ==, 3);
  drive_close (window);
}

TEST (test_checkbox_wraps_its_description, "/forms/checkbox-metrics")
{
  GtkWidget *box = omarchy_checkbox_new ("Twelve-hour clock");
  int one;
  gtk_widget_measure (box, GTK_ORIENTATION_VERTICAL, 400, NULL, &one, NULL, NULL);
  omarchy_checkbox_set_description (OMARCHY_CHECKBOX (box), "Show AM and PM after the time, as most of the world does not");
  int wide, narrow;
  gtk_widget_measure (box, GTK_ORIENTATION_VERTICAL, 800, NULL, &wide, NULL, NULL);
  gtk_widget_measure (box, GTK_ORIENTATION_VERTICAL, 120, NULL, &narrow, NULL, NULL);
  g_assert_cmpint (wide, >, one);
  g_assert_cmpint (narrow, >, wide);
  g_object_ref_sink (box);
  g_object_unref (box);
}

TEST (test_radio_checks_and_stays_checked, "/forms/radio")
{
  GtkWidget *radio = omarchy_radio_new ("Top");
  g_assert_true (omarchy_checkbox_get_exclusive (OMARCHY_CHECKBOX (radio)));
  gtk_widget_set_halign (radio, GTK_ALIGN_START);
  GtkWidget *window = drive_window (radio, 200, 60);
  fired = 0;
  g_signal_connect (radio, "toggled", G_CALLBACK (count), NULL);
  drive_click (radio, -1, -1);
  g_assert_true (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (radio)));
  gtk_widget_grab_focus (radio);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_true (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (radio)));
  g_assert_cmpint (fired, ==, 2);
  drive_close (window);
}

static void
on_string (GtkWidget *w, const char *value, gpointer data)
{
  g_free (last_value);
  last_value = g_strdup (value);
  fired++;
}

TEST (test_radios_choose_on_click_and_keys, "/forms/radio-group")
{
  GtkWidget *group = omarchy_radio_group_new ();
  OmarchyRadioGroup *g = OMARCHY_RADIO_GROUP (group);
  omarchy_radio_group_append (g, "top", "Top", NULL);
  omarchy_radio_group_append (g, "bottom", "Bottom", NULL);
  omarchy_radio_group_append (g, "hidden", "Hidden", NULL);
  omarchy_radio_group_set_value (g, "bottom");
  gtk_widget_set_halign (group, GTK_ALIGN_START);
  gtk_widget_set_valign (group, GTK_ALIGN_START);
  GtkWidget *window = drive_window (group, 300, 200);
  fired = 0;
  g_signal_connect (group, "changed", G_CALLBACK (on_string), NULL);
  GtkWidget *hidden = drive_find (group, OMARCHY_TYPE_RADIO, 2);
  g_assert_cmpstr (omarchy_checkbox_get_label (OMARCHY_CHECKBOX (hidden)), ==, "Hidden");
  drive_click (hidden, -1, -1);
  g_assert_cmpstr (omarchy_radio_group_get_value (g), ==, "hidden");
  g_assert_cmpstr (last_value, ==, "hidden");
  g_assert_true (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (hidden)));
  drive_click (drive_find (group, OMARCHY_TYPE_RADIO, 0), -1, -1);
  g_assert_cmpstr (omarchy_radio_group_get_value (g), ==, "top");
  g_assert_false (omarchy_checkbox_get_checked (OMARCHY_CHECKBOX (hidden)));
  gtk_widget_grab_focus (group);
  const struct {
    guint key;
    const char *value;
  } steps[] = { { GDK_KEY_Down, "bottom" }, { GDK_KEY_j, "hidden" }, { GDK_KEY_Down, "hidden" }, { GDK_KEY_k, "bottom" },
                { GDK_KEY_Up, "top" },      { GDK_KEY_Up, "top" },    { GDK_KEY_Right, "bottom" }, { GDK_KEY_Left, "top" } };
  for (guint i = 0; i < G_N_ELEMENTS (steps); i++) {
    drive_key (window, NULL, steps[i].key, 0);
    g_assert_cmpstr (omarchy_radio_group_get_value (g), ==, steps[i].value);
  }
  g_assert_cmpint (fired, ==, 2 + 6); /* two clicks, six moves (two at an end) */
  drive_close (window);
}

/* ---- PasswordField, TextArea ---- */

TEST (test_password_field_reveals_and_accepts, "/forms/password-field")
{
  GtkWidget *p = omarchy_password_field_new ();
  OmarchyPasswordField *pf = OMARCHY_PASSWORD_FIELD (p);
  gtk_editable_set_text (GTK_EDITABLE (p), "hunter22");
  omarchy_password_field_set_revealable (pf, TRUE);
  gtk_widget_set_valign (p, GTK_ALIGN_START);
  GtkWidget *window = drive_window (p, 240, 80);
  GtkText *text = omarchy_password_field_get_text_widget (pf);
  g_assert_false (gtk_text_get_visibility (text));
  /* The eye sits at the end of the field. */
  int pad = spacing ("controlPaddingX");
  drive_click (p, gtk_widget_get_width (p) - pad - 4, gtk_widget_get_height (p) / 2.0);
  g_assert_true (omarchy_password_field_get_revealed (pf));
  g_assert_true (gtk_text_get_visibility (text));
  /* Focus is the text's (GtkText takes its own clicks). */
  g_assert_true (gtk_widget_grab_focus (p));
  g_assert_true (gtk_widget_get_state_flags (p) & GTK_STATE_FLAG_FOCUS_WITHIN);
  fired = 0;
  g_signal_connect (p, "accepted", G_CALLBACK (count), NULL);
  g_signal_emit_by_name (text, "activate");
  g_assert_cmpint (fired, ==, 1);
  int h0 = gtk_widget_get_height (p);
  omarchy_password_field_set_error (pf, "Wrong password, try again");
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_height (p), >, h0);
  g_assert_cmpstr (gtk_editable_get_text (GTK_EDITABLE (p)), ==, "hunter22");
  drive_close (window);
}

TEST (test_text_area_grows_between_its_rows, "/forms/text-area")
{
  GtkWidget *a = omarchy_text_area_new ();
  OmarchyTextArea *ta = OMARCHY_TEXT_AREA (a);
  gtk_widget_set_valign (a, GTK_ALIGN_START);
  GtkWidget *window = drive_window (a, 300, 300);
  int three = gtk_widget_get_height (a);
  omarchy_text_area_set_text (ta, "one\ntwo");
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_height (a), ==, three);
  omarchy_text_area_set_text (ta, "1\n2\n3\n4\n5");
  drive_wait (20);
  int five = gtk_widget_get_height (a);
  g_assert_cmpint (five, >, three);
  omarchy_text_area_set_text (ta, "1\n2\n3\n4\n5\n6\n7\n8\n9");
  drive_wait (20);
  int most = gtk_widget_get_height (a);
  g_assert_cmpint (most, >, five);
  int line = (five - three) / 2;
  g_assert_cmpint (most - three, ==, 3 * line); /* capped at six rows */
  omarchy_text_area_set_max_rows (ta, 4);
  drive_wait (20);
  g_assert_cmpint (gtk_widget_get_height (a), <, most);
  /* Focus is the text view's: the field wears the focus look. */
  g_assert_true (gtk_widget_grab_focus (a));
  g_assert_true (gtk_widget_get_state_flags (a) & GTK_STATE_FLAG_FOCUS_WITHIN);
  g_autofree char *text = omarchy_text_area_get_text (ta);
  g_assert_cmpstr (text, ==, "1\n2\n3\n4\n5\n6\n7\n8\n9");
  drive_close (window);
}

/* ---- MultiSelect ---- */

static GtkWidget *
notify_for (void)
{
  GtkWidget *m = omarchy_multi_select_new ("Notify for");
  static const char *const options[] = { "Calendar", "Mail", "Messages", "Updates", "Battery", NULL };
  for (const char *const *o = options; *o; o++)
    omarchy_multi_select_append (OMARCHY_MULTI_SELECT (m), *o, NULL, NULL);
  static const char *const values[] = { "Mail", "Updates", NULL };
  omarchy_multi_select_set_values (OMARCHY_MULTI_SELECT (m), values);
  gtk_widget_set_size_request (m, 260, -1);
  gtk_widget_set_halign (m, GTK_ALIGN_START);
  gtk_widget_set_valign (m, GTK_ALIGN_START);
  return m;
}

/* The open list (the rows), and a row's center in it. */
static GtkWidget *
option_list (GtkWidget *ms)
{
  GtkWidget *list = drive_find (ms, g_type_from_name ("OmarchyMultiSelectList"), 0);
  g_assert_nonnull (list);
  return list;
}

static double
row_center (int i)
{
  return i * (spacing ("popupRowHeight") + spacing ("labelGap")) + spacing ("popupRowHeight") / 2.0;
}

TEST (test_multiselect_toggles_on_click, "/forms/multiselect-click")
{
  GtkWidget *m = notify_for ();
  OmarchyMultiSelect *ms = OMARCHY_MULTI_SELECT (m);
  GtkWidget *window = drive_window (m, 400, 400);
  /* The trigger, under the label. */
  drive_click (m, 40, gtk_widget_get_height (m) - 6);
  drive_wait (50);
  g_assert_true (omarchy_multi_select_get_popup_open (ms));
  GtkWidget *list = option_list (m);
  drive_click (list, 40, row_center (0));
  g_assert_true (omarchy_multi_select_is_selected (ms, "Calendar"));
  drive_click (list, 40, row_center (1));
  g_assert_false (omarchy_multi_select_is_selected (ms, "Mail"));
  static const char *const want[] = { "Updates", "Calendar", NULL };
  assert_values (ms, want, "after two clicks");
  /* A click on the trigger closes it. */
  drive_click (m, 40, gtk_widget_get_height (m) - 6);
  drive_wait (50);
  g_assert_false (omarchy_multi_select_get_popup_open (ms));
  drive_close (window);
}

TEST (test_multiselect_keys, "/forms/multiselect-keys")
{
  GtkWidget *m = notify_for ();
  OmarchyMultiSelect *ms = OMARCHY_MULTI_SELECT (m);
  GtkWidget *window = drive_window (m, 400, 400);
  fired = 0;
  g_signal_connect (m, "changed", G_CALLBACK (count), NULL);
  gtk_widget_grab_focus (m);
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_wait (50);
  g_assert_true (omarchy_multi_select_get_popup_open (ms));
  GtkWidget *search = omarchy_multi_select_get_search_field (ms);
  GtkWidget *focus = gtk_root_get_focus (GTK_ROOT (window));
  g_assert_true (focus && gtk_widget_is_ancestor (focus, search));
  /* Typing filters (GTK's own text input; set as typed). */
  gtk_editable_set_text (GTK_EDITABLE (search), "mes");
  drive_key (window, NULL, GDK_KEY_Return, 0);
  static const char *const one[] = { "Mail", "Updates", "Messages", NULL };
  assert_values (ms, one, "Return toggles the first match");
  gtk_editable_set_text (GTK_EDITABLE (search), "");
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpint (omarchy_multi_select_get_current_index (ms), ==, 0);
  drive_key (window, NULL, GDK_KEY_space, 0);
  static const char *const two[] = { "Mail", "Updates", "Messages", "Calendar", NULL };
  assert_values (ms, two, "Down into the list, Space toggles");
  drive_key (window, NULL, GDK_KEY_j, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  static const char *const three[] = { "Updates", "Messages", "Calendar", NULL };
  assert_values (ms, three, "j walks, Return toggles");
  g_assert_cmpint (fired, ==, 3);
  /* j stops at the last row; k off the first goes back to the search. */
  for (int i = 0; i < 8; i++)
    drive_key (window, NULL, GDK_KEY_j, 0);
  g_assert_cmpint (omarchy_multi_select_get_current_index (ms), ==, 4);
  for (int i = 0; i < 5; i++)
    drive_key (window, NULL, GDK_KEY_k, 0);
  focus = gtk_root_get_focus (GTK_ROOT (window));
  g_assert_true (focus && gtk_widget_is_ancestor (focus, search));
  drive_key (window, NULL, GDK_KEY_Escape, 0);
  drive_wait (50);
  g_assert_false (omarchy_multi_select_get_popup_open (ms));
  g_assert_cmpstr (gtk_editable_get_text (GTK_EDITABLE (search)), ==, "");
  drive_close (window);
}

TEST (test_multiselect_trigger_reads_the_choice, "/forms/multiselect-label")
{
  GtkWidget *m = notify_for ();
  OmarchyMultiSelect *ms = OMARCHY_MULTI_SELECT (m);
  g_object_ref_sink (m);
  static const char *const four[] = { "Calendar", "Mail", "Messages", "Updates", NULL };
  omarchy_multi_select_set_values (ms, four);
  g_auto (GStrv) got = omarchy_multi_select_get_values (ms);
  g_assert_cmpuint (g_strv_length (got), ==, 4);
  /* No options matching: the inline popup still opens, empty. */
  omarchy_multi_select_set_inline_popup (ms, TRUE);
  int closed;
  gtk_widget_measure (m, GTK_ORIENTATION_VERTICAL, 260, NULL, &closed, NULL, NULL);
  omarchy_multi_select_open (ms);
  int open;
  gtk_widget_measure (m, GTK_ORIENTATION_VERTICAL, 260, NULL, &open, NULL, NULL);
  g_assert_cmpint (open, >, closed);
  omarchy_multi_select_close (ms);
  g_assert_false (omarchy_multi_select_get_popup_open (ms));
  g_object_unref (m);
}

/* ---- ColorWell, SwatchStrip ---- */

TEST (test_color_well_clicks_and_keys, "/forms/color-well")
{
  GtkWidget *w = omarchy_color_well_new_for_role ("accent");
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  gtk_widget_set_valign (w, GTK_ALIGN_START);
  GtkWidget *window = drive_window (w, 200, 80);
  GdkRGBA c;
  omarchy_color_well_get_swatch (OMARCHY_COLOR_WELL (w), &c);
  GdkRGBA *accent = omarchy_theme_get_color (omarchy_theme_get_default (), "palette.accent");
  g_assert_true (gdk_rgba_equal (&c, accent));
  gdk_rgba_free (accent);
  fired = 0;
  g_signal_connect (w, "clicked", G_CALLBACK (count), NULL);
  drive_click (w, -1, -1);
  gtk_widget_grab_focus (w);
  drive_key (window, NULL, GDK_KEY_space, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_false (drive_key (window, NULL, GDK_KEY_a, 0));
  g_assert_cmpint (fired, ==, 3);
  drive_close (window);
}

static void
on_picked (GtkWidget *w, int index, gpointer data)
{
  last_index = index;
  fired++;
}

TEST (test_swatch_strip_picks_on_click_and_keys, "/forms/swatch-strip")
{
  GtkWidget *s = omarchy_swatch_strip_new ();
  OmarchySwatchStrip *ss = OMARCHY_SWATCH_STRIP (s);
  static const char *const roles[] = { "accent", "urgent", "warning", "success", "foreground" };
  for (guint i = 0; i < G_N_ELEMENTS (roles); i++)
    omarchy_swatch_strip_append_role (ss, roles[i]);
  omarchy_swatch_strip_set_current_index (ss, 2);
  gtk_widget_set_halign (s, GTK_ALIGN_START);
  gtk_widget_set_valign (s, GTK_ALIGN_START);
  GtkWidget *window = drive_window (s, 300, 60);
  fired = 0;
  g_signal_connect (s, "picked", G_CALLBACK (on_picked), NULL);
  int cell = gtk_widget_get_height (s);
  drive_click (s, 3 * (cell + spacing ("xxs")) + cell / 2.0, cell / 2.0);
  g_assert_cmpint (last_index, ==, 3);
  g_assert_cmpint (omarchy_swatch_strip_get_current_index (ss), ==, 3);
  gtk_widget_grab_focus (s);
  drive_key (window, NULL, GDK_KEY_l, 0);
  g_assert_cmpint (omarchy_swatch_strip_get_cursor_index (ss), ==, 4);
  drive_key (window, NULL, GDK_KEY_Right, 0);
  g_assert_cmpint (omarchy_swatch_strip_get_cursor_index (ss), ==, 4);
  drive_key (window, NULL, GDK_KEY_h, 0);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_cmpint (omarchy_swatch_strip_get_cursor_index (ss), ==, 2);
  g_assert_cmpint (omarchy_swatch_strip_get_current_index (ss), ==, 3);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_cmpint (omarchy_swatch_strip_get_current_index (ss), ==, 2);
  g_assert_cmpint (last_index, ==, 2);
  g_assert_cmpint (fired, ==, 2);
  drive_close (window);
}

/* ---- Dialog ---- */

static int rejected, accepted;
static void on_reject (GtkWidget *w, gpointer d) { rejected++; }
static void on_accept (GtkWidget *w, gpointer d) { accepted++; }

TEST (test_dialog_keys_and_clicks, "/forms/dialog")
{
  GtkWidget *d = omarchy_dialog_new ("Discard changes?", "Your edits will be lost.");
  OmarchyDialog *dlg = OMARCHY_DIALOG (d);
  g_assert_false (gtk_widget_get_visible (d));
  g_assert_cmpstr (omarchy_dialog_get_secondary_text (dlg), ==,
                   omarchy_theme_translate (omarchy_theme_get_default (), "common.cancel", "Cancel"));
  omarchy_dialog_set_opened (dlg, TRUE);
  g_assert_true (gtk_widget_get_visible (d));
  GtkWidget *window = drive_window (d, 420, 300);
  gtk_widget_grab_focus (d);
  rejected = accepted = 0;
  g_signal_connect (d, "rejected", G_CALLBACK (on_reject), NULL);
  g_signal_connect (d, "accepted", G_CALLBACK (on_accept), NULL);
  g_assert_cmpint (omarchy_dialog_get_selected_index (dlg), ==, 1);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpint (accepted, ==, 1);
  drive_key (window, NULL, GDK_KEY_h, 0);
  g_assert_cmpint (omarchy_dialog_get_selected_index (dlg), ==, 0);
  drive_key (window, NULL, GDK_KEY_Return, 0);
  drive_key (window, NULL, GDK_KEY_Escape, 0);
  g_assert_cmpint (rejected, ==, 2);
  drive_key (window, NULL, GDK_KEY_Tab, 0);
  g_assert_cmpint (omarchy_dialog_get_selected_index (dlg), ==, 1);
  /* A press on the scrim rejects; on the card, nothing. */
  drive_click (d, 4, 4);
  g_assert_cmpint (rejected, ==, 3);
  drive_click (d, -1, -1);
  g_assert_cmpint (rejected + accepted, ==, 4);
  /* The primary button: bottom right of the centered card, `space(18)` + border inside it. */
  int W = gtk_widget_get_width (d), H = gtk_widget_get_height (d);
  int cw = MIN (W - omarchy_theme_space (omarchy_theme_get_default (), 32),
                omarchy_theme_space (omarchy_theme_get_default (), 370));
  double x = (W + cw) / 2.0 - 20 - omarchy_theme_space (omarchy_theme_get_default (), 88) / 2.0;
  /* The card is about 137 tall here (title, message, buttons); aim at the buttons' middle. */
  double y = H / 2.0 + 137 / 2.0 - 20 - omarchy_theme_space (omarchy_theme_get_default (), 34) / 2.0;
  drive_click (d, x, y);
  g_assert_cmpint (accepted, ==, 2);
  /* With the dialog closed, keys mean nothing. */
  omarchy_dialog_set_opened (dlg, FALSE);
  g_assert_false (omarchy_dialog_handle_key (dlg, GDK_KEY_Escape, 0));
  drive_close (window);
}
