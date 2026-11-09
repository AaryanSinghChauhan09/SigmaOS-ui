/* progress: three Meters 220 wide with markers at 0.5 and 0.8 (0.2; 0.65; 0.92 alarming), a
 * labeled Meter 360 wide, two ProgressBars 340 wide (0.4; indeterminate) and four Spinners
 * (spec/components/feedback.md). (qt/gallery/sections/progress.qml) */
#include "gallery.h"

static GtkWidget *
meter (double value, gboolean alarming)
{
  GtkWidget *m = omarchy_meter_new (value);
  omarchy_meter_set_alarming (OMARCHY_METER (m), alarming);
  omarchy_meter_add_marker (OMARCHY_METER (m), 0.5);
  omarchy_meter_add_marker (OMARCHY_METER (m), 0.8);
  return gallery_width (m, 220);
}

static GtkWidget *
spinner (const char *size, gboolean pulse)
{
  GtkWidget *s = omarchy_spinner_new ();
  if (size)
    omarchy_spinner_set_size (OMARCHY_SPINNER (s), size);
  omarchy_spinner_set_pulse (OMARCHY_SPINNER (s), pulse);
  return s;
}

GtkWidget *
gallery_section_progress (void)
{
  GtkWidget *section = gallery_section ("Progress");

  GtkWidget *row = gallery_row ("huge");
  gallery_add (row, meter (0.2, FALSE));
  gallery_add (row, meter (0.65, FALSE));
  gallery_add (row, meter (0.92, TRUE));
  gallery_add (section, row);

  GtkWidget *battery = omarchy_meter_new (0.65);
  omarchy_meter_set_label (OMARCHY_METER (battery), "Battery");
  omarchy_meter_set_value_text (OMARCHY_METER (battery), "65%");
  gallery_add (section, gallery_width (battery, 360));

  row = gallery_row ("huge");
  gallery_add (row, gallery_width (omarchy_progress_bar_new (0.4), 340));
  gallery_add (row, gallery_width (omarchy_progress_bar_new_indeterminate (), 340));
  gallery_add (section, row);

  row = gallery_row ("huge");
  gallery_add (row, spinner (NULL, FALSE));
  gallery_add (row, spinner ("iconLarge", FALSE));
  gallery_add (row, spinner (NULL, TRUE));
  gallery_add (row, spinner ("iconLarge", TRUE));
  gallery_add (section, row);
  return section;
}
