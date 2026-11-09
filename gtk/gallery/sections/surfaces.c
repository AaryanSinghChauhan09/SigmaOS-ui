/* surfaces: Card "Network" 300 wide (Toggle "Wi-Fi" / "Basecamp" on, Separator, MenuRows
 * Basecamp checked, Basecamp Guest highlighted, Coffee Shop); beside it a Tooltip "Restart
 * Wi-Fi" and a ConfirmDialog "Remove the Coffee Shop network?" in a 340 × 170 box. Then Panes
 * and Headings (spec/components/surfaces.md). (qt/gallery/sections/surfaces.qml) */
#include "gallery.h"

static GtkWidget *
menu_row (const char *text, gboolean checked, gboolean highlighted)
{
  GtkWidget *r = omarchy_menu_row_new (text);
  omarchy_menu_row_set_checked (OMARCHY_MENU_ROW (r), checked);
  omarchy_menu_row_set_highlighted (OMARCHY_MENU_ROW (r), highlighted);
  return r;
}

static GtkWidget *
heading (const char *text, const char *subtitle, const char *level, gboolean centered)
{
  GtkWidget *h = omarchy_heading_new (text);
  if (subtitle)
    omarchy_heading_set_subtitle (OMARCHY_HEADING (h), subtitle);
  omarchy_heading_set_level (OMARCHY_HEADING (h), level);
  omarchy_heading_set_centered (OMARCHY_HEADING (h), centered);
  return gallery_fill (h);
}

static GtkWidget *
panes_and_headings (void)
{
  GtkWidget *rows = gallery_column ("xxl");

  GtkWidget *row = gallery_row ("huge");
  GtkWidget *layers = omarchy_pane_new ("Layers");
  omarchy_pane_set_subtitle (OMARCHY_PANE (layers), "3 items");
  omarchy_pane_set_divider (OMARCHY_PANE (layers), "left");
  omarchy_pane_add_action (OMARCHY_PANE (layers), omarchy_icon_button_new (gallery_glyph (0xF067)));
  GtkWidget *list = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (list), menu_row ("Background", FALSE, FALSE));
  gtk_box_append (GTK_BOX (list), menu_row ("Title", TRUE, FALSE));
  gtk_box_append (GTK_BOX (list), menu_row ("Logo", FALSE, FALSE));
  omarchy_pane_append (OMARCHY_PANE (layers), list);
  gallery_add (row, gallery_top (gallery_width (layers, 240)));

  GtkWidget *night = omarchy_pane_new ("Night light");
  omarchy_pane_set_subtitle (OMARCHY_PANE (night), "Sunset to sunrise");
  omarchy_pane_set_bordered (OMARCHY_PANE (night), TRUE);
  GtkWidget *on = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (on), TRUE);
  omarchy_pane_add_action (OMARCHY_PANE (night), on);
  omarchy_pane_append (OMARCHY_PANE (night),
                       omarchy_setting_row_new ("Strength", NULL, gallery_width (omarchy_slider_new (0.6), 100)));
  gallery_add (row, gallery_top (gallery_width (night, 240)));
  gallery_add (rows, row);

  row = gallery_row ("huge");
  GtkWidget *left = gallery_column ("huge");
  gallery_add (left, heading ("Settings", "Omarchy", "display", FALSE));
  gallery_add (left, heading ("Appearance", "Theme, font and rounding", "heading", FALSE));
  gallery_add (row, gallery_top (gallery_width (left, 240)));
  GtkWidget *right = gallery_column ("huge");
  GtkWidget *frame = heading ("Frame", NULL, "title", FALSE);
  omarchy_heading_add_trailing (OMARCHY_HEADING (frame), omarchy_icon_button_new (gallery_glyph (0xF141)));
  gallery_add (right, frame);
  gallery_add (right, heading ("notes.md", "~/Documents", "title", TRUE));
  gallery_add (row, gallery_top (gallery_width (right, 240)));
  gallery_add (rows, row);
  return rows;
}

GtkWidget *
gallery_section_surfaces (void)
{
  GtkWidget *section = gallery_section ("Surfaces");
  GtkWidget *row = gallery_row ("huge");

  GtkWidget *card = omarchy_card_new ("Network");
  GtkWidget *wifi = omarchy_toggle_new ("Wi-Fi", "Basecamp");
  omarchy_toggle_set_checked (OMARCHY_TOGGLE (wifi), TRUE);
  omarchy_card_append (OMARCHY_CARD (card), wifi);
  omarchy_card_append (OMARCHY_CARD (card), omarchy_separator_new ());
  GtkWidget *rows = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (rows), menu_row ("Basecamp", TRUE, FALSE));
  gtk_box_append (GTK_BOX (rows), menu_row ("Basecamp Guest", FALSE, TRUE));
  gtk_box_append (GTK_BOX (rows), menu_row ("Coffee Shop", FALSE, FALSE));
  omarchy_card_append (OMARCHY_CARD (card), rows);
  gallery_add (row, gallery_top (gallery_width (card, 300)));

  GtkWidget *col = gallery_column ("huge");
  gallery_add (col, omarchy_tooltip_new ("Restart Wi-Fi"));
  gallery_add (col, gallery_size (omarchy_confirm_dialog_new ("Remove the Coffee Shop network?"), 340, 170));
  gallery_add (row, gallery_top (col));
  gallery_add (section, row);

  gallery_add (section, panes_and_headings ());
  return section;
}
