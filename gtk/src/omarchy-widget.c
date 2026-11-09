/* omarchy-widget.c: what every kit widget does the same way: follow the theme, track hover,
 * pressed and keyboard focus, take clicks and keys. */
#include "omarchy-private.h"

static void
queue_draw (gpointer widget)
{
  gtk_widget_queue_draw (GTK_WIDGET (widget));
}

static void
queue_resize (gpointer widget)
{
  gtk_widget_queue_resize (GTK_WIDGET (widget));
}

static void
state_changed (GtkWidget *w, GtkStateFlags old, gpointer data)
{
  GtkStateFlags now = gtk_widget_get_state_flags (w);
  const GtkStateFlags watched = GTK_STATE_FLAG_PRELIGHT | GTK_STATE_FLAG_ACTIVE | GTK_STATE_FLAG_FOCUSED
                                | GTK_STATE_FLAG_FOCUS_VISIBLE | GTK_STATE_FLAG_FOCUS_WITHIN
                                | GTK_STATE_FLAG_INSENSITIVE | GTK_STATE_FLAG_DIR_LTR | GTK_STATE_FLAG_DIR_RTL;
  if ((now ^ old) & watched)
    gtk_widget_queue_draw (w);
}

void
omarchy_widget_follow (GtkWidget *w)
{
  OmarchyTheme *theme = omarchy_theme_get_default ();
  g_signal_connect_object (theme, "repaint", G_CALLBACK (queue_draw), w, G_CONNECT_SWAPPED);
  g_signal_connect_object (theme, "restyle", G_CALLBACK (queue_resize), w, G_CONNECT_SWAPPED);
  g_signal_connect (w, "state-flags-changed", G_CALLBACK (state_changed), NULL);
}

gboolean
omarchy_widget_hovered (GtkWidget *w)
{
  return (gtk_widget_get_state_flags (w) & GTK_STATE_FLAG_PRELIGHT) != 0;
}

gboolean
omarchy_widget_focus_shown (GtkWidget *w)
{
  GtkStateFlags f = gtk_widget_get_state_flags (w);
  return (f & GTK_STATE_FLAG_FOCUSED) && (f & GTK_STATE_FLAG_FOCUS_VISIBLE);
}

gboolean
omarchy_widget_pressed (GtkWidget *w)
{
  return (gtk_widget_get_state_flags (w) & GTK_STATE_FLAG_ACTIVE) != 0;
}

static void
click_pressed (GtkGestureClick *g, int n, double x, double y, GtkWidget *w)
{
  gtk_widget_set_state_flags (w, GTK_STATE_FLAG_ACTIVE, FALSE);
}

static void
click_released (GtkGestureClick *g, int n, double x, double y, GtkWidget *w)
{
  gtk_widget_unset_state_flags (w, GTK_STATE_FLAG_ACTIVE);
  OmarchyClicked clicked = g_object_get_data (G_OBJECT (g), "omarchy-clicked");
  guint button = gtk_gesture_single_get_current_button (GTK_GESTURE_SINGLE (g));
  if (button == 0) /* a test's synthesized click (tests/drive.c) */
    button = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (g), "drive-button"));
  /* Released outside the widget is no click. */
  if (x < 0 || y < 0 || x > gtk_widget_get_width (w) || y > gtk_widget_get_height (w))
    if (gtk_widget_get_width (w) > 0)
      return;
  if (clicked)
    clicked (w, button == 0 ? GDK_BUTTON_PRIMARY : button, x, y);
}

static void
click_cancel (GtkGesture *g, GdkEventSequence *seq, GtkWidget *w)
{
  gtk_widget_unset_state_flags (w, GTK_STATE_FLAG_ACTIVE);
}

GtkGesture *
omarchy_widget_add_click (GtkWidget *w, OmarchyClicked clicked)
{
  GtkGesture *g = gtk_gesture_click_new ();
  gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (g), 0);
  g_object_set_data (G_OBJECT (g), "omarchy-clicked", clicked);
  g_signal_connect (g, "pressed", G_CALLBACK (click_pressed), w);
  g_signal_connect (g, "released", G_CALLBACK (click_released), w);
  g_signal_connect (g, "cancel", G_CALLBACK (click_cancel), w);
  gtk_widget_add_controller (w, GTK_EVENT_CONTROLLER (g));
  gtk_widget_set_cursor_from_name (w, "pointer");
  return g;
}

static gboolean
key_pressed (GtkEventControllerKey *c, guint keyval, guint keycode, GdkModifierType state, GtkWidget *w)
{
  OmarchyKeyPressed pressed = g_object_get_data (G_OBJECT (c), "omarchy-keys");
  return pressed ? pressed (w, keyval, state) : FALSE;
}

GtkEventController *
omarchy_widget_add_keys (GtkWidget *w, OmarchyKeyPressed pressed)
{
  GtkEventController *c = gtk_event_controller_key_new ();
  g_object_set_data (G_OBJECT (c), "omarchy-keys", pressed);
  g_signal_connect (c, "key-pressed", G_CALLBACK (key_pressed), w);
  gtk_widget_add_controller (w, c);
  return c;
}

gboolean
omarchy_key_is_activate (guint keyval)
{
  return keyval == GDK_KEY_Return || keyval == GDK_KEY_KP_Enter || keyval == GDK_KEY_ISO_Enter
         || keyval == GDK_KEY_space || keyval == GDK_KEY_KP_Space;
}

guint
omarchy_reading_key (GtkWidget *w, guint keyval, gboolean vim)
{
  if (vim && keyval == GDK_KEY_h)
    keyval = GDK_KEY_Left;
  else if (vim && keyval == GDK_KEY_l)
    keyval = GDK_KEY_Right;
  if (gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL) {
    if (keyval == GDK_KEY_Left)
      return GDK_KEY_Right;
    if (keyval == GDK_KEY_Right)
      return GDK_KEY_Left;
  }
  return keyval;
}

void
omarchy_widget_dispose_children (GtkWidget *w)
{
  GtkWidget *child;
  while ((child = gtk_widget_get_first_child (w)))
    gtk_widget_unparent (child);
}
