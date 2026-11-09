/* timeline: a Timeline 720 × 160 (0:30, playhead at 0:12; Intro, Demo (selected), Outro over
 * Voiceover and Music); a Scrubber 360 wide at 0:42 of 2:10; then, `huge` apart and
 * top-aligned, a 360-wide list of four `popupRowHeight` rows with DragHandles (Clock under the
 * cursor, a DropIndicator between Battery and Network) and a 240 × 140 normal-fill rectangle
 * with a CropFrame over it (spec/components/editing.md). (qt/gallery/sections/timeline.qml) */
#include "gallery.h"

static void
on_seek (OmarchyTimeline *tl, double time, gpointer data)
{
  omarchy_timeline_set_position (tl, time);
}

static GtkWidget *
timeline (void)
{
  GtkWidget *w = omarchy_timeline_new (30);
  OmarchyTimeline *tl = OMARCHY_TIMELINE (w);
  omarchy_timeline_set_position (tl, 12);
  int a = omarchy_timeline_add_track (tl);
  omarchy_timeline_append_clip (tl, a, 0, 7.5, "Intro");
  omarchy_timeline_append_clip (tl, a, 8, 18, "Demo");
  omarchy_timeline_append_clip (tl, a, 18.5, 30, "Outro");
  int b = omarchy_timeline_add_track (tl);
  omarchy_timeline_append_clip (tl, b, 2, 12, "Voiceover");
  omarchy_timeline_append_clip (tl, b, 14, 26, "Music");
  omarchy_timeline_set_selected (tl, 0, 1);
  g_signal_connect (w, "seek", G_CALLBACK (on_seek), NULL);
  gtk_widget_set_size_request (w, -1, 160);
  return gallery_width (w, 720);
}

/* The drop indicator sits across the boundary between rows 3 and 4. */
static void
place_indicator (GtkWidget *indicator)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  int row = omarchy_theme_get_spacing (theme, "popupRowHeight");
  int h = omarchy_drop_indicator_get_thickness ();
  gtk_widget_set_margin_top (indicator, row * 3 - (h + 1) / 2);
}

static void
size_row (GtkWidget *row)
{
  gtk_widget_set_size_request (row, -1, omarchy_theme_get_spacing (omarchy_theme_get_default (), "popupRowHeight"));
}

static void
space_row (GtkWidget *box)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  int sm = omarchy_theme_get_spacing (theme, "sm");
  gtk_box_set_spacing (GTK_BOX (box), sm);
  gtk_widget_set_margin_start (box, sm);
}

static GtkWidget *
reorder_list (void)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  GtkWidget *rows = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  const char *labels[] = { "Workspaces", "Clock", "Battery", "Network" };
  for (int i = 0; i < 4; i++) {
    GtkWidget *row = omarchy_surface_new (i == 1 ? "controls.hover.fill" : NULL, NULL);
    GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
    space_row (box);
    g_signal_connect_object (theme, "restyle", G_CALLBACK (space_row), box, G_CONNECT_SWAPPED);
    GtkWidget *grip = omarchy_drag_handle_new ();
    omarchy_drag_handle_set_cursor (OMARCHY_DRAG_HANDLE (grip), i == 1);
    gtk_widget_set_valign (grip, GTK_ALIGN_CENTER);
    gtk_box_append (GTK_BOX (box), grip);
    GtkWidget *label = gallery_text (labels[i], "body", NULL);
    gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
    gtk_box_append (GTK_BOX (box), label);
    omarchy_surface_set_child (OMARCHY_SURFACE (row), box);
    size_row (row);
    g_signal_connect_object (theme, "restyle", G_CALLBACK (size_row), row, G_CONNECT_SWAPPED);
    gtk_box_append (GTK_BOX (rows), row);
  }
  GtkWidget *overlay = gtk_overlay_new ();
  gtk_overlay_set_child (GTK_OVERLAY (overlay), rows);
  GtkWidget *indicator = omarchy_drop_indicator_new ();
  gtk_widget_set_halign (indicator, GTK_ALIGN_FILL);
  gtk_widget_set_valign (indicator, GTK_ALIGN_START);
  place_indicator (indicator);
  g_signal_connect_object (theme, "restyle", G_CALLBACK (place_indicator), indicator, G_CONNECT_SWAPPED);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), indicator);
  return gallery_width (overlay, 360);
}

static void
draw_normal_fill (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
  g_autoptr (GdkRGBA) c = omarchy_theme_get_paint_color (omarchy_theme_get_default (), "controls.normal.fill");
  if (!c)
    return;
  gdk_cairo_set_source_rgba (cr, c);
  cairo_paint (cr);
}

static GtkWidget *
crop (void)
{
  GtkWidget *image = gtk_drawing_area_new ();
  gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (image), draw_normal_fill, NULL, NULL);
  g_signal_connect_object (omarchy_theme_get_default (), "repaint", G_CALLBACK (gtk_widget_queue_draw), image,
                           G_CONNECT_SWAPPED);
  gtk_widget_set_size_request (image, 240, 140);
  GtkWidget *overlay = gtk_overlay_new ();
  gtk_overlay_set_child (GTK_OVERLAY (overlay), image);
  GtkWidget *frame = omarchy_crop_frame_new ();
  gtk_widget_set_size_request (frame, 0, 0);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), frame);
  gtk_overlay_set_clip_overlay (GTK_OVERLAY (overlay), frame, FALSE);
  return overlay;
}

GtkWidget *
gallery_section_timeline (void)
{
  GtkWidget *section = gallery_section ("Timeline");
  gallery_add (section, timeline ());
  gallery_add (section, gallery_width (omarchy_scrubber_new (42, 130), 360));
  GtkWidget *row = gallery_row ("huge");
  gallery_add (row, gallery_top (reorder_list ()));
  gallery_add (row, gallery_top (crop ()));
  gallery_add (section, gallery_top (row));
  return section;
}
