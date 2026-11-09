/* layout: a SplitView 720 × 220 framed by a hairline track border at the radius: a Sidebar 200
 * wide (min 120; Library: All files, Recent selected, Starred, Trash; Places: Documents,
 * Downloads), the handle at rest, a content pane with "No file selected" centered in body faint.
 * Below it a Calendar for November 2026, today the 8th, the 14th selected
 * (spec/components/data.md, qt/gallery/sections/layout.qml). */
#include "gallery.h"

/* The frame drawn over the split view so its edges show (the Qt gallery's Rectangle). */
#define GALLERY_TYPE_FRAME (gallery_frame_get_type ())
G_DECLARE_FINAL_TYPE (GalleryFrame, gallery_frame, GALLERY, FRAME, GtkWidget)

struct _GalleryFrame {
  GtkWidget parent_instance;
};

G_DEFINE_FINAL_TYPE (GalleryFrame, gallery_frame, GTK_TYPE_WIDGET)

static void
frame_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  g_autoptr (GdkRGBA) track = omarchy_theme_get_paint_color (theme, "roles.track");
  float hair = omarchy_theme_get_spacing (theme, "hairline");
  float W = gtk_widget_get_width (w), H = gtk_widget_get_height (w);
  float r = MIN (omarchy_theme_get_radius (theme), MIN (W, H) / 2);
  GskRoundedRect rr;
  gsk_rounded_rect_init_from_rect (&rr, &GRAPHENE_RECT_INIT (0, 0, W, H), r);
  const float widths[4] = { hair, hair, hair, hair };
  const GdkRGBA colors[4] = { *track, *track, *track, *track };
  gtk_snapshot_append_border (s, &rr, widths, colors);
}

static void
gallery_frame_init (GalleryFrame *self)
{
  gtk_widget_set_can_target (GTK_WIDGET (self), FALSE);
  g_signal_connect_object (omarchy_theme_get_default (), "repaint", G_CALLBACK (gtk_widget_queue_draw), self, G_CONNECT_SWAPPED);
}

static void
gallery_frame_class_init (GalleryFrameClass *klass)
{
  GTK_WIDGET_CLASS (klass)->snapshot = frame_snapshot;
}

static GtkWidget *
sidebar (void)
{
  GtkWidget *w = omarchy_sidebar_new ();
  OmarchySidebar *s = OMARCHY_SIDEBAR (w);
  omarchy_sidebar_append_section (s, "Library");
  omarchy_sidebar_append_item (s, "all", "All files", gallery_glyph (0xF024B));
  omarchy_sidebar_append_item (s, "recent", "Recent", gallery_glyph (0xF02DA));
  omarchy_sidebar_append_item (s, "starred", "Starred", gallery_glyph (0xF04CE));
  omarchy_sidebar_append_item (s, "trash", "Trash", gallery_glyph (0xF0A7A));
  omarchy_sidebar_append_section (s, "Places");
  omarchy_sidebar_append_item (s, "documents", "Documents", gallery_glyph (0xF0219));
  omarchy_sidebar_append_item (s, "downloads", "Downloads", gallery_glyph (0xF01DA));
  omarchy_sidebar_set_selected_id (s, "recent");
  return w;
}

GtkWidget *
gallery_section_layout (void)
{
  GtkWidget *section = gallery_section ("Layout");

  GtkWidget *split = omarchy_split_view_new ();
  omarchy_split_view_append_sized (OMARCHY_SPLIT_VIEW (split), sidebar (), 200, 120, -1);
  GtkWidget *empty = gallery_text ("No file selected", "body", "faint");
  gtk_widget_set_halign (empty, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (empty, GTK_ALIGN_CENTER);
  omarchy_split_view_append (OMARCHY_SPLIT_VIEW (split), empty);

  GtkWidget *framed = gtk_overlay_new ();
  gtk_overlay_set_child (GTK_OVERLAY (framed), split);
  gtk_overlay_add_overlay (GTK_OVERLAY (framed), g_object_new (GALLERY_TYPE_FRAME, NULL));
  gallery_add (section, gallery_size (framed, 720, 220));

  GtkWidget *calendar = omarchy_calendar_new (2026, 11);
  omarchy_calendar_set_today (OMARCHY_CALENDAR (calendar), "2026-11-08");
  omarchy_calendar_set_selected (OMARCHY_CALENDAR (calendar), "2026-11-14");
  gallery_add (section, calendar);
  return section;
}
