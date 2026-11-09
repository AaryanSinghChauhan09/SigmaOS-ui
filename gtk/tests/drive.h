/* drive.h: keys and clicks for behavior tests, through the widgets' own event controllers.
 *
 * GTK 4 has no public way to synthesize input events, so a test emits the signals GTK would:
 * key-pressed on the key controllers between the focus widget and the root (capture phase from
 * the root down, then bubble back up, stopping when one handles it), and pressed/released on the
 * click gesture of the widget under a point. */
#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef void (*TestFunc) (void);
void test_register (const char *path, TestFunc func);
#define TEST(name, path)                                                                   \
  static void name (void);                                                                 \
  static void __attribute__ ((constructor)) name##_register (void) { test_register (path, name); } \
  static void name (void)

/* A window on the private display holding `child`, presented and settled. */
GtkWidget *drive_window (GtkWidget *child, int width, int height);
void drive_close (GtkWidget *window);
/* Key `keyval` to `target` (or the window's focus widget when NULL). TRUE when handled. */
gboolean drive_key (GtkWidget *window, GtkWidget *target, guint keyval, GdkModifierType mods);
/* A primary click at (x, y) in `widget`'s coordinates (center when x < 0). */
void drive_click (GtkWidget *widget, double x, double y);
void drive_right_click (GtkWidget *widget, double x, double y);
/* Pointer over `widget` (PRELIGHT), or off it. */
void drive_hover (GtkWidget *widget, gboolean over);
/* Let the main loop run. */
void drive_wait (guint ms);
/* Find the first descendant of `root` of `type` (depth first), the nth when n > 0. */
GtkWidget *drive_find (GtkWidget *root, GType type, int n);

G_END_DECLS
