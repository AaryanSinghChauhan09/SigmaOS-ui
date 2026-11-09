/* color: a ColorPicker 300 × 260 for the fixed color #7aa2f7 (not the theme's accent, so the
 * picker's content is the same in every theme), with the alpha strip, the hex field and the
 * theme's palette (spec/components/editing.md). (qt/gallery/sections/color.qml) */
#include "gallery.h"

GtkWidget *
gallery_section_color (void)
{
  GtkWidget *section = gallery_section ("Color");
  GtkWidget *picker = omarchy_color_picker_new ();
  omarchy_color_picker_set_color (OMARCHY_COLOR_PICKER (picker), "#7aa2f7");
  gallery_add (section, gallery_size (picker, 300, 260));
  return section;
}
