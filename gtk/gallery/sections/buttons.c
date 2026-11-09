/* buttons: Plain; Bordered; Bordered + cursor; Bordered + selected; Bordered with an icon;
 * IconButton bordered; IconButton bordered + cursor. Then ButtonGroup Left / Center / Right with
 * Center selected. (spec/gallery.md, qt/gallery/sections/buttons.qml) */
#include "gallery.h"

GtkWidget *
gallery_section_buttons (void)
{
  GtkWidget *section = gallery_section ("Buttons");
  GtkWidget *row = gallery_row ("controlGap");
  gallery_add (row, omarchy_button_new ("Plain"));
  GtkWidget *b = omarchy_button_new ("Bordered");
  omarchy_button_set_bordered (OMARCHY_BUTTON (b), TRUE);
  gallery_add (row, b);
  b = omarchy_button_new ("Hover");
  omarchy_button_set_bordered (OMARCHY_BUTTON (b), TRUE);
  omarchy_button_set_cursor (OMARCHY_BUTTON (b), TRUE);
  gallery_add (row, b);
  b = omarchy_button_new ("Selected");
  omarchy_button_set_bordered (OMARCHY_BUTTON (b), TRUE);
  omarchy_button_set_selected (OMARCHY_BUTTON (b), TRUE);
  gallery_add (row, b);
  b = omarchy_button_new ("Icon");
  omarchy_button_set_icon (OMARCHY_BUTTON (b), gallery_glyph (0xF013));
  omarchy_button_set_bordered (OMARCHY_BUTTON (b), TRUE);
  gallery_add (row, b);
  GtkWidget *i = omarchy_icon_button_new (gallery_glyph (0xF011));
  omarchy_icon_button_set_bordered (OMARCHY_ICON_BUTTON (i), TRUE);
  gallery_add (row, i);
  i = omarchy_icon_button_new (gallery_glyph (0xF021));
  omarchy_icon_button_set_bordered (OMARCHY_ICON_BUTTON (i), TRUE);
  omarchy_icon_button_set_cursor (OMARCHY_ICON_BUTTON (i), TRUE);
  gallery_add (row, i);
  gallery_add (section, row);

  GtkWidget *group = omarchy_button_group_new ();
  omarchy_button_group_append (OMARCHY_BUTTON_GROUP (group), "left", "Left");
  omarchy_button_group_append (OMARCHY_BUTTON_GROUP (group), "center", "Center");
  omarchy_button_group_append (OMARCHY_BUTTON_GROUP (group), "right", "Right");
  omarchy_button_group_set_value (OMARCHY_BUTTON_GROUP (group), "center");
  gallery_add (section, group);
  return section;
}
