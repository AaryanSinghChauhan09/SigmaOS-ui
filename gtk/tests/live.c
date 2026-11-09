/* omarchy-gtk-live-test: the GTK binding's live-switch test (spec/following.md).
 *
 *   tools/live/run build/gtk/omarchy-gtk-live-test
 *   OMARCHY_UI_REDUCE_MOTION=1 tools/live/run build/gtk/omarchy-gtk-live-test
 *
 * A simulated `omarchy theme set` reaches a running GTK app that made the one call: the theme
 * reads catppuccin-latte's background, ::changed fired once with the new tokens (and the
 * accessors already read them in the hook, before the fade), the revision is 1, and the colors
 * faded (kit widgets through ::repaint, stock widgets through the CSS) and landed exactly — or,
 * under reduced motion, landed at once. On a private broadway display: no window shows. */
#include <omarchy-gtk.h>
#include <stdio.h>
#include <string.h>

#include "headless.h"

static const GdkRGBA TOKYO = { 0x1a / 255.0, 0x1b / 255.0, 0x26 / 255.0, 1 };
static const GdkRGBA LATTE = { 0xef / 255.0, 0xf1 / 255.0, 0xf5 / 255.0, 1 };

static int failures;
static int hooks;
static gboolean hook_saw_latte, hook_saw_fading = TRUE, hook_tokens_latte, hook_tokens_light;
static GArray *paints;  /* palette.background on every repaint */
static GArray *stock;   /* a stock label's CSS color, every frame */
static GtkWidget *label;

#define CHECK(cond, ...)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      failures++;                                                              \
      fprintf (stderr, "FAIL %s:%d: ", __FILE__, __LINE__);                    \
      fprintf (stderr, __VA_ARGS__);                                           \
      fprintf (stderr, "\n");                                                  \
    }                                                                          \
  } while (0)

static gboolean
near (const GdkRGBA *a, const GdkRGBA *b)
{
  return fabs (a->red - b->red) < 0.002 && fabs (a->green - b->green) < 0.002 && fabs (a->blue - b->blue) < 0.002
         && fabs (a->alpha - b->alpha) < 0.002;
}

static gboolean
between (GArray *list, const GdkRGBA *a, const GdkRGBA *b)
{
  for (guint i = 0; i < list->len; i++) {
    GdkRGBA *c = &g_array_index (list, GdkRGBA, i);
    if (!near (c, a) && !near (c, b))
      return TRUE;
  }
  return FALSE;
}

static void
on_changed (OmarchyTheme *theme, GVariant *tokens, gpointer data)
{
  hooks++;
  g_autoptr (GdkRGBA) bg = omarchy_theme_get_color (theme, "palette.background");
  hook_saw_latte = bg && near (bg, &LATTE);
  hook_saw_fading = omarchy_theme_get_fading (theme);
  const char *mode = NULL, *background = NULL;
  g_autoptr (GVariant) palette = g_variant_lookup_value (tokens, "palette", G_VARIANT_TYPE_VARDICT);
  if (palette)
    g_variant_lookup (palette, "background", "&s", &background);
  g_variant_lookup (tokens, "mode", "&s", &mode);
  hook_tokens_latte = !g_strcmp0 (background, "#eff1f5ff");
  hook_tokens_light = !g_strcmp0 (mode, "light");
}

static void
on_repaint (OmarchyTheme *theme, gpointer data)
{
  g_autoptr (GdkRGBA) c = omarchy_theme_get_paint_color (theme, "palette.background");
  g_array_append_val (paints, *c);
}

static gboolean
on_tick (GtkWidget *w, GdkFrameClock *clock, gpointer data)
{
  GdkRGBA c;
  gtk_widget_get_color (label, &c);
  g_array_append_val (stock, c);
  return G_SOURCE_CONTINUE;
}

static void
wait_until (gboolean (*cond) (void), guint ms)
{
  for (guint t = 0; t < ms && !cond (); t += 10)
    headless_wait (10);
}

static gboolean
hooked (void)
{
  return hooks > 0;
}

static gboolean
settled (void)
{
  return !omarchy_theme_get_fading (omarchy_theme_get_default ());
}

int
main (int argc, char **argv)
{
  const char *home = g_getenv ("OMARCHY_UI_LIVE_HOME");
  const char *switcher = g_getenv ("OMARCHY_UI_LIVE_SWITCH");
  if (!home || !switcher) {
    printf ("skipped: run under tools/live/run\n");
    return 0;
  }
  if (!headless_start (1))
    return 1;
  paints = g_array_new (FALSE, FALSE, sizeof (GdkRGBA));
  stock = g_array_new (FALSE, FALSE, sizeof (GdkRGBA));

  OmarchyTheme *theme = omarchy_gtk_init ();
  g_signal_connect (theme, "changed", G_CALLBACK (on_changed), NULL);
  g_signal_connect (theme, "repaint", G_CALLBACK (on_repaint), NULL);

  /* A window with a kit card and a stock label, so both paths are live. */
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 8);
  label = gtk_label_new ("Stock");
  gtk_box_append (GTK_BOX (box), label);
  GtkWidget *card = omarchy_card_new ("Network");
  omarchy_card_append (OMARCHY_CARD (card), omarchy_button_new ("Kit"));
  gtk_box_append (GTK_BOX (box), card);
  GtkWidget *window = gtk_window_new ();
  gtk_window_set_child (GTK_WINDOW (window), box);
  headless_present (GTK_WINDOW (window), 320, 200);
  gtk_widget_add_tick_callback (window, on_tick, NULL, NULL);

  g_autoptr (GdkRGBA) start = omarchy_theme_get_color (theme, "palette.background");
  CHECK (near (start, &TOKYO), "starts on tokyo-night");
  CHECK (omarchy_theme_get_revision (theme) == 0, "revision 0 at start");
  headless_wait (300);
  CHECK (hooks == 0, "the initial load is not a change");

  g_array_set_size (paints, 0);
  g_array_set_size (stock, 0);
  char *args[] = { (char *) switcher, (char *) home, "catppuccin-latte", NULL };
  g_autoptr (GError) error = NULL;
  int status = 0;
  CHECK (g_spawn_sync (NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &status, &error), "theme-set ran");
  wait_until (hooked, 5000);
  CHECK (hooks == 1, "the hook fired once (%d)", hooks);
  CHECK (hook_tokens_latte, "the hook got the new tokens");
  CHECK (hook_tokens_light, "with mode light");
  CHECK (hook_saw_latte, "the accessors read the new theme in the hook");
  CHECK (!hook_saw_fading, "and not a frame of the fade");
  CHECK (omarchy_theme_get_revision (theme) == 1, "revision 1");

  gboolean reduced = omarchy_theme_get_reduce_motion (theme);
  if (reduced) {
    CHECK (!omarchy_theme_get_fading (theme), "no fade under reduced motion");
    headless_wait (200);
    CHECK (!between (paints, &TOKYO, &LATTE), "nothing in between on the kit's paint");
  } else {
    CHECK (omarchy_theme_get_fading (theme), "the colors fade");
    wait_until (settled, 2000);
    CHECK (!omarchy_theme_get_fading (theme), "the fade ends");
    CHECK (between (paints, &TOKYO, &LATTE), "the kit's background passed through colors between (%u frames)", paints->len);
  }
  headless_wait (100);
  /* The stock label: tokyo-night's foreground to latte's, through the CSS. */
  GdkRGBA fg_tokyo, fg_latte;
  gdk_rgba_parse (&fg_tokyo, "#a9b1d6");
  gdk_rgba_parse (&fg_latte, "#4c4f69");
  if (!reduced)
    CHECK (between (stock, &fg_tokyo, &fg_latte), "a stock label's color faded through the CSS (%u frames)", stock->len);
  GdkRGBA now;
  gtk_widget_get_color (label, &now);
  CHECK (near (&now, &fg_latte), "the stock label lands on latte's foreground");
  g_autoptr (GdkRGBA) paint = omarchy_theme_get_paint_color (theme, "palette.background");
  CHECK (gdk_rgba_equal (paint, &LATTE) || near (paint, &LATTE), "it lands exactly");
  g_autoptr (GdkRGBA) card_bg = omarchy_theme_get_paint_color (theme, "surfaces.popups.background");
  CHECK (near (card_bg, &LATTE), "the cards land too");

  headless_wait (600);
  CHECK (hooks == 1, "one switch is one change (%d)", hooks);
  gtk_window_destroy (GTK_WINDOW (window));
  headless_stop ();
  if (failures)
    return 1;
  printf ("a switch reached GTK once, revision 1, %s\n",
          reduced ? "no fade under reduced motion" : "colors faded on kit and stock widgets and landed exactly");
  return 0;
}
