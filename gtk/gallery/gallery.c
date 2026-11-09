/* gallery.c: the helpers sections are built with. */
#include "gallery.h"

#include <string.h>

static int
spacing_px (const char *token)
{
  return omarchy_theme_get_spacing (omarchy_theme_get_default (), token);
}

static void
respace (GtkWidget *box)
{
  const char *token = g_object_get_data (G_OBJECT (box), "gallery-spacing");
  if (token)
    gtk_box_set_spacing (GTK_BOX (box), spacing_px (token));
}

static GtkWidget *
spaced (GtkOrientation o, const char *token)
{
  GtkWidget *box = gtk_box_new (o, spacing_px (token));
  g_object_set_data_full (G_OBJECT (box), "gallery-spacing", g_strdup (token), g_free);
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (respace), box, G_CONNECT_SWAPPED);
  return box;
}

GtkWidget *
gallery_row (const char *spacing)
{
  GtkWidget *row = spaced (GTK_ORIENTATION_HORIZONTAL, spacing);
  gtk_widget_set_halign (row, GTK_ALIGN_START);
  return row;
}

GtkWidget *
gallery_column (const char *spacing)
{
  return spaced (GTK_ORIENTATION_VERTICAL, spacing);
}

void
gallery_add (GtkWidget *container, GtkWidget *child)
{
  GtkWidget *box = g_object_get_data (G_OBJECT (container), "gallery-body");
  if (!box)
    box = container;
  if (gtk_orientable_get_orientation (GTK_ORIENTABLE (box)) == GTK_ORIENTATION_HORIZONTAL) {
    if (!g_object_get_data (G_OBJECT (child), "gallery-top"))
      gtk_widget_set_valign (child, GTK_ALIGN_CENTER);
    else
      gtk_widget_set_valign (child, GTK_ALIGN_START);
  } else if (!g_object_get_data (G_OBJECT (child), "gallery-fill")) {
    gtk_widget_set_halign (child, GTK_ALIGN_START);
  }
  gtk_box_append (GTK_BOX (box), child);
}

GtkWidget *
gallery_section (const char *title)
{
  GtkWidget *section = gallery_column ("lg");
  GtkWidget *header = omarchy_section_header_new (title);
  gtk_box_append (GTK_BOX (section), header);
  GtkWidget *body = gallery_column ("xxl");
  gtk_box_append (GTK_BOX (section), body);
  g_object_set_data (G_OBJECT (section), "gallery-body", body);
  return section;
}

GtkWidget *
gallery_width (GtkWidget *w, int width)
{
  gtk_widget_set_size_request (w, width, -1);
  return w;
}

GtkWidget *
gallery_size (GtkWidget *w, int width, int height)
{
  gtk_widget_set_size_request (w, width, height);
  return w;
}

GtkWidget *
gallery_fill (GtkWidget *w)
{
  g_object_set_data (G_OBJECT (w), "gallery-fill", GINT_TO_POINTER (1));
  gtk_widget_set_halign (w, GTK_ALIGN_FILL);
  return w;
}

GtkWidget *
gallery_top (GtkWidget *w)
{
  g_object_set_data (G_OBJECT (w), "gallery-top", GINT_TO_POINTER (1));
  return w;
}

GtkWidget *
gallery_text (const char *text, const char *size, const char *role)
{
  GtkWidget *l = omarchy_label_new (text);
  if (size)
    omarchy_label_set_size (OMARCHY_LABEL (l), size);
  if (role)
    omarchy_label_set_role (OMARCHY_LABEL (l), role);
  return l;
}

const char *
gallery_glyph (gunichar c)
{
  static char bufs[16][8];
  static int next;
  char *buf = bufs[next++ % 16];
  int n = g_unichar_to_utf8 (c, buf);
  buf[n] = 0;
  return buf;
}
