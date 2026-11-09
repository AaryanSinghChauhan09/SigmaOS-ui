/* headless.h: GTK with no window on the desktop, for snaps and tests. */
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Start a private broadway display (gtk4-broadwayd on a unix socket, no TCP port) and point GDK
 * at it, before gtk_init(). Nothing is ever shown on the user's desktop; the daemon dies with
 * us. A stand-in viewer keeps frames coming and sets the display's scale (device pixels per
 * point). FALSE (with a message on stderr) when it can't. */
gboolean headless_start (int scale);
void headless_stop (void);

/* Run the main loop for `ms` milliseconds. */
void headless_wait (guint ms);
/* Present `window` (undecorated, w × h), and wait until it is mapped, allocated and painted. */
void headless_present (GtkWindow *window, int width, int height);
/* Render `widget` at `scale` device pixels per point into a texture, offscreen. */
GdkTexture *headless_render (GtkWidget *widget, double scale);

G_END_DECLS
