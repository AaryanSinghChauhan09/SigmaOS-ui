/* navigation: a Toolbar (three groups split by separators, the cursor on Italic, "notes.md"
 * centered, a bordered "Share" and the overflow), Tabs (Display selected, Audio under the
 * cursor), and an IconRail (Search selected) beside a `space(320)` placeholder naming the
 * selected panel (spec/components/navigation.md). (qt/gallery/sections/navigation.qml) */
#include "gallery.h"

static GtkWidget *
icon (gunichar glyph, gboolean cursor)
{
  GtkWidget *b = omarchy_icon_button_new (gallery_glyph (glyph));
  omarchy_icon_button_set_cursor (OMARCHY_ICON_BUTTON (b), cursor);
  return b;
}

/* The placeholder's size follows the rail and the theme's spacing scale. */
static void
placeholder_resize (GtkWidget *box)
{
  GtkWidget *rail = g_object_get_data (G_OBJECT (box), "rail");
  int h;
  gtk_widget_measure (rail, GTK_ORIENTATION_VERTICAL, -1, NULL, &h, NULL, NULL);
  gtk_widget_set_size_request (box, omarchy_theme_space (omarchy_theme_get_default (), 320), h);
}

GtkWidget *
gallery_section_navigation (void)
{
  GtkWidget *section = gallery_section ("Navigation");

  GtkWidget *bar = omarchy_toolbar_new ();
  OmarchyToolbar *tb = OMARCHY_TOOLBAR (bar);
  omarchy_toolbar_append_leading (tb, icon (0xF032, FALSE));
  omarchy_toolbar_append_leading (tb, icon (0xF033, TRUE));
  omarchy_toolbar_append_leading (tb, icon (0xF0CD, FALSE));
  omarchy_toolbar_append_separator (tb);
  omarchy_toolbar_append_leading (tb, icon (0xF0CA, FALSE));
  omarchy_toolbar_append_leading (tb, icon (0xF0CB, FALSE));
  omarchy_toolbar_append_separator (tb);
  omarchy_toolbar_append_leading (tb, icon (0xF03E, FALSE));
  omarchy_toolbar_append_leading (tb, icon (0xF0C1, FALSE));
  GtkWidget *title = omarchy_heading_new ("notes.md");
  omarchy_heading_set_centered (OMARCHY_HEADING (title), TRUE);
  omarchy_toolbar_append_center (tb, title);
  GtkWidget *share = omarchy_button_new ("Share");
  omarchy_button_set_bordered (OMARCHY_BUTTON (share), TRUE);
  omarchy_toolbar_append_trailing (tb, share);
  omarchy_toolbar_set_overflow (tb, TRUE);
  gallery_add (section, gallery_fill (bar));

  static const char *const tabs_labels[] = { "General", "Display", "Audio", "Network", NULL };
  GtkWidget *tabs = omarchy_tabs_new (tabs_labels);
  omarchy_tabs_set_current_index (OMARCHY_TABS (tabs), 1);
  omarchy_tabs_set_cursor_index (OMARCHY_TABS (tabs), 2);
  gallery_add (section, gallery_fill (tabs));

  /* Qt's RowLayout { spacing: 0 } */
  GtkWidget *row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign (row, GTK_ALIGN_START);
  GtkWidget *rail = omarchy_icon_rail_new ();
  OmarchyIconRail *r = OMARCHY_ICON_RAIL (rail);
  omarchy_icon_rail_append (r, gallery_glyph (0xF07B), "Files");
  omarchy_icon_rail_append (r, gallery_glyph (0xF002), "Search");
  omarchy_icon_rail_append (r, gallery_glyph (0xF126), "Source control");
  omarchy_icon_rail_append (r, gallery_glyph (0xF188), "Debug");
  omarchy_icon_rail_append (r, gallery_glyph (0xF013), "Settings");
  omarchy_icon_rail_set_current_index (r, 1);
  gallery_add (row, rail);
  GtkWidget *content = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *name = gallery_text ("Search", "body", "dim");
  gtk_widget_set_halign (name, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (name, GTK_ALIGN_CENTER);
  gtk_widget_set_vexpand (name, TRUE);
  gtk_box_append (GTK_BOX (content), name);
  g_object_set_data (G_OBJECT (content), "rail", rail);
  placeholder_resize (content);
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (placeholder_resize), content, G_CONNECT_SWAPPED);
  gallery_add (row, content);
  gallery_add (section, row);
  return section;
}
