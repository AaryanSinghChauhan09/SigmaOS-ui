/* Editing: Timeline, Scrubber, TrimHandle, CropFrame, DragHandle, DropIndicator and ColorPicker:
 * drags, clicks and keys (spec/components/editing.md). */
#include <omarchy-gtk.h>

#include <math.h>
#include <string.h>

#include "drive.h"

/* ---- drags: the signals GtkGestureDrag would emit, on `w`'s own drag gesture */

static GtkGesture *
drag_gesture (GtkWidget *w)
{
  GListModel *list = gtk_widget_observe_controllers (w);
  GtkGesture *found = NULL;
  for (guint i = 0; i < g_list_model_get_n_items (list) && !found; i++) {
    g_autoptr (GtkEventController) c = g_list_model_get_item (list, i);
    if (GTK_IS_GESTURE_DRAG (c))
      found = GTK_GESTURE (c);
  }
  g_object_unref (list);
  g_assert_nonnull (found);
  return found;
}

static void
drag (GtkWidget *w, double x, double y, double dx, double dy)
{
  GtkGesture *g = drag_gesture (w);
  g_signal_emit_by_name (g, "drag-begin", x, y);
  if (dx != 0 || dy != 0)
    g_signal_emit_by_name (g, "drag-update", dx, dy);
  g_signal_emit_by_name (g, "drag-end", dx, dy);
  drive_wait (5);
}

/* A press and release where it was pressed: a click on a drag-driven control. */
static void
press (GtkWidget *w, double x, double y)
{
  drag (w, x, y, 0, 0);
}

static int fired;
static double last_a, last_b;
static char *last_edge;

static void
on_double (GtkWidget *w, double v, gpointer d)
{
  fired++;
  last_a = v;
}

static void
on_two_doubles (GtkWidget *w, double a, double b, gpointer d)
{
  fired++;
  last_a = a;
  last_b = b;
}

static void
reset (void)
{
  fired = 0;
  last_a = last_b = -1;
  g_clear_pointer (&last_edge, g_free);
}

/* ================================================================== Scrubber */

TEST (test_scrubber_drag_and_keys, "/editing/scrubber")
{
  GtkWidget *s = omarchy_scrubber_new (42, 130);
  GtkWidget *window = drive_window (s, 360, 60);
  OmarchyScrubber *sc = OMARCHY_SCRUBBER (s);
  reset ();
  g_signal_connect (s, "seek", G_CALLBACK (on_double), NULL);
  int W = gtk_widget_get_width (s);

  /* A press at the far left of the bar seeks to 0; on the elapsed time, nothing. */
  press (s, 2, 20);
  g_assert_cmpint (fired, ==, 0);
  /* Find the bar's ends by pressing near them: the elapsed text ends well before W / 4. */
  press (s, W / 4.0, 20);
  g_assert_cmpint (fired, ==, 1);
  double quarter = omarchy_scrubber_get_position (sc);
  g_assert_cmpfloat (quarter, >, 0);
  g_assert_cmpfloat (quarter, <, 65);
  g_assert_cmpfloat (last_a, ==, quarter);

  /* Dragging right moves later, clamped at the duration; dragging back moves earlier. */
  drag (s, W / 4.0, 20, W, 0);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 130);
  drag (s, W / 2.0, 20, -W, 0);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 0);
  g_assert_false (omarchy_scrubber_get_dragging (sc));

  /* Keys: ±5 s, Shift ±30 s, Home/End, clamped. */
  omarchy_scrubber_set_position (sc, 42);
  gtk_widget_grab_focus (s);
  reset ();
  g_assert_true (drive_key (window, NULL, GDK_KEY_Right, 0));
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 47);
  drive_key (window, NULL, GDK_KEY_Right, GDK_SHIFT_MASK);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 77);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 72);
  drive_key (window, NULL, GDK_KEY_End, 0);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 130);
  drive_key (window, NULL, GDK_KEY_Right, GDK_SHIFT_MASK);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 130);
  drive_key (window, NULL, GDK_KEY_Home, 0);
  g_assert_cmpfloat (omarchy_scrubber_get_position (sc), ==, 0);
  drive_key (window, NULL, GDK_KEY_Left, 0);
  g_assert_cmpfloat (last_a, ==, 0);
  g_assert_cmpint (fired, ==, 7);
  g_assert_false (drive_key (window, NULL, GDK_KEY_a, 0));
  drive_close (window);
}

/* ================================================================ TrimHandle */

static int released;
static void on_released (GtkWidget *w, gpointer d) { released++; }

TEST (test_trim_handle_drag_and_nudge, "/editing/trim-handle")
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_widget_set_valign (box, GTK_ALIGN_START);
  GtkWidget *trim = omarchy_trim_handle_new ("left");
  GtkWidget *top = omarchy_trim_handle_new_crop ("top");
  GtkWidget *side = omarchy_trim_handle_new_crop ("right");
  GtkWidget *corner = omarchy_trim_handle_new_crop ("bottom-right");
  gtk_box_append (GTK_BOX (box), trim);
  gtk_box_append (GTK_BOX (box), top);
  gtk_box_append (GTK_BOX (box), side);
  gtk_box_append (GTK_BOX (box), corner);
  for (GtkWidget *c = gtk_widget_get_first_child (box); c; c = gtk_widget_get_next_sibling (c))
    gtk_widget_set_valign (c, GTK_ALIGN_START);
  GtkWidget *window = drive_window (box, 300, 100);

  /* Sizes: a trim cap is space(8) × space(40); a crop bar space(18) × space(4); a corner space(18) square. */
  OmarchyTheme *theme = omarchy_theme_get_default ();
  g_assert_cmpint (gtk_widget_get_width (trim), ==, omarchy_theme_space (theme, 8));
  g_assert_cmpint (gtk_widget_get_height (trim), ==, omarchy_theme_space (theme, 40));
  g_assert_cmpint (gtk_widget_get_width (top), ==, omarchy_theme_space (theme, 18));
  g_assert_cmpint (gtk_widget_get_height (top), ==, omarchy_theme_space (theme, 4));
  g_assert_cmpint (gtk_widget_get_width (corner), ==, gtk_widget_get_height (corner));

  /* A drag reports travel since the press, then released. */
  reset ();
  released = 0;
  g_signal_connect (trim, "moved", G_CALLBACK (on_two_doubles), NULL);
  g_signal_connect (trim, "released", G_CALLBACK (on_released), NULL);
  GtkGesture *g = drag_gesture (trim);
  g_signal_emit_by_name (g, "drag-begin", 4.0, 10.0);
  g_assert_true (omarchy_trim_handle_get_dragging (OMARCHY_TRIM_HANDLE (trim)));
  g_assert_true (gtk_widget_has_focus (trim));
  g_signal_emit_by_name (g, "drag-update", 12.0, 3.0);
  g_assert_cmpfloat (last_a, ==, 12);
  g_assert_cmpfloat (last_b, ==, 3);
  g_signal_emit_by_name (g, "drag-update", -7.0, 0.0);
  g_assert_cmpfloat (last_a, ==, -7);
  g_signal_emit_by_name (g, "drag-end", -7.0, 0.0);
  g_assert_false (omarchy_trim_handle_get_dragging (OMARCHY_TRIM_HANDLE (trim)));
  g_assert_cmpint (fired, ==, 2);
  g_assert_cmpint (released, ==, 1);

  /* Arrows nudge 1px (Shift 10px) along the axes the handle moves. */
  reset ();
  g_signal_connect (trim, "nudged", G_CALLBACK (on_two_doubles), NULL);
  g_signal_connect (top, "nudged", G_CALLBACK (on_two_doubles), NULL);
  g_signal_connect (side, "nudged", G_CALLBACK (on_two_doubles), NULL);
  g_signal_connect (corner, "nudged", G_CALLBACK (on_two_doubles), NULL);
  g_assert_true (drive_key (window, trim, GDK_KEY_Right, 0));
  g_assert_cmpfloat (last_a, ==, 1);
  g_assert_cmpfloat (last_b, ==, 0);
  drive_key (window, trim, GDK_KEY_Left, GDK_SHIFT_MASK);
  g_assert_cmpfloat (last_a, ==, -10);
  g_assert_false (drive_key (window, trim, GDK_KEY_Up, 0)); /* a trim cap only moves across */
  g_assert_false (drive_key (window, top, GDK_KEY_Left, 0)); /* a top bar only moves down/up */
  g_assert_true (drive_key (window, top, GDK_KEY_Up, 0));
  g_assert_cmpfloat (last_a, ==, 0);
  g_assert_cmpfloat (last_b, ==, -1);
  g_assert_false (drive_key (window, side, GDK_KEY_Down, 0));
  g_assert_true (drive_key (window, side, GDK_KEY_Left, 0));
  g_assert_cmpfloat (last_a, ==, -1);
  g_assert_true (drive_key (window, corner, GDK_KEY_Down, GDK_SHIFT_MASK));
  g_assert_cmpfloat (last_b, ==, 10);
  g_assert_true (drive_key (window, corner, GDK_KEY_Right, 0));
  g_assert_cmpfloat (last_a, ==, 1);
  g_assert_cmpint (fired, ==, 6);

  /* hasCursor and crop are properties. */
  omarchy_trim_handle_set_cursor (OMARCHY_TRIM_HANDLE (trim), TRUE);
  g_assert_true (omarchy_trim_handle_get_cursor (OMARCHY_TRIM_HANDLE (trim)));
  g_assert_true (omarchy_trim_handle_get_crop (OMARCHY_TRIM_HANDLE (top)));
  drive_close (window);
}

/* ================================================================= CropFrame */

static void
on_edge (GtkWidget *w, const char *edge, gpointer d)
{
  fired++;
  g_free (last_edge);
  last_edge = g_strdup (edge);
}

static void
on_edge_move (GtkWidget *w, const char *edge, double dx, double dy, gpointer d)
{
  on_edge (w, edge, d);
  last_a = dx;
  last_b = dy;
}

TEST (test_crop_frame_reports_handles, "/editing/crop-frame")
{
  GtkWidget *f = omarchy_crop_frame_new ();
  gtk_widget_set_halign (f, GTK_ALIGN_START);
  gtk_widget_set_valign (f, GTK_ALIGN_START);
  GtkWidget *window = drive_window (f, 400, 300);
  OmarchyCropFrame *frame = OMARCHY_CROP_FRAME (f);
  OmarchyTheme *theme = omarchy_theme_get_default ();
  g_assert_cmpint (gtk_widget_get_width (f), ==, omarchy_theme_space (theme, 240));
  g_assert_cmpint (gtk_widget_get_height (f), ==, omarchy_theme_space (theme, 140));

  /* Handles sit inside the edges; side bars centered on their side. */
  GtkWidget *br = GTK_WIDGET (omarchy_crop_frame_get_handle (frame, "bottom-right"));
  GtkWidget *left = GTK_WIDGET (omarchy_crop_frame_get_handle (frame, "left"));
  graphene_rect_t r;
  g_assert_true (gtk_widget_compute_bounds (br, f, &r));
  g_assert_cmpfloat (r.origin.x + r.size.width, ==, gtk_widget_get_width (f));
  g_assert_cmpfloat (r.origin.y + r.size.height, ==, gtk_widget_get_height (f));
  g_assert_true (gtk_widget_compute_bounds (left, f, &r));
  g_assert_cmpfloat (r.origin.x, ==, 0);
  g_assert_cmpfloat (fabs (r.origin.y + r.size.height / 2 - gtk_widget_get_height (f) / 2.0), <=, 1);

  reset ();
  g_signal_connect (f, "pressed", G_CALLBACK (on_edge), NULL);
  g_signal_connect (f, "released", G_CALLBACK (on_edge), NULL);
  g_signal_connect (f, "moved", G_CALLBACK (on_edge_move), NULL);
  g_signal_connect (f, "nudged", G_CALLBACK (on_edge_move), NULL);
  GtkGesture *g = drag_gesture (br);
  g_signal_emit_by_name (g, "drag-begin", 5.0, 5.0);
  g_assert_cmpstr (last_edge, ==, "bottom-right");
  g_signal_emit_by_name (g, "drag-update", -20.0, -10.0);
  g_assert_cmpfloat (last_a, ==, -20);
  g_assert_cmpfloat (last_b, ==, -10);
  g_signal_emit_by_name (g, "drag-end", -20.0, -10.0);
  g_assert_cmpint (fired, ==, 3);

  /* A click (press and release) on a handle is pressed + released for its edge. */
  reset ();
  press (left, 1, 5);
  g_assert_cmpint (fired, ==, 2);
  g_assert_cmpstr (last_edge, ==, "left");

  /* Keys on a focused handle nudge it, with its edge. */
  reset ();
  g_assert_true (drive_key (window, left, GDK_KEY_Right, GDK_SHIFT_MASK));
  g_assert_cmpstr (last_edge, ==, "left");
  g_assert_cmpfloat (last_a, ==, 10);
  g_assert_cmpfloat (last_b, ==, 0);

  /* cursorEdge draws one handle hot. */
  omarchy_crop_frame_set_cursor_edge (frame, "top");
  g_assert_true (omarchy_trim_handle_get_cursor (omarchy_crop_frame_get_handle (frame, "top")));
  g_assert_false (omarchy_trim_handle_get_cursor (omarchy_crop_frame_get_handle (frame, "left")));
  omarchy_crop_frame_set_cursor_edge (frame, NULL);
  g_assert_false (omarchy_trim_handle_get_cursor (omarchy_crop_frame_get_handle (frame, "top")));
  drive_close (window);
}

/* ======================================================= DragHandle, DropIndicator */

static int pressed_n, moved_n, released_n, move_delta;
static void on_grip_pressed (GtkWidget *w, gpointer d) { pressed_n++; }
static void on_grip_moved (GtkWidget *w, double dx, double dy, gpointer d) { moved_n++; last_a = dx; last_b = dy; }
static void on_grip_released (GtkWidget *w, gpointer d) { released_n++; }
static void on_move_requested (GtkWidget *w, int delta, gpointer d) { move_delta += delta; fired++; }

TEST (test_drag_handle_drag_and_keys, "/editing/drag-handle")
{
  GtkWidget *grip = omarchy_drag_handle_new ();
  gtk_widget_set_halign (grip, GTK_ALIGN_START);
  gtk_widget_set_valign (grip, GTK_ALIGN_START);
  GtkWidget *window = drive_window (grip, 100, 100);
  OmarchyTheme *theme = omarchy_theme_get_default ();
  g_assert_cmpint (gtk_widget_get_width (grip), ==, omarchy_theme_space (theme, 16));
  g_assert_cmpint (gtk_widget_get_height (grip), ==, omarchy_theme_space (theme, 22));
  reset ();
  pressed_n = moved_n = released_n = move_delta = 0;
  g_signal_connect (grip, "pressed", G_CALLBACK (on_grip_pressed), NULL);
  g_signal_connect (grip, "moved", G_CALLBACK (on_grip_moved), NULL);
  g_signal_connect (grip, "released", G_CALLBACK (on_grip_released), NULL);
  g_signal_connect (grip, "move-requested", G_CALLBACK (on_move_requested), NULL);

  GtkGesture *g = drag_gesture (grip);
  g_signal_emit_by_name (g, "drag-begin", 8.0, 11.0);
  g_assert_true (omarchy_drag_handle_get_dragging (OMARCHY_DRAG_HANDLE (grip)));
  g_signal_emit_by_name (g, "drag-update", 0.0, 37.0);
  g_signal_emit_by_name (g, "drag-end", 0.0, 37.0);
  g_assert_false (omarchy_drag_handle_get_dragging (OMARCHY_DRAG_HANDLE (grip)));
  g_assert_cmpint (pressed_n, ==, 1);
  g_assert_cmpint (moved_n, ==, 1);
  g_assert_cmpint (released_n, ==, 1);
  g_assert_cmpfloat (last_b, ==, 37);

  /* A click is a press and a release, no move. */
  press (grip, 8, 11);
  g_assert_cmpint (pressed_n, ==, 2);
  g_assert_cmpint (moved_n, ==, 1);
  g_assert_cmpint (released_n, ==, 2);

  gtk_widget_grab_focus (grip);
  g_assert_true (drive_key (window, NULL, GDK_KEY_Up, 0));
  g_assert_cmpint (move_delta, ==, -1);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  drive_key (window, NULL, GDK_KEY_Down, 0);
  g_assert_cmpint (move_delta, ==, 1);
  g_assert_false (drive_key (window, NULL, GDK_KEY_Left, 0));
  g_assert_cmpint (fired, ==, 3);
  drive_close (window);
}

TEST (test_drop_indicator_is_a_line, "/editing/drop-indicator")
{
  GtkWidget *d = omarchy_drop_indicator_new ();
  gtk_widget_set_valign (d, GTK_ALIGN_START);
  GtkWidget *window = drive_window (d, 200, 40);
  g_assert_cmpint (gtk_widget_get_height (d), ==, omarchy_drop_indicator_get_thickness ());
  g_assert_cmpint (gtk_widget_get_width (d), ==, 200);
  g_assert_false (gtk_widget_get_can_target (d));
  g_assert_false (gtk_widget_get_focusable (d));
  drive_close (window);
}

/* ================================================================== Timeline */

static int sel_track, sel_clip;
static void
on_clip_pressed (GtkWidget *w, int track, int clip, gpointer d)
{
  fired++;
  sel_track = track;
  sel_clip = clip;
}

static void
on_trimmed (GtkWidget *w, int track, int clip, const char *edge, double time, gpointer d)
{
  fired++;
  sel_track = track;
  sel_clip = clip;
  g_free (last_edge);
  last_edge = g_strdup (edge);
  last_a = time;
}

static GtkWidget *
make_timeline (void)
{
  GtkWidget *w = omarchy_timeline_new (30);
  OmarchyTimeline *tl = OMARCHY_TIMELINE (w);
  int a = omarchy_timeline_add_track (tl);
  omarchy_timeline_append_clip (tl, a, 0, 7.5, "Intro");
  omarchy_timeline_append_clip (tl, a, 8, 18, "Demo");
  omarchy_timeline_append_clip (tl, a, 18.5, 30, "Outro");
  int b = omarchy_timeline_add_track (tl);
  omarchy_timeline_append_clip (tl, b, 2, 12, "Voiceover");
  omarchy_timeline_append_clip (tl, b, 14, 26, "Music");
  omarchy_timeline_set_position (tl, 12);
  return w;
}

TEST (test_timeline_seek_select_and_keys, "/editing/timeline")
{
  GtkWidget *w = make_timeline ();
  OmarchyTimeline *tl = OMARCHY_TIMELINE (w);
  GtkWidget *window = drive_window (w, 600, 160);
  double pps = 600 / 30.0;
  OmarchyTheme *theme = omarchy_theme_get_default ();
  double ruler = omarchy_theme_space (theme, 24);
  double lane2 = 160 - 10; /* inside the second lane */

  /* A press on the ruler seeks there; dragging scrubs. */
  reset ();
  g_signal_connect (w, "seek", G_CALLBACK (on_double), NULL);
  press (w, 5 * pps, ruler / 3);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 5, 1e-9);
  g_assert_cmpfloat_with_epsilon (last_a, 5, 1e-9);
  drag (w, 5 * pps, ruler / 3, 10 * pps, 0);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 15, 1e-9);
  /* An empty part of a lane seeks too (track 2 is empty 0–2 s). */
  press (w, 1 * pps, lane2);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 1, 1e-9);
  int seeks = fired;

  /* A press on a clip selects it and doesn't seek. */
  reset ();
  sel_track = sel_clip = -1;
  g_signal_connect (w, "clip-pressed", G_CALLBACK (on_clip_pressed), NULL);
  press (w, 20 * pps, lane2);
  g_assert_cmpint (fired, ==, 1);
  g_assert_cmpint (sel_track, ==, 1);
  g_assert_cmpint (sel_clip, ==, 1);
  g_assert_cmpint (omarchy_timeline_get_selected_track (tl), ==, 1);
  g_assert_cmpint (omarchy_timeline_get_selected_clip (tl), ==, 1);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 1, 1e-9);
  (void) seeks;

  /* Keys: ±1 s, Shift a major step, Home/End, clamped. */
  gtk_widget_grab_focus (w);
  omarchy_timeline_set_position (tl, 12);
  g_assert_true (drive_key (window, w, GDK_KEY_Right, 0));
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 13, 1e-9);
  drive_key (window, w, GDK_KEY_Left, 0);
  drive_key (window, w, GDK_KEY_Left, 0);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 11, 1e-9);
  double step = omarchy_timeline_get_tick_step (tl);
  g_assert_cmpfloat (step, >, 1);
  drive_key (window, w, GDK_KEY_Right, GDK_SHIFT_MASK);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_position (tl), 11 + step, 1e-9);
  drive_key (window, w, GDK_KEY_End, 0);
  g_assert_cmpfloat (omarchy_timeline_get_position (tl), ==, 30);
  drive_key (window, w, GDK_KEY_Right, 0);
  g_assert_cmpfloat (omarchy_timeline_get_position (tl), ==, 30);
  drive_key (window, w, GDK_KEY_Home, 0);
  g_assert_cmpfloat (omarchy_timeline_get_position (tl), ==, 0);
  drive_close (window);
}

TEST (test_timeline_trims_and_zooms, "/editing/timeline-trim")
{
  GtkWidget *w = make_timeline ();
  OmarchyTimeline *tl = OMARCHY_TIMELINE (w);
  omarchy_timeline_set_selected (tl, 0, 1);
  GtkWidget *window = drive_window (w, 600, 160);
  double pps = 600 / 30.0;

  /* The selected clip shows its trim handles at its ends. */
  GtkWidget *left = drive_find (w, OMARCHY_TYPE_TRIM_HANDLE, 0);
  GtkWidget *right = drive_find (w, OMARCHY_TYPE_TRIM_HANDLE, 1);
  g_assert_true (gtk_widget_get_child_visible (left));
  graphene_rect_t r;
  g_assert_true (gtk_widget_compute_bounds (left, w, &r));
  g_assert_cmpfloat (r.origin.x, ==, 8 * pps);
  g_assert_true (gtk_widget_compute_bounds (right, w, &r));
  g_assert_cmpfloat (r.origin.x + r.size.width, ==, 18 * pps);

  /* Dragging the left handle 40px trims the start by 2 s. */
  reset ();
  g_signal_connect (w, "trimmed", G_CALLBACK (on_trimmed), NULL);
  drag (left, 2, 10, 40, 5);
  g_assert_cmpint (fired, ==, 1);
  g_assert_cmpstr (last_edge, ==, "left");
  g_assert_cmpint (sel_track, ==, 0);
  g_assert_cmpint (sel_clip, ==, 1);
  g_assert_cmpfloat_with_epsilon (last_a, 10, 1e-9);
  double start, end;
  omarchy_timeline_get_clip (tl, 0, 1, &start, &end, NULL);
  g_assert_cmpfloat_with_epsilon (start, 10, 1e-9);

  /* Arrows nudge the right handle 1pt (Shift 10pt). */
  g_assert_true (drive_key (window, right, GDK_KEY_Right, GDK_SHIFT_MASK));
  g_assert_cmpstr (last_edge, ==, "right");
  g_assert_cmpfloat_with_epsilon (last_a, 18 + 10 / pps, 1e-9);
  drive_key (window, right, GDK_KEY_Left, 0);
  g_assert_cmpfloat_with_epsilon (last_a, 18 + 9 / pps, 1e-9);

  /* A click elsewhere on the clip selects another one and the handles follow. */
  press (w, 25 * pps, 60);
  g_assert_cmpint (omarchy_timeline_get_selected_clip (tl), ==, 2);

  /* The wheel zooms ×1.25 a notch around the pointer, clamped to 1. */
  GListModel *list = gtk_widget_observe_controllers (w);
  GtkEventController *scroll = NULL;
  for (guint i = 0; i < g_list_model_get_n_items (list) && !scroll; i++) {
    g_autoptr (GtkEventController) c = g_list_model_get_item (list, i);
    if (GTK_IS_EVENT_CONTROLLER_SCROLL (c))
      scroll = c;
  }
  g_object_unref (list);
  g_assert_nonnull (scroll);
  reset ();
  g_signal_connect (w, "view-changed", G_CALLBACK (on_two_doubles), NULL);
  gboolean handled = FALSE;
  g_signal_emit_by_name (scroll, "scroll", 0.0, -1.0, &handled);
  g_assert_true (handled);
  g_assert_cmpfloat_with_epsilon (omarchy_timeline_get_zoom (tl), 1.25, 1e-9);
  g_assert_cmpfloat_with_epsilon (last_a, 1.25, 1e-9);
  g_assert_cmpfloat (omarchy_timeline_get_view_start (tl), >=, 0);
  g_assert_cmpfloat (omarchy_timeline_get_view_start (tl), <=, 30 - 30 / 1.25 + 1e-9);
  g_signal_emit_by_name (scroll, "scroll", 0.0, 3.0, &handled);
  g_assert_cmpfloat (omarchy_timeline_get_zoom (tl), ==, 1);
  g_assert_cmpfloat (omarchy_timeline_get_view_start (tl), ==, 0);
  g_assert_cmpint (fired, ==, 2);
  drive_close (window);
}

/* =============================================================== ColorPicker */

static GdkRGBA edited_color;
static void
on_edited (GtkWidget *w, const GdkRGBA *c, gpointer d)
{
  fired++;
  edited_color = *c;
}

static GtkWidget *
part (GtkWidget *picker, int n)
{
  /* The picker's children in order: square, hue strip, alpha strip, field, swatches. */
  GtkWidget *c = gtk_widget_get_first_child (picker);
  for (int i = 0; i < n && c; i++)
    c = gtk_widget_get_next_sibling (c);
  g_assert_nonnull (c);
  return c;
}

TEST (test_color_picker_hex_and_square, "/editing/color-picker")
{
  GtkWidget *w = omarchy_color_picker_new ();
  OmarchyColorPicker *cp = OMARCHY_COLOR_PICKER (w);
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  gtk_widget_set_valign (w, GTK_ALIGN_START);
  gtk_widget_set_size_request (w, 300, 260);
  GtkWidget *window = drive_window (w, 400, 300);

  /* setColor: HSV from hex, the hex back the same; alpha below 1 adds two digits. */
  g_assert_true (omarchy_color_picker_set_color (cp, "#7aa2f7"));
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_hue (cp) * 360, 220.8, 0.05);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_saturation (cp), 0.506, 0.001);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_value (cp), 0.969, 0.001);
  g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, "#7aa2f7");
  g_assert_false (omarchy_color_picker_set_color (cp, "nope"));
  g_assert_true (omarchy_color_picker_set_color (cp, "FF000080"));
  g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, "#ff000080");
  omarchy_color_picker_set_alpha (cp, 1);
  g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, "#ff0000");

  /* The field shows the hex, and a valid hex typed into it applies at once. */
  GtkWidget *field = drive_find (w, OMARCHY_TYPE_TEXT_FIELD, 0);
  g_assert_cmpstr (gtk_editable_get_text (GTK_EDITABLE (field)), ==, "#ff0000");
  reset ();
  g_signal_connect (w, "edited", G_CALLBACK (on_edited), NULL);
  gtk_editable_set_text (GTK_EDITABLE (field), "#00ff");
  g_assert_cmpint (fired, ==, 0);
  gtk_editable_set_text (GTK_EDITABLE (field), " #00ff00 ");
  g_assert_cmpint (fired, ==, 1);
  g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, "#00ff00");
  g_assert_cmpstr (gtk_editable_get_text (GTK_EDITABLE (field)), ==, " #00ff00 "); /* not rewritten under the typing */

  /* A press on the square's top left picks white; a drag to the bottom picks black. */
  GtkWidget *square = part (w, 0);
  reset ();
  press (square, 0, 0);
  g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, "#ffffff");
  g_assert_cmpint (fired, ==, 1);
  g_assert_cmpfloat (edited_color.red, ==, 1);
  g_assert_cmpstr (gtk_editable_get_text (GTK_EDITABLE (field)), ==, "#ffffff");
  drag (square, 0, 0, gtk_widget_get_width (square), gtk_widget_get_height (square) * 2);
  g_assert_cmpfloat (omarchy_color_picker_get_saturation (cp), ==, 1);
  g_assert_cmpfloat (omarchy_color_picker_get_value (cp), ==, 0);
  g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, "#000000");

  /* Arrows move the knob 1% (Shift 10%), clamped. */
  omarchy_color_picker_set_saturation (cp, 0.5);
  omarchy_color_picker_set_value (cp, 0.5);
  reset ();
  g_assert_true (drive_key (window, square, GDK_KEY_Right, 0));
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_saturation (cp), 0.51, 1e-9);
  drive_key (window, square, GDK_KEY_Up, GDK_SHIFT_MASK);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_value (cp), 0.6, 1e-9);
  drive_key (window, square, GDK_KEY_Left, GDK_SHIFT_MASK);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_saturation (cp), 0.41, 1e-9);
  drive_key (window, square, GDK_KEY_Down, 0);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_value (cp), 0.59, 1e-9);
  g_assert_cmpint (fired, ==, 4);
  drive_close (window);
}

TEST (test_color_picker_strips_and_swatches, "/editing/color-picker-strips")
{
  GtkWidget *w = omarchy_color_picker_new ();
  OmarchyColorPicker *cp = OMARCHY_COLOR_PICKER (w);
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  gtk_widget_set_valign (w, GTK_ALIGN_START);
  gtk_widget_set_size_request (w, 300, 260);
  GtkWidget *window = drive_window (w, 400, 300);
  omarchy_color_picker_set_color (cp, "#7aa2f7");
  GtkWidget *hue = part (w, 1), *alpha = part (w, 2);

  /* A press at the top of the hue strip is red; a drag past the bottom stays just under 1. */
  reset ();
  g_signal_connect (w, "edited", G_CALLBACK (on_edited), NULL);
  press (hue, 7, 0);
  g_assert_cmpfloat (omarchy_color_picker_get_hue (cp), ==, 0);
  drag (hue, 7, 0, 0, gtk_widget_get_height (hue) * 2);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_hue (cp), 0.9999, 1e-9);
  /* Up/Down move a focused strip 1% (Shift 10%). */
  omarchy_color_picker_set_hue (cp, 0.5);
  g_assert_true (drive_key (window, hue, GDK_KEY_Down, 0));
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_hue (cp), 0.51, 1e-9);
  drive_key (window, hue, GDK_KEY_Up, GDK_SHIFT_MASK);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_hue (cp), 0.41, 1e-9);
  g_assert_false (drive_key (window, hue, GDK_KEY_Left, 0));

  /* The alpha strip is opaque at the top. */
  drag (alpha, 7, 0, 0, gtk_widget_get_height (alpha) * 2);
  g_assert_cmpfloat (omarchy_color_picker_get_alpha (cp), ==, 0);
  drive_key (window, alpha, GDK_KEY_Up, GDK_SHIFT_MASK);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_alpha (cp), 0.1, 1e-9);
  omarchy_color_picker_set_alpha (cp, 0.5);
  g_assert_cmpuint (strlen (omarchy_color_picker_get_hex (cp)), ==, 9);

  /* Swatches: the theme's palette (here from the theme in use). Pressing one picks it and
   * keeps the alpha; Return on a focused one does too. */
  int n = omarchy_color_picker_get_n_swatches (cp);
  g_assert_cmpint (n, >=, 1);
  GtkWidget *first = part (w, 4);
  reset ();
  drive_click (first, -1, -1);
  g_assert_cmpint (fired, ==, 1);
  g_assert_cmpfloat_with_epsilon (omarchy_color_picker_get_alpha (cp), 0.5, 1e-9);
  g_autofree char *picked = g_strndup (omarchy_color_picker_get_hex (cp), 7);
  if (n > 1) {
    GtkWidget *second = part (w, 5);
    omarchy_color_picker_set_alpha (cp, 1);
    g_assert_true (drive_key (window, second, GDK_KEY_Return, 0));
    g_assert_cmpint (fired, ==, 2);
    g_assert_cmpfloat (omarchy_color_picker_get_alpha (cp), ==, 1);
    drive_key (window, first, GDK_KEY_space, 0);
    g_assert_cmpstr (omarchy_color_picker_get_hex (cp), ==, picked);
  }

  /* A palette with no known names shows no swatches; the default comes back with NULL. */
  const char *none[] = { "no-such-color", NULL };
  omarchy_color_picker_set_palette_names (cp, none);
  g_assert_cmpint (omarchy_color_picker_get_n_swatches (cp), ==, 0);
  omarchy_color_picker_set_palette_names (cp, NULL);
  g_assert_cmpint (omarchy_color_picker_get_n_swatches (cp), ==, n);
  drive_close (window);
}
