/* omarchy-gtk-tests: the behavior tests (tests/test-*.c), on a private broadway display. Keys and
 * clicks go through the widgets' own controllers (drive.c), mirroring qt/tests. */
#include <omarchy-gtk.h>

#include "drive.h"
#include "headless.h"

void test_add_registered (void);

int
main (int argc, char **argv)
{
  if (!headless_start (1))
    return 1;
  /* Deterministic tokens: a fixture theme, sharp corners, no motion, unless the caller set them. */
  g_setenv ("OMARCHY_UI_REDUCE_MOTION", "1", FALSE);
  g_setenv ("OMARCHY_UI_ROUNDING", "0", FALSE);
  if (!g_getenv ("OMARCHY_UI_THEME_DIR") && g_file_test ("fixtures/themes/tokyo-night", G_FILE_TEST_IS_DIR))
    g_setenv ("OMARCHY_UI_THEME_DIR", "fixtures/themes/tokyo-night", TRUE);
  omarchy_gtk_init ();
  g_test_init (&argc, &argv, NULL);
  /* Criticals fail a test; warnings (broadway's frame timing) don't. */
  g_log_set_always_fatal (G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL);
  test_add_registered ();
  int rc = g_test_run ();
  headless_stop ();
  return rc;
}
