/* omarchy-gtk-headless: run a GTK program on a private broadway display nobody sees.
 *
 *   omarchy-gtk-headless [--scale N] CMD [ARGS...]
 *
 * With OMARCHY_GTK_SNAP=out.png set, an app that made the one call (omarchy_gtk_init(),
 * Omarchy.init()) renders its first window there and exits: how the examples are rendered
 * in tests, in C, Python, GJS, Vala and Rust alike. The exit status is CMD's. */
#include "headless.h"

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int
main (int argc, char **argv)
{
  int first = 1, scale = 2;
  if (argc > 2 && !strcmp (argv[1], "--scale")) {
    scale = atoi (argv[2]);
    first = 3;
  }
  if (first >= argc) {
    g_printerr ("usage: omarchy-gtk-headless [--scale N] CMD [ARGS...]\n");
    return 2;
  }
  if (!headless_start (scale))
    return 1;
  g_auto (GStrv) env = headless_child_env ();
  int status = 0;
  g_autoptr (GError) error = NULL;
  if (!g_spawn_sync (NULL, argv + first, env, G_SPAWN_SEARCH_PATH | G_SPAWN_CHILD_INHERITS_STDIN,
                     NULL, NULL, NULL, NULL, &status, &error)) {
    g_printerr ("omarchy-gtk-headless: %s\n", error->message);
    headless_stop ();
    return 127;
  }
  headless_stop ();
  return WIFEXITED (status) ? WEXITSTATUS (status) : 1;
}
