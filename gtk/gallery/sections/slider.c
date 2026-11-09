/* slider: Slider at 0.35, 360 wide; Slider at 0.75 with 5 notches, 360 wide.
 * (qt/gallery/sections/slider.qml) */
#include "gallery.h"

GtkWidget *
gallery_section_slider (void)
{
  GtkWidget *section = gallery_section ("Slider");
  gallery_add (section, gallery_width (omarchy_slider_new (0.35), 360));
  GtkWidget *notched = omarchy_slider_new (0.75);
  omarchy_slider_set_tick_count (OMARCHY_SLIDER (notched), 5);
  gallery_add (section, gallery_width (notched, 360));
  return section;
}
