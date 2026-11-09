/* A tiny Omarchy panel in GTK 4 and C, following the live theme.
 *
 *   cc main.c -o network $(pkg-config --cflags --libs omarchy-gtk-1) && ./network
 *
 * Switch themes while it runs. OMARCHY_GTK_SNAP=out.png renders it once and exits. */
#include <omarchy-gtk.h>

static void
on_wifi (OmarchyToggle *toggle, gpointer data)
{
  omarchy_toggle_set_description (toggle, omarchy_toggle_get_checked (toggle) ? "On" : "Off");
}

static void
activate (GtkApplication *app, gpointer data)
{
  /* The one call: Omarchy's look on GTK and libadwaita, following the desktop. */
  OmarchyTheme *theme = omarchy_gtk_init ();
  int pad = omarchy_theme_get_spacing (theme, "panelPadding");

  GtkWidget *pane = omarchy_pane_new (NULL);
  omarchy_pane_set_bordered (OMARCHY_PANE (pane), TRUE);
  GtkWidget *heading = omarchy_heading_new ("Network");
  omarchy_heading_set_subtitle (OMARCHY_HEADING (heading), "Wi-Fi and saved networks");
  omarchy_heading_set_level (OMARCHY_HEADING (heading), "heading");
  omarchy_pane_append (OMARCHY_PANE (pane), heading);

  GtkWidget *wifi = omarchy_toggle_new ("Wi-Fi", "On");
  omarchy_toggle_set_checked (OMARCHY_TOGGLE (wifi), TRUE);
  g_signal_connect (wifi, "toggled", G_CALLBACK (on_wifi), NULL);
  omarchy_pane_append (OMARCHY_PANE (pane), wifi);

  GtkWidget *row = omarchy_list_row_new ("Basecamp");
  omarchy_list_row_set_glyph (OMARCHY_LIST_ROW (row), "\U000F0928");
  omarchy_list_row_set_status (OMARCHY_LIST_ROW (row), "Connected", "success");
  omarchy_list_row_set_value (OMARCHY_LIST_ROW (row), "5 GHz");
  omarchy_list_row_set_current (OMARCHY_LIST_ROW (row), TRUE);
  omarchy_pane_append (OMARCHY_PANE (pane), row);

  GtkWidget *forget = omarchy_button_new ("Forget network");
  omarchy_button_set_bordered (OMARCHY_BUTTON (forget), TRUE);
  gtk_widget_set_halign (forget, GTK_ALIGN_START);
  omarchy_pane_append (OMARCHY_PANE (pane), forget);

  gtk_widget_set_margin_start (pane, pad);
  gtk_widget_set_margin_end (pane, pad);
  gtk_widget_set_margin_top (pane, pad);
  gtk_widget_set_valign (pane, GTK_ALIGN_START);

  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Network");
  gtk_window_set_default_size (GTK_WINDOW (window), 400, 340);
  gtk_window_set_child (GTK_WINDOW (window), pane);
  gtk_window_present (GTK_WINDOW (window));
}

int
main (int argc, char **argv)
{
  g_autoptr (GtkApplication) app = gtk_application_new ("org.omarchy.examples.Network", G_APPLICATION_NON_UNIQUE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
