/* tokens: swatches (64 × 32) for foreground, background, accent, urgent, muted; state chips
 * (120 × controlHeight) normal, hover-cursor, focus, selected; the type scale from displayLarge
 * to caption. (qt/gallery/sections/tokens.qml) */
#include "gallery.h"

static GtkWidget *
swatch (const char *name)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  GtkWidget *col = gtk_box_new (GTK_ORIENTATION_VERTICAL, omarchy_theme_get_spacing (theme, "xs"));
  g_autofree char *path = g_strconcat ("palette.", name, NULL);
  GtkWidget *chip = omarchy_surface_new (path, "controls.normal.border");
  omarchy_surface_set_border_width (OMARCHY_SURFACE (chip), 1);
  omarchy_surface_set_small (OMARCHY_SURFACE (chip), TRUE);
  gtk_widget_set_size_request (chip, 64, 32);
  gtk_widget_set_halign (chip, GTK_ALIGN_START);
  gtk_box_append (GTK_BOX (col), chip);
  GtkWidget *label = gallery_text (name, "caption", NULL);
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_append (GTK_BOX (col), label);
  return col;
}

static GtkWidget *
state_chip (const char *state, const char *key)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  g_autofree char *fill = g_strdup_printf ("controls.%s.fill", key);
  g_autofree char *border = g_strdup_printf ("controls.%s.border", key);
  GtkWidget *chip = omarchy_surface_new (fill, border);
  gboolean selected = !g_strcmp0 (key, "selected");
  GtkWidget *label = gallery_text (state, "body", selected ? "selected" : NULL);
  omarchy_label_set_bold (OMARCHY_LABEL (label), selected);
  gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
  omarchy_surface_set_child (OMARCHY_SURFACE (chip), label);
  gtk_widget_set_size_request (chip, 120, omarchy_theme_get_spacing (theme, "controlHeight"));
  return chip;
}

GtkWidget *
gallery_section_tokens (void)
{
  GtkWidget *section = gallery_section ("Tokens");
  GtkWidget *row = gallery_row ("lg");
  const char *swatches[] = { "foreground", "background", "accent", "urgent", "muted" };
  for (guint i = 0; i < G_N_ELEMENTS (swatches); i++)
    gallery_add (gallery_top (row), gallery_top (swatch (swatches[i])));
  gallery_add (section, row);

  row = gallery_row ("lg");
  gallery_add (row, state_chip ("normal", "normal"));
  gallery_add (row, state_chip ("hover-cursor", "hover"));
  gallery_add (row, state_chip ("focus", "focus"));
  gallery_add (row, state_chip ("selected", "selected"));
  gallery_add (section, row);

  GtkWidget *scale = gallery_column ("xs");
  const char *sizes[] = { "displayLarge", "display", "heading", "title", "subtitle", "body", "bodySmall", "caption" };
  for (guint i = 0; i < G_N_ELEMENTS (sizes); i++) {
    g_autofree char *text = g_strdup_printf ("%s %d", sizes[i], omarchy_theme_get_font_size (omarchy_theme_get_default (), sizes[i]));
    gallery_add (scale, gallery_text (text, sizes[i], NULL));
  }
  gallery_add (section, scale);
  return section;
}
