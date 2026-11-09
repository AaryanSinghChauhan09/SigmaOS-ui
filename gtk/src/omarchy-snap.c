/* omarchy-snap.c: OMARCHY_GTK_SNAP=out.png renders an app's first window to a PNG and exits,
 * with no app code (as Rune's --snap tooling and the Qt gallery's --snap do). For stills of
 * examples and apps in review and tests; run it on a display nobody sees (omarchy-gtk-headless). */
#include "omarchy-private.h"

#include <stdio.h>
#include <stdlib.h>

static gint64 mapped_at;
static gint64 started;

static GdkTexture *
render (GtkWidget *widget, double scale)
{
  int w = gtk_widget_get_width (widget), h = gtk_widget_get_height (widget);
  GdkPaintable *paintable = gtk_widget_paintable_new (widget);
  GtkSnapshot *s = gtk_snapshot_new ();
  gtk_snapshot_scale (s, (float) scale, (float) scale);
  gdk_paintable_snapshot (paintable, s, w, h);
  GskRenderNode *node = gtk_snapshot_free_to_node (s);
  g_object_unref (paintable);
  GskRenderer *renderer = gsk_cairo_renderer_new ();
  if (!gsk_renderer_realize_for_display (renderer, gtk_widget_get_display (widget), NULL)) {
    g_object_unref (renderer);
    g_clear_pointer (&node, gsk_render_node_unref);
    return NULL;
  }
  graphene_rect_t viewport = GRAPHENE_RECT_INIT (0, 0, (float) (w * scale), (float) (h * scale));
  if (!node)
    node = gsk_color_node_new (&(GdkRGBA) { 0, 0, 0, 0 }, &viewport);
  GdkTexture *texture = gsk_renderer_render_texture (renderer, node, &viewport);
  gsk_render_node_unref (node);
  gsk_renderer_unrealize (renderer);
  g_object_unref (renderer);
  return texture;
}

static gboolean
poll_windows (gpointer data)
{
  const char *out = data;
  if (g_get_monotonic_time () - started > 20 * G_USEC_PER_SEC) {
    g_printerr ("omarchy-gtk: OMARCHY_GTK_SNAP: no window showed within 20 s\n");
    exit (1);
  }
  GListModel *tops = gtk_window_get_toplevels ();
  GtkWidget *window = NULL;
  for (guint i = 0; i < g_list_model_get_n_items (tops) && !window; i++) {
    g_autoptr (GtkWidget) w = g_list_model_get_item (tops, i);
    if (gtk_widget_get_mapped (w) && gtk_widget_get_width (w) > 0)
      window = w;
  }
  if (!window) {
    mapped_at = 0;
    return G_SOURCE_CONTINUE;
  }
  if (!mapped_at) {
    /* Nothing focused in a still: no caret, no focus look. */
    gtk_window_set_focus (GTK_WINDOW (window), NULL);
    mapped_at = g_get_monotonic_time ();
  }
  if (g_get_monotonic_time () - mapped_at < 300 * 1000)
    return G_SOURCE_CONTINUE;
  const char *scale_env = g_getenv ("OMARCHY_GTK_SNAP_SCALE");
  double scale = scale_env ? g_ascii_strtod (scale_env, NULL) : 2;
  GdkTexture *t = render (window, scale > 0 ? scale : 2);
  if (!t || !gdk_texture_save_to_png (t, out)) {
    g_printerr ("omarchy-gtk: OMARCHY_GTK_SNAP: could not write %s\n", out);
    exit (1);
  }
  g_object_unref (t);
  exit (0);
}

void
omarchy_snap_watch (void)
{
  const char *out = g_getenv ("OMARCHY_GTK_SNAP");
  if (!out || !*out)
    return;
  started = g_get_monotonic_time ();
  g_timeout_add (50, poll_windows, g_strdup (out));
}
