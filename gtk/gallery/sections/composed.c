/* composed: real UIs built only from kit widgets, the gallery adding nothing but layout. A Wi-Fi
 * panel (Card) and an app inspector (Pane) in one row, two SearchLists in the next, rows
 * `huge × 2` apart and top-aligned (spec/components/composed.md, qt/gallery/sections/composed.qml). */
#include "gallery.h"

static void
respace_double (GtkWidget *row)
{
  gtk_box_set_spacing (GTK_BOX (row), omarchy_theme_get_spacing (omarchy_theme_get_default (), "huge") * 2);
}

static GtkWidget *
double_huge_row (void)
{
  GtkWidget *row = gallery_row ("huge");
  g_object_set_data (G_OBJECT (row), "gallery-spacing", NULL);
  respace_double (row);
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (respace_double), row, G_CONNECT_SWAPPED);
  return row;
}

static GtkWidget *
network (const char *title, gunichar glyph, const char *subtitle)
{
  GtkWidget *r = omarchy_list_row_new (title);
  omarchy_list_row_set_glyph (OMARCHY_LIST_ROW (r), gallery_glyph (glyph));
  if (subtitle)
    omarchy_list_row_set_subtitle (OMARCHY_LIST_ROW (r), subtitle);
  return r;
}

/* A Wi-Fi panel, as the shell's network panel. */
static GtkWidget *
wifi_panel (void)
{
  GtkWidget *card = omarchy_card_new (NULL);
  OmarchyCard *c = OMARCHY_CARD (card);

  GtkWidget *hero = omarchy_hero_new (gallery_glyph (0xF0928), "Wi-Fi");
  omarchy_hero_set_status (OMARCHY_HERO (hero), "Connected to Basecamp");
  GtkWidget *on = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (on), TRUE);
  omarchy_hero_set_trailing (OMARCHY_HERO (hero), on);
  omarchy_card_append (c, hero);

  omarchy_card_append (c, omarchy_section_header_new ("Networks"));

  GtkWidget *scroll = omarchy_scroll_area_new ();
  OmarchyScrollArea *sa = OMARCHY_SCROLL_AREA (scroll);
  omarchy_scroll_area_set_max_height (sa, 280);
  omarchy_scroll_area_set_spacing_token (sa, "xxs");

  GtkWidget *basecamp = network ("Basecamp", 0xF0928, NULL);
  omarchy_list_row_set_status (OMARCHY_LIST_ROW (basecamp), "Connected", "success");
  omarchy_list_row_set_current (OMARCHY_LIST_ROW (basecamp), TRUE);
  omarchy_scroll_area_append (sa, basecamp);

  GtkWidget *guest = network ("Basecamp Guest", 0xF0925, "Open");
  omarchy_list_row_set_action_glyph (OMARCHY_LIST_ROW (guest), gallery_glyph (0xF0159));
  omarchy_list_row_set_action_urgent (OMARCHY_LIST_ROW (guest), TRUE);
  omarchy_list_row_set_has_cursor (OMARCHY_LIST_ROW (guest), TRUE);
  omarchy_scroll_area_append (sa, guest);

  GtkWidget *coffee = network ("Coffee Shop", 0xF0922, "WPA2");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (coffee), gallery_glyph (0xF033E));
  GtkWidget *join = gallery_row ("md");
  gtk_widget_set_halign (join, GTK_ALIGN_FILL);
  GtkWidget *password = omarchy_password_field_new ();
  omarchy_password_field_set_placeholder_text (OMARCHY_PASSWORD_FIELD (password), "Password");
  gtk_widget_set_hexpand (password, TRUE);
  gallery_add (join, password);
  GtkWidget *connect = omarchy_button_new ("Connect");
  omarchy_button_set_bordered (OMARCHY_BUTTON (connect), TRUE);
  gallery_add (join, connect);
  omarchy_list_row_set_expansion (OMARCHY_LIST_ROW (coffee), join);
  omarchy_scroll_area_append (sa, coffee);

  GtkWidget *library = network ("Library", 0xF091F, "Saved");
  omarchy_list_row_set_trailing_glyph (OMARCHY_LIST_ROW (library), gallery_glyph (0xF033E));
  omarchy_list_row_set_action_glyph (OMARCHY_LIST_ROW (library), gallery_glyph (0xF0159));
  omarchy_list_row_set_action_urgent (OMARCHY_LIST_ROW (library), TRUE);
  omarchy_scroll_area_append (sa, library);
  omarchy_card_append (c, scroll);

  omarchy_card_append (c, omarchy_separator_new ());

  GtkWidget *kv = omarchy_key_value_new ();
  omarchy_key_value_append (OMARCHY_KEY_VALUE (kv), "IP", "192.168.1.24");
  omarchy_key_value_append (OMARCHY_KEY_VALUE (kv), "Gateway", "192.168.1.1");
  omarchy_key_value_append (OMARCHY_KEY_VALUE (kv), "DNS", "1.1.1.1");
  gtk_widget_set_halign (kv, GTK_ALIGN_START);
  omarchy_card_append (c, kv);
  return gallery_top (gallery_width (card, 360));
}

static GtkWidget *
setting (const char *label, GtkWidget *control, gboolean cursor)
{
  GtkWidget *row = omarchy_setting_row_new (label, NULL, control);
  omarchy_setting_row_set_cursor (OMARCHY_SETTING_ROW (row), cursor);
  return row;
}

/* An app inspector, as omareel's side panel: a pane on the window, ruled off from the canvas. */
static GtkWidget *
inspector (void)
{
  GtkWidget *pane = omarchy_pane_new ("Recording 1");
  OmarchyPane *p = OMARCHY_PANE (pane);
  omarchy_pane_set_subtitle (p, "1920 × 1080 · 0:42");
  omarchy_pane_set_divider (p, "left");
  omarchy_pane_add_action (p, omarchy_icon_button_new (gallery_glyph (0xF0E2)));
  omarchy_pane_add_action (p, omarchy_icon_button_new (gallery_glyph (0xF01E)));
  GtkWidget *export = omarchy_button_new ("Export");
  omarchy_button_set_bordered (OMARCHY_BUTTON (export), TRUE);
  omarchy_pane_add_action (p, export);

  const char *tabs[] = { "Shape", "Cursor", "Zoom", NULL };
  GtkWidget *t = omarchy_tabs_new (tabs);
  omarchy_tabs_set_current_index (OMARCHY_TABS (t), 0);
  omarchy_pane_append (p, t);

  omarchy_pane_append (p, omarchy_section_header_new ("Frame"));
  GtkWidget *frame = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (frame), setting ("Radius", gallery_width (omarchy_slider_new (0.4), 110), FALSE));
  GtkWidget *border = omarchy_dropdown_new (NULL);
  omarchy_dropdown_append (OMARCHY_DROPDOWN (border), "solid", "Solid");
  omarchy_dropdown_append (OMARCHY_DROPDOWN (border), "none", "None");
  omarchy_dropdown_set_value (OMARCHY_DROPDOWN (border), "solid");
  gtk_box_append (GTK_BOX (frame), setting ("Border", gallery_width (border, 110), FALSE));
  GtkWidget *shadow = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (shadow), TRUE);
  gtk_box_append (GTK_BOX (frame), setting ("Shadow", shadow, TRUE));
  omarchy_pane_append (p, frame);

  omarchy_pane_append (p, omarchy_section_header_new ("Background"));
  GtkWidget *background = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (background), setting ("Fill", omarchy_color_well_new_for_role ("accent"), FALSE));
  GtkWidget *padding = omarchy_number_field_new (NULL, 0, 96);
  omarchy_number_field_set_value (OMARCHY_NUMBER_FIELD (padding), 24);
  gtk_box_append (GTK_BOX (background), setting ("Padding", padding, FALSE));
  omarchy_pane_append (p, background);
  return gallery_top (gallery_width (pane, 300));
}

static const struct {
  gunichar glyph;
  const char *title, *subtitle, *detail;
} search_items[] = {
  { 0xF120, "Terminal", "App", "Open a new terminal window." },
  { 0xF031, "Terminal font", "Style", "Pick the monospace font every terminal uses." },
  { 0xF1FC, "Terminal theme", "Style", "Match the terminal colors to the desktop theme." },
  { 0xF0A3, "Intermediate CA", "Certificate", "Trust a certificate authority for this machine." },
};

/* A SearchList with `query` typed over the four items, filtered by matches() as a caller does. */
static GtkWidget *
search (const char *query, int width, int height, gboolean detail)
{
  GtkWidget *w = omarchy_search_list_new ();
  OmarchySearchList *list = OMARCHY_SEARCH_LIST (w);
  omarchy_search_list_set_query (list, query);
  omarchy_search_list_set_show_detail (list, detail);
  for (guint i = 0; i < G_N_ELEMENTS (search_items); i++)
    if (omarchy_search_list_matches (search_items[i].title, query))
      omarchy_search_list_append (list, search_items[i].title, search_items[i].subtitle, gallery_glyph (search_items[i].glyph),
                                  NULL, search_items[i].detail);
  gtk_widget_set_size_request (w, width, height);
  return gallery_top (w);
}

GtkWidget *
gallery_section_composed (void)
{
  GtkWidget *section = gallery_section ("Composed");

  GtkWidget *row = double_huge_row ();
  gallery_add (row, wifi_panel ());
  gallery_add (row, inspector ());
  gallery_add (section, row);

  row = double_huge_row ();
  /* "term": four results, the second under the cursor, and the detail pane. */
  GtkWidget *term = search ("term", 360, 300, TRUE);
  omarchy_cursor_model_set (omarchy_search_list_get_cursor (OMARCHY_SEARCH_LIST (term)), "results", 1);
  gallery_add (row, term);
  /* Nothing matches "zzz". */
  gallery_add (row, search ("zzz", 300, 200, FALSE));
  gallery_add (section, row);
  return section;
}
