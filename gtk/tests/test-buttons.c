/* Buttons, IconButton and ButtonGroup: clicks and keys. */
#include <omarchy-gtk.h>

#include "drive.h"

static int clicks;

static void
count (GtkWidget *w, gpointer data)
{
  clicks++;
}

TEST (test_button_clicks_and_keys, "/buttons/button-clicks-and-keys")
{
  GtkWidget *b = omarchy_button_new ("Save");
  GtkWidget *window = drive_window (b, 200, 80);
  clicks = 0;
  g_signal_connect (b, "clicked", G_CALLBACK (count), NULL);
  drive_click (b, -1, -1);
  g_assert_cmpint (clicks, ==, 1);
  gtk_widget_grab_focus (b);
  g_assert_true (drive_key (window, NULL, GDK_KEY_Return, 0));
  g_assert_true (drive_key (window, NULL, GDK_KEY_space, 0));
  g_assert_cmpint (clicks, ==, 3);
  g_assert_false (drive_key (window, NULL, GDK_KEY_a, 0));
  g_assert_cmpint (clicks, ==, 3);
  drive_close (window);
}

TEST (test_icon_button_clicks, "/buttons/icon-button")
{
  GtkWidget *b = omarchy_icon_button_new ("+");
  gtk_widget_set_halign (b, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (b, GTK_ALIGN_CENTER);
  GtkWidget *window = drive_window (b, 100, 80);
  clicks = 0;
  g_signal_connect (b, "clicked", G_CALLBACK (count), NULL);
  drive_click (b, -1, -1);
  gtk_widget_grab_focus (b);
  drive_key (window, NULL, GDK_KEY_KP_Enter, 0);
  g_assert_cmpint (clicks, ==, 2);
  /* The size is whole and odd. */
  int w = gtk_widget_get_width (b);
  g_assert_cmpint (w % 2, ==, 1);
  drive_close (window);
}

static char *chosen;

static void
on_changed (OmarchyButtonGroup *g, const char *value, gpointer data)
{
  g_free (chosen);
  chosen = g_strdup (value);
}

TEST (test_button_group_chooses_on_click_and_keys, "/buttons/group")
{
  GtkWidget *g = omarchy_button_group_new ();
  OmarchyButtonGroup *group = OMARCHY_BUTTON_GROUP (g);
  omarchy_button_group_append (group, "left", "Left");
  omarchy_button_group_append (group, "center", "Center");
  omarchy_button_group_append (group, "right", "Right");
  omarchy_button_group_set_value (group, "center");
  g_signal_connect (g, "changed", G_CALLBACK (on_changed), NULL);
  GtkWidget *window = drive_window (g, 300, 80);

  /* A click on a chip chooses it. */
  GtkWidget *right = gtk_widget_get_last_child (g);
  drive_click (right, -1, -1);
  g_assert_cmpstr (chosen, ==, "right");
  g_assert_cmpstr (omarchy_button_group_get_value (group), ==, "right");

  /* Focus lands on the selected chip; h/Left walk back, Return chooses. */
  gtk_widget_grab_focus (g);
  g_assert_cmpint (omarchy_button_group_get_focused_index (group), ==, 2);
  drive_key (window, NULL, GDK_KEY_h, 0);
  g_assert_cmpint (omarchy_button_group_get_focused_index (group), ==, 1);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_cmpint (omarchy_button_group_get_focused_index (group), ==, 0);
  g_assert_true (omarchy_button_get_cursor (OMARCHY_BUTTON (gtk_widget_get_first_child (g))));
  drive_key (window, NULL, GDK_KEY_Return, 0);
  g_assert_cmpstr (chosen, ==, "left");
  drive_key (window, NULL, GDK_KEY_l, 0);
  drive_key (window, NULL, GDK_KEY_space, 0);
  g_assert_cmpstr (chosen, ==, "center");
  drive_close (window);
}
