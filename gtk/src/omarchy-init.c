/* omarchy-init.c: the one call. */
#include "omarchy-private.h"

#include <adwaita.h>

/**
 * omarchy_gtk_init: (rename-to init)
 *
 * Adopt Omarchy's look and follow the theme live, in one call. After gtk_init() (it calls it
 * when GTK isn't initialized yet), before the first window shows:
 *
 * - initializes libadwaita and sets its color scheme from the theme's mode;
 * - installs CSS on the default display that restyles stock GTK and libadwaita widgets the
 *   Omarchy way (foreground glass, Hyprland's rounding, the fontconfig monospace font at the
 *   token sizes, flat, focus like hover, accent only for what's chosen), libadwaita's color
 *   variables (accent included) among them;
 * - follows the desktop through libomarchy-theme's watch on the main loop, fading colors over
 *   `motion.theme` on a theme switch;
 * - registers the kit's widget types, so GtkBuilder files can name them.
 *
 * Calling it again does nothing more.
 *
 * Returns: (transfer none): the theme, [func@Omarchy.Theme.get_default]
 */
OmarchyTheme *
omarchy_gtk_init (void)
{
  static gboolean done;
  if (!gtk_is_initialized ())
    gtk_init ();
  OmarchyTheme *theme = omarchy_theme_get_default ();
  if (done)
    return theme;
  done = TRUE;
  adw_init ();
  GdkDisplay *display = gdk_display_get_default ();
  if (display)
    omarchy_css_install (display);
  else
    g_warning ("omarchy-gtk: no display; the stock widgets keep their look");
  omarchy_ensure_types ();
  return theme;
}

/**
 * omarchy_get_version:
 *
 * Returns: (transfer none): the library's version
 */
const char *
omarchy_get_version (void)
{
  return OMARCHY_GTK_VERSION;
}
