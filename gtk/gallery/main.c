/* The GTK gallery: every Omarchy UI component in the live theme, following theme switches.
 *
 *   omarchy-gtk-gallery                              a window, following the desktop theme
 *   omarchy-gtk-gallery --section buttons            one section (spec/gallery.md)
 *   omarchy-gtk-gallery --list-sections              the sections, in order
 *   omarchy-gtk-gallery --snap out.png --section buttons --size 760x220 --scale 2
 *                                                    render offscreen to a PNG and exit
 *   omarchy-gtk-gallery --dump-tokens                the tokens it draws with, as canonical JSON
 *   omarchy-gtk-gallery --watch-tokens               one line now, then one per change (the hook)
 *   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-gtk-gallery --snap nord.png
 *
 * --snap renders on a private broadway display (gallery/headless.c): no window ever shows on
 * the desktop. The OMARCHY_UI_* overrides reach the tokens through libomarchy-theme. */
#include "gallery.h"
#include "headless.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *opt_section;
static char *opt_snap;
static char *opt_size;
static double opt_scale = 2;
static int opt_width = 760;
static gboolean opt_list;
static gboolean opt_dump;
static gboolean opt_watch;

static const GOptionEntry entries[] = {
  { "section", 0, 0, G_OPTION_ARG_STRING, &opt_section, "Show only this section", "NAME" },
  { "snap", 0, 0, G_OPTION_ARG_FILENAME, &opt_snap, "Render offscreen to this PNG and exit", "OUT" },
  { "size", 0, 0, G_OPTION_ARG_STRING, &opt_size, "Size in points for --snap (default 760 wide, as tall as it is)", "WxH" },
  { "scale", 0, 0, G_OPTION_ARG_DOUBLE, &opt_scale, "Device pixels per point for --snap (default 2)", "N" },
  { "width", 0, 0, G_OPTION_ARG_INT, &opt_width, "Width in points when --size is not given (default 760)", "N" },
  { "list-sections", 0, 0, G_OPTION_ARG_NONE, &opt_list, "Print the sections, one per line, and exit", NULL },
  { "dump-tokens", 0, 0, G_OPTION_ARG_NONE, &opt_dump, "Print the tokens as canonical JSON and exit", NULL },
  { "watch-tokens", 0, 0, G_OPTION_ARG_NONE, &opt_watch, "Print the tokens, then a line per change", NULL },
  { NULL },
};

static GtkWidget *
build_page (const char *only)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  int pad = omarchy_theme_get_spacing (theme, "panelPadding");
  GtkWidget *page = gtk_box_new (GTK_ORIENTATION_VERTICAL, omarchy_theme_get_spacing (theme, "huge") * 2);
  gtk_widget_set_margin_start (page, pad);
  gtk_widget_set_margin_end (page, pad);
  gtk_widget_set_margin_top (page, pad);
  gtk_widget_set_margin_bottom (page, pad);
  gtk_widget_set_valign (page, GTK_ALIGN_START);
  for (const GallerySection *s = gallery_sections; s->name; s++)
    if (!only || !strcmp (only, s->name))
      gtk_box_append (GTK_BOX (page), s->build ());
  return page;
}

static void
on_changed (OmarchyTheme *theme, GVariant *tokens, gpointer data)
{
  printf ("%s\n", omarchy_theme_get_tokens_json (theme));
  fflush (stdout);
}

static int
snap (void)
{
  int w = opt_width, h = 0;
  if (opt_size && sscanf (opt_size, "%dx%d", &w, &h) < 1) {
    g_printerr ("--size wants WxH\n");
    return 2;
  }
  if (!headless_start ((int) (opt_scale + 0.5)))
    return 1;
  omarchy_gtk_init ();
  GtkWidget *page = build_page (opt_section);
  if (h <= 0) {
    /* As tall as the page, as Qt's gallery window is (at least 200). */
    int nat;
    gtk_widget_measure (page, GTK_ORIENTATION_VERTICAL, w, NULL, &nat, NULL, NULL);
    h = MAX (200, nat);
  }
  GtkWidget *window = gtk_window_new ();
  GtkWidget *scroller = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller), GTK_POLICY_EXTERNAL, GTK_POLICY_EXTERNAL);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroller), page);
  gtk_window_set_child (GTK_WINDOW (window), scroller);
  gtk_widget_add_css_class (window, "omarchy-snap");
  headless_present (GTK_WINDOW (window), w, h);
  GdkTexture *texture = headless_render (window, opt_scale);
  if (!texture || !gdk_texture_save_to_png (texture, opt_snap)) {
    g_printerr ("could not write %s\n", opt_snap);
    return 1;
  }
  g_object_unref (texture);
  gtk_window_destroy (GTK_WINDOW (window));
  headless_stop ();
  return 0;
}

static void
activate (GtkApplication *app, gpointer data)
{
  omarchy_gtk_init ();
  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Omarchy UI · GTK");
  gtk_window_set_default_size (GTK_WINDOW (window), opt_width, 900);
  GtkWidget *scroller = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scroller), build_page (opt_section));
  gtk_window_set_child (GTK_WINDOW (window), scroller);
  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  g_autoptr (GOptionContext) ctx = g_option_context_new ("- the Omarchy UI GTK gallery");
  g_option_context_add_main_entries (ctx, entries, NULL);
  g_autoptr (GError) error = NULL;
  if (!g_option_context_parse (ctx, &argc, &argv, &error)) {
    g_printerr ("%s\n", error->message);
    return 2;
  }
  if (opt_list) {
    for (const GallerySection *s = gallery_sections; s->name; s++)
      printf ("%s\n", s->name);
    return 0;
  }
  if (opt_dump || opt_watch) {
    /* No display needed: the theme object alone. */
    OmarchyTheme *theme = omarchy_theme_get_default ();
    if (opt_dump) {
      printf ("%s\n", omarchy_theme_get_tokens_json (theme));
      return 0;
    }
    printf ("%s\n", omarchy_theme_get_tokens_json (theme));
    fflush (stdout);
    g_signal_connect (theme, "changed", G_CALLBACK (on_changed), NULL);
    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);
    return 0;
  }
  if (opt_snap)
    return snap ();
  g_autoptr (GtkApplication) app = gtk_application_new ("org.omarchy.ui.GtkGallery", G_APPLICATION_NON_UNIQUE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  return g_application_run (G_APPLICATION (app), 1, argv);
}
