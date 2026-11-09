/* lists: a ScrollArea 420 wide capped at 340, rows `xxs` apart, driven by a CursorModel (one
 * section `rows` of 9, cursor on row 1; the keys and the pointer move it in the live gallery);
 * rows with trailing glyphs that stay; two Heroes; a KeyValue two pairs to a line
 * (spec/components/lists.md, qt/gallery/sections/lists.qml). */
#include "gallery.h"

static GtkWidget *
row (const char *title, gunichar glyph, const char *subtitle)
{
  GtkWidget *r = omarchy_list_row_new (title);
  if (glyph)
    omarchy_list_row_set_glyph (OMARCHY_LIST_ROW (r), gallery_glyph (glyph));
  if (subtitle)
    omarchy_list_row_set_subtitle (OMARCHY_LIST_ROW (r), subtitle);
  return r;
}

static void
reveal (OmarchyCursorModel *model, const char *section, int index, gboolean by_key, OmarchyScrollArea *scroll)
{
  if (!by_key)
    return;
  GtkWidget *child = gtk_widget_get_first_child (omarchy_scroll_area_get_content (scroll));
  for (int i = 0; child && i < index; i++)
    child = gtk_widget_get_next_sibling (child);
  omarchy_scroll_area_ensure_visible (scroll, child);
}

static GtkWidget *
cursor_list (GtkWidget *section)
{
  GtkWidget *scroll = omarchy_scroll_area_new ();
  OmarchyScrollArea *sa = OMARCHY_SCROLL_AREA (scroll);
  omarchy_scroll_area_set_max_height (sa, 340);
  omarchy_scroll_area_set_spacing_token (sa, "xxs");

  GtkWidget *rows[9];
  rows[0] = row ("Coffee Shop", 0xF0928, "WPA2");
  omarchy_list_row_set_value (OMARCHY_LIST_ROW (rows[0]), "5 GHz");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (rows[0]), gallery_glyph (0xF033E));

  rows[1] = row ("Basecamp Guest", 0xF0925, "Open");

  rows[2] = row ("Basecamp", 0xF0928, NULL);
  omarchy_list_row_set_status (OMARCHY_LIST_ROW (rows[2]), "Connected", "success");
  omarchy_list_row_set_current (OMARCHY_LIST_ROW (rows[2]), TRUE);

  rows[3] = row ("Neighbor 5G", 0xF091F, "Out of range");
  omarchy_list_row_set_disabled (OMARCHY_LIST_ROW (rows[3]), TRUE);

  rows[4] = row ("Messages", 0, "From Basecamp");
  GtkWidget *avatar = omarchy_avatar_new ("Messages");
  omarchy_avatar_set_size (OMARCHY_AVATAR (avatar), "small");
  omarchy_list_row_set_leading (OMARCHY_LIST_ROW (rows[4]), avatar);
  GtkWidget *badge = omarchy_badge_new ("3");
  omarchy_badge_set_tone (OMARCHY_BADGE (badge), "accent");
  omarchy_badge_set_filled (OMARCHY_BADGE (badge), TRUE);
  omarchy_list_row_set_trailing (OMARCHY_LIST_ROW (rows[4]), badge);

  rows[5] = row ("Headphones", 0xF00AF, "Battery 80%");
  GtkWidget *sw = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (sw), TRUE);
  omarchy_toggle_switch_set_interactive (OMARCHY_TOGGLE_SWITCH (sw), FALSE);
  omarchy_list_row_set_trailing (OMARCHY_LIST_ROW (rows[5]), sw);

  rows[6] = row ("Office", 0xF0922, "Saved");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (rows[6]), gallery_glyph (0xF033E));
  omarchy_list_row_set_action_glyph (OMARCHY_LIST_ROW (rows[6]), gallery_glyph (0xF0159));
  omarchy_list_row_set_action_urgent (OMARCHY_LIST_ROW (rows[6]), TRUE);
  omarchy_list_row_set_action_cursor (OMARCHY_LIST_ROW (rows[6]), TRUE);

  /* Past the cap, so the bar shows. */
  rows[7] = row ("Library", 0xF0922, "WPA2");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (rows[7]), gallery_glyph (0xF033E));
  omarchy_list_row_set_action_glyph (OMARCHY_LIST_ROW (rows[7]), gallery_glyph (0xF0159));
  omarchy_list_row_set_action_urgent (OMARCHY_LIST_ROW (rows[7]), TRUE);

  rows[8] = row ("Airport Free WiFi", 0xF091F, "Open");

  /* The cursor model drives every row; the area takes the keys. */
  OmarchyCursorModel *cursor = omarchy_cursor_model_new ();
  omarchy_cursor_model_add_section (cursor, "rows", 9);
  omarchy_cursor_model_set_reference_item (cursor, section);
  for (int i = 0; i < 9; i++) {
    gtk_widget_set_focusable (rows[i], FALSE);
    omarchy_scroll_area_append (sa, rows[i]);
    omarchy_cursor_model_bind (cursor, rows[i], "rows", i);
  }
  omarchy_cursor_model_set (cursor, "rows", 1);
  g_signal_connect_object (cursor, "moved", G_CALLBACK (reveal), scroll, 0);
  omarchy_cursor_model_attach (cursor, scroll);
  g_object_unref (cursor);
  return gallery_width (scroll, 420);
}

static GtkWidget *
staying_glyphs (void)
{
  GtkWidget *col = gallery_column ("xxs");
  GtkWidget *a = row ("Appearance", 0xF03D8, "Theme, font, rounding");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (a), gallery_glyph (0xF0142));
  gallery_add (col, gallery_fill (a));
  GtkWidget *d = row ("Display", 0xF0379, "2560 × 1440");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (d), gallery_glyph (0xF0142));
  gallery_add (col, gallery_fill (d));
  GtkWidget *h = row ("Headphones", 0xF02CB, "Output");
  omarchy_list_row_set_current (OMARCHY_LIST_ROW (h), TRUE);
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (h), gallery_glyph (0xF012C));
  omarchy_list_row_set_trailing_tone (OMARCHY_LIST_ROW (h), "accent");
  gallery_add (col, gallery_fill (h));
  return gallery_width (col, 420);
}

GtkWidget *
gallery_section_lists (void)
{
  GtkWidget *section = gallery_section ("Lists");
  gallery_add (section, cursor_list (section));
  gallery_add (section, staying_glyphs ());

  GtkWidget *wifi = omarchy_hero_new (gallery_glyph (0xF0928), "Wi-Fi");
  omarchy_hero_set_status (OMARCHY_HERO (wifi), "Connected to Basecamp");
  GtkWidget *sw = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (sw), TRUE);
  omarchy_hero_set_trailing (OMARCHY_HERO (wifi), sw);
  gallery_add (section, gallery_width (wifi, 420));

  GtkWidget *bt = omarchy_hero_new (gallery_glyph (0xF00B2), "Bluetooth");
  omarchy_hero_set_status (OMARCHY_HERO (bt), "No adapter found");
  omarchy_hero_set_error (OMARCHY_HERO (bt), TRUE);
  omarchy_hero_set_glyph_opacity (OMARCHY_HERO (bt), 0.5);
  gallery_add (section, gallery_width (bt, 420));

  GtkWidget *kv = omarchy_key_value_new ();
  OmarchyKeyValue *k = OMARCHY_KEY_VALUE (kv);
  omarchy_key_value_set_columns (k, 2);
  omarchy_key_value_append (k, "Ping", "18 ms");
  omarchy_key_value_append_toned (k, "Packet loss", "4%", "urgent");
  omarchy_key_value_append (k, "Receiving", "1.2 MB/s");
  omarchy_key_value_append (k, "Sending", "240 KB/s");
  omarchy_key_value_append (k, "IP address", "192.168.1.24");
  omarchy_key_value_append (k, "Gateway", "192.168.1.1");
  gallery_add (section, kv);
  return section;
}
