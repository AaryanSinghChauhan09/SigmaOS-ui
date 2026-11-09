/* inputs: three TextFields 220 wide (placeholder "Search…"; "Omarchy"; "Hovered" with the
 * cursor), then NumberField "Gaps" 10 (0–40) and Dropdown "Theme" = Tokyo Night.
 * (qt/gallery/sections/inputs.qml) */
#include "gallery.h"

static GtkWidget *
field (const char *text, const char *placeholder, gboolean cursor)
{
  GtkWidget *f = omarchy_text_field_new ();
  if (text)
    gtk_editable_set_text (GTK_EDITABLE (f), text);
  if (placeholder)
    omarchy_text_field_set_placeholder_text (OMARCHY_TEXT_FIELD (f), placeholder);
  omarchy_text_field_set_cursor (OMARCHY_TEXT_FIELD (f), cursor);
  return gallery_width (f, 220);
}

GtkWidget *
gallery_section_inputs (void)
{
  GtkWidget *section = gallery_section ("Inputs");
  GtkWidget *row = gallery_row ("controlGap");
  gallery_add (row, field (NULL, "Search…", FALSE));
  gallery_add (row, field ("Omarchy", NULL, FALSE));
  gallery_add (row, field ("Hovered", NULL, TRUE));
  gallery_add (section, row);

  row = gallery_row ("xxl");
  GtkWidget *gaps = omarchy_number_field_new ("Gaps", 0, 40);
  omarchy_number_field_set_value (OMARCHY_NUMBER_FIELD (gaps), 10);
  gallery_add (row, gaps);
  GtkWidget *theme = omarchy_dropdown_new ("Theme");
  omarchy_dropdown_append (OMARCHY_DROPDOWN (theme), "tokyo-night", "Tokyo Night");
  omarchy_dropdown_append (OMARCHY_DROPDOWN (theme), "nord", "Nord");
  omarchy_dropdown_set_value (OMARCHY_DROPDOWN (theme), "tokyo-night");
  gallery_add (row, theme);
  gallery_add (section, row);
  return section;
}
