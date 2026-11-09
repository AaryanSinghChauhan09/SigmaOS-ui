/* drive.c: see drive.h. */
#include "drive.h"
#include "headless.h"

#define MAX_TESTS 256
static struct {
  const char *path;
  TestFunc func;
} tests[MAX_TESTS];
static int n_tests;

void
test_register (const char *path, TestFunc func)
{
  if (n_tests < MAX_TESTS)
    tests[n_tests++] = (typeof (tests[0])) { path, func };
}

void test_add_registered (void);
void
test_add_registered (void)
{
  for (int i = 0; i < n_tests; i++)
    g_test_add_func (tests[i].path, tests[i].func);
}

void
drive_wait (guint ms)
{
  headless_wait (ms);
}

GtkWidget *
drive_window (GtkWidget *child, int width, int height)
{
  GtkWidget *window = gtk_window_new ();
  gtk_window_set_child (GTK_WINDOW (window), child);
  headless_present (GTK_WINDOW (window), width, height);
  return window;
}

void
drive_close (GtkWidget *window)
{
  gtk_window_destroy (GTK_WINDOW (window));
  drive_wait (10);
}

static gboolean
emit_keys (GtkWidget *w, GtkPropagationPhase phase, guint keyval, GdkModifierType mods)
{
  GListModel *list = gtk_widget_observe_controllers (w);
  gboolean handled = FALSE;
  guint n = g_list_model_get_n_items (list);
  for (guint i = 0; i < n && !handled; i++) {
    g_autoptr (GtkEventController) c = g_list_model_get_item (list, i);
    if (!GTK_IS_EVENT_CONTROLLER_KEY (c) || gtk_event_controller_get_propagation_phase (c) != phase)
      continue;
    g_signal_emit_by_name (c, "key-pressed", keyval, 0, mods, &handled);
  }
  g_object_unref (list);
  return handled;
}

gboolean
drive_key (GtkWidget *window, GtkWidget *target, guint keyval, GdkModifierType mods)
{
  if (!target)
    target = gtk_root_get_focus (GTK_ROOT (window));
  if (!target)
    target = window;
  GPtrArray *chain = g_ptr_array_new ();
  for (GtkWidget *w = target; w; w = gtk_widget_get_parent (w))
    g_ptr_array_add (chain, w);
  gboolean handled = FALSE;
  for (int i = (int) chain->len - 1; i >= 0 && !handled; i--)
    handled = emit_keys (g_ptr_array_index (chain, i), GTK_PHASE_CAPTURE, keyval, mods);
  for (guint i = 0; i < chain->len && !handled; i++)
    handled = emit_keys (g_ptr_array_index (chain, i), GTK_PHASE_BUBBLE, keyval, mods)
              || emit_keys (g_ptr_array_index (chain, i), GTK_PHASE_TARGET, keyval, mods);
  g_ptr_array_unref (chain);
  drive_wait (5);
  return handled;
}

static void
click (GtkWidget *widget, double x, double y, guint button)
{
  if (x < 0) {
    x = gtk_widget_get_width (widget) / 2.0;
    y = gtk_widget_get_height (widget) / 2.0;
  }
  GtkWidget *hit = gtk_widget_pick (widget, x, y, GTK_PICK_DEFAULT);
  if (!hit)
    hit = widget;
  for (GtkWidget *w = hit; w; w = gtk_widget_get_parent (w)) {
    graphene_point_t p;
    if (!gtk_widget_compute_point (widget, w, &GRAPHENE_POINT_INIT (x, y), &p))
      continue;
    GListModel *list = gtk_widget_observe_controllers (w);
    gboolean done = FALSE;
    for (guint i = 0; i < g_list_model_get_n_items (list); i++) {
      g_autoptr (GtkEventController) c = g_list_model_get_item (list, i);
      if (!GTK_IS_GESTURE_CLICK (c))
        continue;
      guint want = gtk_gesture_single_get_button (GTK_GESTURE_SINGLE (c));
      if (want != 0 && want != button)
        continue;
      g_object_set_data (G_OBJECT (c), "drive-button", GUINT_TO_POINTER (button));
      g_signal_emit_by_name (c, "pressed", 1, p.x, p.y);
      g_signal_emit_by_name (c, "released", 1, p.x, p.y);
      done = TRUE;
    }
    g_object_unref (list);
    if (done)
      break;
    if (w == widget)
      break;
  }
  drive_wait (5);
}

void
drive_click (GtkWidget *widget, double x, double y)
{
  click (widget, x, y, GDK_BUTTON_PRIMARY);
}

void
drive_right_click (GtkWidget *widget, double x, double y)
{
  click (widget, x, y, GDK_BUTTON_SECONDARY);
}

void
drive_hover (GtkWidget *widget, gboolean over)
{
  if (over)
    gtk_widget_set_state_flags (widget, GTK_STATE_FLAG_PRELIGHT, FALSE);
  else
    gtk_widget_unset_state_flags (widget, GTK_STATE_FLAG_PRELIGHT);
  drive_wait (5);
}

static GtkWidget *
find (GtkWidget *w, GType type, int *n)
{
  if (G_TYPE_CHECK_INSTANCE_TYPE (w, type) && (*n)-- == 0)
    return w;
  for (GtkWidget *c = gtk_widget_get_first_child (w); c; c = gtk_widget_get_next_sibling (c)) {
    GtkWidget *hit = find (c, type, n);
    if (hit)
      return hit;
  }
  return NULL;
}

GtkWidget *
drive_find (GtkWidget *root, GType type, int n)
{
  return find (root, type, &n);
}
