/* forms: a 480-wide column of SettingRows (Night light with a ToggleSwitch on; Theme with a
 * 180-wide Dropdown = Tokyo Night; Gaps with a NumberField 10 (0–40)). Then, `huge × 2` apart,
 * a column of Checkboxes `lg` apart and a RadioGroup Top / Bottom / Hidden = Bottom. Then three
 * 220-wide PasswordFields (placeholder; "hunter22" revealable; "letmein" with an error), then a
 * 480-wide TextArea of three lines. (qt/gallery/sections/forms.qml) */
#include "gallery.h"

static void
respace_double (GtkWidget *box)
{
  gtk_box_set_spacing (GTK_BOX (box), omarchy_theme_get_spacing (omarchy_theme_get_default (), "huge") * 2);
}

/* A row `huge × 2` apart (RowLayout { spacing: Style.spacing.huge * 2 }). */
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
check (const char *label, const char *description, gboolean checked, gboolean cursor, gboolean enabled)
{
  GtkWidget *c = omarchy_checkbox_new (label);
  if (description)
    omarchy_checkbox_set_description (OMARCHY_CHECKBOX (c), description);
  omarchy_checkbox_set_checked (OMARCHY_CHECKBOX (c), checked);
  omarchy_checkbox_set_cursor (OMARCHY_CHECKBOX (c), cursor);
  gtk_widget_set_sensitive (c, enabled);
  return c;
}

static GtkWidget *
password (const char *text, const char *placeholder, gboolean revealable, const char *error)
{
  GtkWidget *p = omarchy_password_field_new ();
  if (text)
    gtk_editable_set_text (GTK_EDITABLE (p), text);
  if (placeholder)
    omarchy_password_field_set_placeholder_text (OMARCHY_PASSWORD_FIELD (p), placeholder);
  omarchy_password_field_set_revealable (OMARCHY_PASSWORD_FIELD (p), revealable);
  if (error)
    omarchy_password_field_set_error (OMARCHY_PASSWORD_FIELD (p), error);
  return gallery_top (gallery_width (p, 220));
}

GtkWidget *
gallery_section_forms (void)
{
  GtkWidget *section = gallery_section ("Forms");

  GtkWidget *rows = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *night = omarchy_toggle_switch_new ();
  omarchy_toggle_switch_set_checked (OMARCHY_TOGGLE_SWITCH (night), TRUE);
  gtk_box_append (GTK_BOX (rows), omarchy_setting_row_new ("Night light", "Warm the screen after sunset", night));
  GtkWidget *theme = omarchy_dropdown_new (NULL);
  omarchy_dropdown_append (OMARCHY_DROPDOWN (theme), "tokyo-night", "Tokyo Night");
  omarchy_dropdown_append (OMARCHY_DROPDOWN (theme), "nord", "Nord");
  omarchy_dropdown_set_value (OMARCHY_DROPDOWN (theme), "tokyo-night");
  gtk_box_append (GTK_BOX (rows), omarchy_setting_row_new ("Theme", "Applies to every app", gallery_width (theme, 180)));
  GtkWidget *gaps = omarchy_number_field_new (NULL, 0, 40);
  omarchy_number_field_set_value (OMARCHY_NUMBER_FIELD (gaps), 10);
  gtk_box_append (GTK_BOX (rows), omarchy_setting_row_new ("Gaps", "Space between windows", gaps));
  gallery_add (section, gallery_width (rows, 480));

  GtkWidget *row = double_huge_row ();
  GtkWidget *checks = gallery_column ("lg");
  gallery_add (checks, check ("Show seconds", NULL, FALSE, FALSE, TRUE));
  gallery_add (checks, check ("Show date", NULL, TRUE, FALSE, TRUE));
  gallery_add (checks, check ("Show weekday", NULL, FALSE, TRUE, TRUE));
  gallery_add (checks, check ("Show week number", NULL, FALSE, FALSE, FALSE));
  gallery_add (checks, check ("Twelve-hour clock", "Show AM and PM", TRUE, FALSE, TRUE));
  gallery_add (row, gallery_top (checks));
  GtkWidget *radios = omarchy_radio_group_new ();
  omarchy_radio_group_append (OMARCHY_RADIO_GROUP (radios), "top", "Top", NULL);
  omarchy_radio_group_append (OMARCHY_RADIO_GROUP (radios), "bottom", "Bottom", NULL);
  omarchy_radio_group_append (OMARCHY_RADIO_GROUP (radios), "hidden", "Hidden", NULL);
  omarchy_radio_group_set_value (OMARCHY_RADIO_GROUP (radios), "bottom");
  gallery_add (row, gallery_top (radios));
  gallery_add (section, row);

  row = gallery_row ("controlGap");
  gallery_add (row, password (NULL, "Password", FALSE, NULL));
  gallery_add (row, password ("hunter22", NULL, TRUE, NULL));
  gallery_add (row, password ("letmein", NULL, FALSE, "Wrong password, try again"));
  gallery_add (section, row);

  GtkWidget *area = omarchy_text_area_new ();
  omarchy_text_area_set_text (OMARCHY_TEXT_AREA (area), "Back up your dotfiles\nbefore switching themes,\nthen run omarchy update.");
  gallery_add (section, gallery_width (area, 480));
  return section;
}
