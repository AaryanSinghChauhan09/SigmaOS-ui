/* toggles: four ToggleSwitches (off, on, off + cursor, on + cursor); Toggle "Wi-Fi" on and
 * Toggle "Bluetooth" off with the cursor, 360 wide. (qt/gallery/sections/toggles.qml) */
#include "gallery.h"

static GtkWidget *
switch_ (gboolean checked, gboolean cursor)
{
  GtkWidget *s = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (s), checked);
  omarchy_toggle_switch_set_cursor (OMARCHY_TOGGLE_SWITCH (s), cursor);
  return s;
}

GtkWidget *
gallery_section_toggles (void)
{
  GtkWidget *section = gallery_section ("Toggles");
  GtkWidget *row = gallery_row ("xxl");
  gallery_add (row, switch_ (FALSE, FALSE));
  gallery_add (row, switch_ (TRUE, FALSE));
  gallery_add (row, switch_ (FALSE, TRUE));
  gallery_add (row, switch_ (TRUE, TRUE));
  gallery_add (section, row);

  GtkWidget *wifi = omarchy_toggle_new ("Wi-Fi", "Connected to Basecamp");
  omarchy_toggle_set_checked (OMARCHY_TOGGLE (wifi), TRUE);
  gallery_add (section, gallery_width (wifi, 360));
  GtkWidget *bt = omarchy_toggle_new ("Bluetooth", "Off");
  omarchy_toggle_set_cursor (OMARCHY_TOGGLE (bt), TRUE);
  gallery_add (section, gallery_width (bt, 360));
  return section;
}
