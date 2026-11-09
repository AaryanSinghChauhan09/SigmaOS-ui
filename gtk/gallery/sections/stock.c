/* stock: GTK's and libadwaita's own widgets, restyled by omarchy_gtk_init() alone (not a
 * spec/gallery.md section: it shows what an app gets without the kit). */
#include "gallery.h"

#include <adwaita.h>

static GtkWidget *
button (const char *label, const char *css)
{
  GtkWidget *b = gtk_button_new_with_label (label);
  if (css)
    gtk_widget_add_css_class (b, css);
  return b;
}

GtkWidget *
gallery_section_stock (void)
{
  GtkWidget *section = gallery_section ("Stock GTK and libadwaita");

  GtkWidget *row = gallery_row ("controlGap");
  gallery_add (row, button ("Button", NULL));
  gallery_add (row, button ("Flat", "flat"));
  gallery_add (row, button ("Suggested", "suggested-action"));
  gallery_add (row, button ("Delete", "destructive-action"));
  GtkWidget *t = gtk_toggle_button_new_with_label ("Toggled");
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (t), TRUE);
  gallery_add (row, t);
  GtkWidget *icon = gtk_button_new_from_icon_name ("open-menu-symbolic");
  gallery_add (row, icon);
  gallery_add (section, row);

  row = gallery_row ("controlGap");
  GtkWidget *entry = gtk_entry_new ();
  gtk_entry_set_placeholder_text (GTK_ENTRY (entry), "Search…");
  gallery_add (row, gallery_width (entry, 200));
  GtkWidget *filled = gtk_entry_new ();
  gtk_editable_set_text (GTK_EDITABLE (filled), "Omarchy");
  gallery_add (row, gallery_width (filled, 160));
  GtkWidget *spin = gtk_spin_button_new_with_range (0, 40, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin), 10);
  gallery_add (row, spin);
  const char *themes[] = { "Tokyo Night", "Nord", NULL };
  gallery_add (row, gallery_width (gtk_drop_down_new_from_strings (themes), 180));
  gallery_add (section, row);

  row = gallery_row ("xxl");
  GtkWidget *off = gtk_switch_new ();
  gallery_add (row, off);
  GtkWidget *on = gtk_switch_new ();
  gtk_switch_set_active (GTK_SWITCH (on), TRUE);
  gallery_add (row, on);
  GtkWidget *check = gtk_check_button_new_with_label ("Check");
  gtk_check_button_set_active (GTK_CHECK_BUTTON (check), TRUE);
  gallery_add (row, check);
  gallery_add (row, gtk_check_button_new_with_label ("Unchecked"));
  GtkWidget *r1 = gtk_check_button_new_with_label ("Radio");
  GtkWidget *r2 = gtk_check_button_new_with_label ("Other");
  gtk_check_button_set_group (GTK_CHECK_BUTTON (r2), GTK_CHECK_BUTTON (r1));
  gtk_check_button_set_active (GTK_CHECK_BUTTON (r1), TRUE);
  gallery_add (row, r1);
  gallery_add (row, r2);
  gallery_add (section, row);

  row = gallery_row ("xxl");
  GtkWidget *scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 1, 0.05);
  gtk_range_set_value (GTK_RANGE (scale), 0.35);
  gallery_add (row, gallery_width (scale, 240));
  GtkWidget *progress = gtk_progress_bar_new ();
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress), 0.6);
  gallery_add (row, gallery_width (progress, 240));
  gallery_add (section, row);

  row = gallery_row ("huge");
  GtkWidget *list = gtk_list_box_new ();
  gtk_widget_add_css_class (list, "boxed-list");
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (list), GTK_SELECTION_NONE);
  GtkWidget *wifi = adw_switch_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (wifi), "Wi-Fi");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (wifi), "Connected to Basecamp");
  adw_switch_row_set_active (ADW_SWITCH_ROW (wifi), TRUE);
  gtk_list_box_append (GTK_LIST_BOX (list), wifi);
  GtkWidget *action = adw_action_row_new ();
  adw_preferences_row_set_title (ADW_PREFERENCES_ROW (action), "Bluetooth");
  adw_action_row_set_subtitle (ADW_ACTION_ROW (action), "Off");
  gtk_list_box_append (GTK_LIST_BOX (list), action);
  gallery_add (row, gallery_top (gallery_width (list, 320)));

  GtkWidget *col = gallery_column ("md");
  GtkWidget *switcher = gtk_stack_switcher_new ();
  GtkWidget *stack = gtk_stack_new ();
  gtk_stack_add_titled (GTK_STACK (stack), gtk_label_new (""), "a", "Wi-Fi");
  gtk_stack_add_titled (GTK_STACK (stack), gtk_label_new (""), "b", "Bluetooth");
  gtk_stack_add_titled (GTK_STACK (stack), gtk_label_new (""), "c", "Audio");
  gtk_stack_switcher_set_stack (GTK_STACK_SWITCHER (switcher), GTK_STACK (stack));
  gallery_add (col, switcher);
  GtkWidget *title = gtk_label_new ("Title 2");
  gtk_widget_add_css_class (title, "title-2");
  gallery_add (col, title);
  GtkWidget *dim = gtk_label_new ("A dim caption");
  gtk_widget_add_css_class (dim, "dim-label");
  gtk_widget_add_css_class (dim, "caption");
  gallery_add (col, dim);
  gallery_add (col, gallery_width (gtk_separator_new (GTK_ORIENTATION_HORIZONTAL), 200));
  gallery_add (row, gallery_top (col));
  gallery_add (section, row);
  return section;
}
