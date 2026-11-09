/* omarchy-forms-text.c: PasswordField (PasswordField.qml) and TextArea (TextArea.qml), drawn as
 * the Qt kit draws them around GTK's own GtkText and GtkTextView. */
#include "omarchy-forms-private.h"

#include <math.h>

/* The field chrome every input shares: focus > hover-cursor > normal. */
static void
input_look (GtkWidget *w, gboolean cursor, const GdkRGBA **fill, const OmarchyBorder **border)
{
  const OmarchyTokens *p = omarchy_paint ();
  GtkStateFlags f = gtk_widget_get_state_flags (w);
  gboolean focused = (f & GTK_STATE_FLAG_FOCUS_WITHIN) != 0;
  gboolean hot = cursor || (f & GTK_STATE_FLAG_PRELIGHT);
  *fill = focused ? &p->focus.fill : hot ? &p->hover.fill : &p->normal.fill;
  *border = focused ? &p->focus.border : hot ? &p->hover.border : &p->normal.border;
}

/* Border widths reserved for every state (so a state change never relayouts) plus padding. */
static void
input_insets (float in[4])
{
  const OmarchyTokens *t = omarchy_tokens ();
  const OmarchyBorder *specs[] = { &t->normal.border, &t->hover.border, &t->focus.border };
  float r[4];
  omarchy_border_reserve (r, specs, 3);
  in[0] = r[0] + t->input_padding_y;
  in[2] = r[2] + t->input_padding_y;
  in[1] = r[1] + t->control_padding_x;
  in[3] = r[3] + t->control_padding_x;
}

/* ===================================================================== PasswordField === */

/**
 * OmarchyPasswordField:
 *
 * A text field for secrets: the TextField chrome with a lock glyph before dots (`•`), an
 * optional eye at the end that reveals what's typed (#OmarchyPasswordField:revealable), and an
 * error state (#OmarchyPasswordField:error): polkit's `borderError` border, the lock and the
 * message under the field in `textError`. Setting an error flashes and shakes the field
 * (polkit's shake); [method@Omarchy.PasswordField.shake] replays it. Under reduced motion only
 * the colors change. It is a #GtkEditable through the GtkText inside; Return emits ::accepted.
 */
struct _OmarchyPasswordField {
  GtkWidget parent_instance;
  GtkWidget *text;
  gboolean revealable, revealed, cursor, eye_hot;
  char *error;
  double shake, flash;
  gint64 start;
  guint tick;
};

enum { P_PROP_0, P_PROP_PLACEHOLDER, P_PROP_REVEALABLE, P_PROP_REVEALED, P_PROP_ERROR, P_PROP_CURSOR, P_N_PROPS };
static GParamSpec *p_props[P_N_PROPS];
enum { P_ACCEPTED, P_N_SIGNALS };
static guint p_signals[P_N_SIGNALS];

static GtkEditable *
password_delegate (GtkEditable *e)
{
  return GTK_EDITABLE (OMARCHY_PASSWORD_FIELD (e)->text);
}

static void
password_editable_init (GtkEditableInterface *iface)
{
  iface->get_delegate = password_delegate;
}

G_DEFINE_FINAL_TYPE_WITH_CODE (OmarchyPasswordField, omarchy_password_field, GTK_TYPE_WIDGET,
                               G_IMPLEMENT_INTERFACE (GTK_TYPE_EDITABLE, password_editable_init))

static const char LOCK[] = "\xef\x80\xa3";          /* U+F023 */
static const char EYE[] = "\xf3\xb0\x88\x88";       /* U+F0208 */
static const char EYE_OFF[] = "\xf3\xb0\x88\x89";   /* U+F0209 */

typedef struct {
  int field_h, lock_w, lock_off, eye_w, eye_off, line;
} PasswordMetrics;

/* A Nerd Font glyph's width as Qt's Text reports it: its advance, or its ink when the ink runs
 * past it (the lock and the eye are wider than a cell). Returns the x to draw at. */
static int
glyph_width (PangoLayout *l, int *width)
{
  PangoRectangle ink, logical;
  pango_layout_get_extents (l, &ink, &logical);
  double left = MIN (0, ink.x) / (double) PANGO_SCALE;
  double right = MAX (logical.width, ink.x + ink.width) / (double) PANGO_SCALE;
  *width = (int) ceil (right - left);
  return (int) ceil (-left);
}

static PasswordMetrics
password_metrics (OmarchyPasswordField *self)
{
  const OmarchyTokens *t = omarchy_tokens ();
  PasswordMetrics m;
  float in[4];
  input_insets (in);
  m.line = omarchy_line_height (t->body);
  m.field_h = (int) (in[0] + in[2]) + m.line;
  g_autoptr (PangoLayout) lock = omarchy_layout (GTK_WIDGET (self), LOCK, t->body, FALSE);
  m.lock_off = glyph_width (lock, &m.lock_w);
  g_autoptr (PangoLayout) eye = omarchy_layout (GTK_WIDGET (self), self->revealed ? EYE_OFF : EYE, t->body, FALSE);
  m.eye_off = glyph_width (eye, &m.eye_w);
  return m;
}

static PangoLayout *
password_message (OmarchyPasswordField *self, int width)
{
  PangoLayout *l = omarchy_layout (GTK_WIDGET (self), self->error, omarchy_tokens ()->caption, FALSE);
  pango_layout_set_width (l, MAX (1, width) * PANGO_SCALE);
  pango_layout_set_wrap (l, PANGO_WRAP_WORD_CHAR);
  return l;
}

static gboolean
password_has_error (OmarchyPasswordField *self)
{
  return self->error && *self->error;
}

static void
password_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (w);
  const OmarchyTokens *t = omarchy_tokens ();
  PasswordMetrics m = password_metrics (self);
  float in[4];
  input_insets (in);
  if (o == GTK_ORIENTATION_HORIZONTAL) {
    *min = (int) (in[1] + in[3]) + m.lock_w + t->control_gap + omarchy_space (40)
           + (self->revealable ? m.eye_w + t->control_gap : 0);
    *nat = MAX (*min, omarchy_space (240));
    return;
  }
  int h = m.field_h;
  if (password_has_error (self)) {
    g_autoptr (PangoLayout) l = password_message (self, for_size >= 0 ? for_size : omarchy_space (240));
    h += t->label_gap + omarchy_forms_text_height (l, t->caption);
  }
  *min = *nat = h;
}

static GtkSizeRequestMode
password_request_mode (GtkWidget *w)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
password_allocate (GtkWidget *w, int width, int height, int baseline)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (w);
  const OmarchyTokens *t = omarchy_tokens ();
  PasswordMetrics m = password_metrics (self);
  float in[4];
  input_insets (in);
  gboolean rtl = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL;
  int lead = (int) in[3] + m.lock_w + t->control_gap;
  int trail = (int) in[1] + (self->revealable ? m.eye_w + t->control_gap : 0);
  int ch;
  gtk_widget_measure (self->text, GTK_ORIENTATION_VERTICAL, -1, NULL, &ch, NULL, NULL);
  int inner = m.field_h - (int) (in[0] + in[2]);
  GtkAllocation a = { rtl ? trail : lead, (int) in[0] + (inner - ch) / 2, MAX (0, width - lead - trail), ch };
  gtk_widget_size_allocate (self->text, &a, -1);
}

/* The eye's hit box: the glyph and `xs` around it. */
static graphene_rect_t
password_eye_rect (OmarchyPasswordField *self, const PasswordMetrics *m)
{
  const OmarchyTokens *t = omarchy_tokens ();
  float in[4];
  input_insets (in);
  float W = gtk_widget_get_width (GTK_WIDGET (self));
  gboolean rtl = gtk_widget_get_direction (GTK_WIDGET (self)) == GTK_TEXT_DIR_RTL;
  float x = rtl ? in[3] : W - in[1] - m->eye_w;
  float y = roundf ((m->field_h - m->line) / 2.0f);
  return GRAPHENE_RECT_INIT (x - t->xs, y - t->xs, m->eye_w + 2 * t->xs, m->line + 2 * t->xs);
}

static void
password_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (w);
  const OmarchyTokens *p = omarchy_paint ();
  PasswordMetrics m = password_metrics (self);
  float W = gtk_widget_get_width (w);
  gboolean rtl = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL;
  gboolean error = password_has_error (self);
  const GdkRGBA *fill;
  const OmarchyBorder *border;
  input_look (w, self->cursor, &fill, &border);

  gtk_snapshot_save (s);
  gtk_snapshot_translate (s, &GRAPHENE_POINT_INIT ((float) self->shake, 0));
  graphene_rect_t field = GRAPHENE_RECT_INIT (0, 0, W, m.field_h);
  if (error) {
    OmarchyBorder eb = omarchy_border_flat (&p->polkit_border_error.color, MAX (1, border->widths[0]));
    omarchy_draw_surface (s, &field, p->radius, fill, &eb);
  } else {
    omarchy_draw_surface (s, &field, p->radius, fill, border);
  }
  if (self->flash > 0) {
    GdkRGBA flash = omarchy_rgba_alpha (&p->polkit_text_error, p->selected.fill.alpha * self->flash);
    omarchy_draw_fill (s, &field, p->radius, &flash);
  }

  float y = roundf ((m.field_h - m.line) / 2.0f);
  float lock_x = rtl ? W - p->control_padding_x - border->widths[1] - m.lock_w : p->control_padding_x + border->widths[3];
  g_autoptr (PangoLayout) lock = omarchy_layout (w, LOCK, p->body, FALSE);
  omarchy_draw_layout (s, lock, lock_x + m.lock_off, y, error ? &p->polkit_text_error : &p->text_dim);
  if (self->revealable) {
    float eye_x = rtl ? p->control_padding_x + border->widths[3] : W - p->control_padding_x - border->widths[1] - m.eye_w;
    g_autoptr (PangoLayout) eye = omarchy_layout (w, self->revealed ? EYE_OFF : EYE, p->body, FALSE);
    omarchy_draw_layout (s, eye, eye_x + m.eye_off, y, self->eye_hot ? &p->foreground : &p->text_dim);
  }
  gtk_widget_snapshot_child (w, self->text, s);
  gtk_snapshot_restore (s);

  if (error) {
    g_autoptr (PangoLayout) msg = password_message (self, (int) W);
    if (rtl)
      pango_layout_set_alignment (msg, PANGO_ALIGN_RIGHT);
    omarchy_draw_layout (s, msg, 0, m.field_h + p->label_gap, &p->polkit_text_error);
  }
}

static gboolean
password_grab_focus (GtkWidget *w)
{
  return gtk_widget_grab_focus (OMARCHY_PASSWORD_FIELD (w)->text);
}

static void
password_text_activate (GtkText *text, OmarchyPasswordField *self)
{
  g_signal_emit (self, p_signals[P_ACCEPTED], 0);
}

static gboolean
password_on_eye (OmarchyPasswordField *self, double x, double y)
{
  if (!self->revealable)
    return FALSE;
  PasswordMetrics m = password_metrics (self);
  graphene_rect_t r = password_eye_rect (self, &m);
  return graphene_rect_contains_point (&r, &GRAPHENE_POINT_INIT (x, y));
}

static void
password_pressed (GtkGestureClick *g, int n, double x, double y, OmarchyPasswordField *self)
{
  if (password_on_eye (self, x, y)) {
    gtk_gesture_set_state (GTK_GESTURE (g), GTK_EVENT_SEQUENCE_CLAIMED);
    return;
  }
  gtk_widget_grab_focus (self->text);
}

static void
password_released (GtkGestureClick *g, int n, double x, double y, OmarchyPasswordField *self)
{
  if (password_on_eye (self, x, y))
    omarchy_password_field_set_revealed (self, !self->revealed);
}

static void
password_set_eye_hot (OmarchyPasswordField *self, gboolean hot)
{
  if (self->eye_hot == hot)
    return;
  self->eye_hot = hot;
  gtk_widget_set_cursor_from_name (GTK_WIDGET (self), hot ? "pointer" : "text");
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
password_motion (GtkEventControllerMotion *c, double x, double y, OmarchyPasswordField *self)
{
  password_set_eye_hot (self, password_on_eye (self, x, y));
}

static void
password_leave (GtkEventControllerMotion *c, OmarchyPasswordField *self)
{
  password_set_eye_hot (self, FALSE);
}

static gboolean
password_tick (GtkWidget *w, GdkFrameClock *clock, gpointer data)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (w);
  double ms = (gdk_frame_clock_get_frame_time (clock) - self->start) / 1000.0;
  double a = omarchy_space (8);
  /* -a over 35 ms, +a over 50, 0 over 55 (polkit's shake). */
  if (ms < 35)
    self->shake = -a * omarchy_forms_out_quad (ms / 35);
  else if (ms < 85)
    self->shake = -a + 2 * a * omarchy_forms_in_out_quad ((ms - 35) / 50);
  else if (ms < 140)
    self->shake = a - a * omarchy_forms_out_quad ((ms - 85) / 55);
  else
    self->shake = 0;
  self->flash = ms < 400 ? 1 - omarchy_forms_out_quad (ms / 400) : 0;
  gtk_widget_queue_draw (w);
  if (ms >= 400) {
    self->tick = 0;
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

/**
 * omarchy_password_field_shake:
 * @self: the field
 *
 * Replay the error flash and shake (for a repeated failure). Nothing moves under reduced motion.
 */
void
omarchy_password_field_shake (OmarchyPasswordField *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  if (omarchy_tokens ()->reduce_motion || !gtk_widget_get_mapped (w)) {
    self->shake = self->flash = 0;
    gtk_widget_queue_draw (w);
    return;
  }
  self->start = g_get_monotonic_time ();
  self->flash = 1;
  if (!self->tick)
    self->tick = gtk_widget_add_tick_callback (w, password_tick, NULL, NULL);
}

static void
omarchy_password_field_init (OmarchyPasswordField *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  self->error = g_strdup ("");
  self->text = gtk_text_new ();
  gtk_text_set_visibility (GTK_TEXT (self->text), FALSE);
  gtk_text_set_invisible_char (GTK_TEXT (self->text), 0x2022);
  gtk_text_set_input_purpose (GTK_TEXT (self->text), GTK_INPUT_PURPOSE_PASSWORD);
  gtk_widget_set_parent (self->text, w);
  gtk_editable_init_delegate (GTK_EDITABLE (self));
  g_signal_connect (self->text, "activate", G_CALLBACK (password_text_activate), self);
  GtkGesture *g = gtk_gesture_click_new ();
  g_signal_connect (g, "pressed", G_CALLBACK (password_pressed), self);
  g_signal_connect (g, "released", G_CALLBACK (password_released), self);
  gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (g), GTK_PHASE_CAPTURE);
  gtk_widget_add_controller (w, GTK_EVENT_CONTROLLER (g));
  GtkEventController *motion = gtk_event_controller_motion_new ();
  g_signal_connect (motion, "motion", G_CALLBACK (password_motion), self);
  g_signal_connect (motion, "leave", G_CALLBACK (password_leave), self);
  gtk_widget_add_controller (w, motion);
  gtk_widget_set_cursor_from_name (w, "text");
  omarchy_widget_follow (w);
}

static void
password_dispose (GObject *o)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (o);
  if (self->text)
    gtk_editable_finish_delegate (GTK_EDITABLE (self));
  g_clear_pointer (&self->text, gtk_widget_unparent);
  G_OBJECT_CLASS (omarchy_password_field_parent_class)->dispose (o);
}

static void
password_finalize (GObject *o)
{
  g_free (OMARCHY_PASSWORD_FIELD (o)->error);
  G_OBJECT_CLASS (omarchy_password_field_parent_class)->finalize (o);
}

static void
password_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (o);
  if (gtk_editable_delegate_set_property (o, id, v, pspec))
    return;
  switch (id) {
  case P_PROP_PLACEHOLDER: omarchy_password_field_set_placeholder_text (self, g_value_get_string (v)); break;
  case P_PROP_REVEALABLE: omarchy_password_field_set_revealable (self, g_value_get_boolean (v)); break;
  case P_PROP_REVEALED: omarchy_password_field_set_revealed (self, g_value_get_boolean (v)); break;
  case P_PROP_ERROR: omarchy_password_field_set_error (self, g_value_get_string (v)); break;
  case P_PROP_CURSOR: omarchy_password_field_set_cursor (self, g_value_get_boolean (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
password_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyPasswordField *self = OMARCHY_PASSWORD_FIELD (o);
  if (gtk_editable_delegate_get_property (o, id, v, pspec))
    return;
  switch (id) {
  case P_PROP_PLACEHOLDER: g_value_set_string (v, omarchy_password_field_get_placeholder_text (self)); break;
  case P_PROP_REVEALABLE: g_value_set_boolean (v, self->revealable); break;
  case P_PROP_REVEALED: g_value_set_boolean (v, self->revealed); break;
  case P_PROP_ERROR: g_value_set_string (v, self->error); break;
  case P_PROP_CURSOR: g_value_set_boolean (v, self->cursor); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_password_field_class_init (OmarchyPasswordFieldClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->dispose = password_dispose;
  oc->finalize = password_finalize;
  oc->set_property = password_set_property;
  oc->get_property = password_get_property;
  wc->measure = password_measure;
  wc->get_request_mode = password_request_mode;
  wc->size_allocate = password_allocate;
  wc->snapshot = password_snapshot;
  wc->grab_focus = password_grab_focus;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  p_props[P_PROP_PLACEHOLDER] = g_param_spec_string ("placeholder-text", NULL, NULL, "", f);
  /**
   * OmarchyPasswordField:revealable:
   *
   * Show the eye at the end, which flips #OmarchyPasswordField:revealed.
   */
  p_props[P_PROP_REVEALABLE] = g_param_spec_boolean ("revealable", NULL, NULL, FALSE, f);
  /**
   * OmarchyPasswordField:revealed:
   *
   * Show the text instead of dots.
   */
  p_props[P_PROP_REVEALED] = g_param_spec_boolean ("revealed", NULL, NULL, FALSE, f);
  /**
   * OmarchyPasswordField:error:
   *
   * A message under the field; non-empty is the error state (and shakes the field).
   */
  p_props[P_PROP_ERROR] = g_param_spec_string ("error", NULL, NULL, "", f);
  /**
   * OmarchyPasswordField:cursor:
   *
   * Force the hover-cursor look, as a keyboard-driven panel's cursor does.
   */
  p_props[P_PROP_CURSOR] = g_param_spec_boolean ("cursor", NULL, NULL, FALSE, f);
  g_object_class_install_properties (oc, P_N_PROPS, p_props);
  gtk_editable_install_properties (oc, P_N_PROPS);
  /**
   * OmarchyPasswordField::accepted:
   *
   * Return in the field.
   */
  p_signals[P_ACCEPTED] = g_signal_new ("accepted", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                        G_TYPE_NONE, 0);
  gtk_widget_class_set_css_name (wc, "omarchy-password-field");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_TEXT_BOX);
}

/**
 * omarchy_password_field_new:
 *
 * Returns: (transfer floating): an empty password field
 */
GtkWidget *
omarchy_password_field_new (void)
{
  return g_object_new (OMARCHY_TYPE_PASSWORD_FIELD, NULL);
}

const char *
omarchy_password_field_get_placeholder_text (OmarchyPasswordField *self)
{
  const char *p = gtk_text_get_placeholder_text (GTK_TEXT (self->text));
  return p ? p : "";
}

void
omarchy_password_field_set_placeholder_text (OmarchyPasswordField *self, const char *text)
{
  gtk_text_set_placeholder_text (GTK_TEXT (self->text), text);
  g_object_notify_by_pspec (G_OBJECT (self), p_props[P_PROP_PLACEHOLDER]);
}

OMARCHY_BOOL_ACCESSORS (OmarchyPasswordField, password_field, revealable, p_props, P_PROP_REVEALABLE, TRUE)
OMARCHY_BOOL_ACCESSORS (OmarchyPasswordField, password_field, cursor, p_props, P_PROP_CURSOR, FALSE)

gboolean
omarchy_password_field_get_revealed (OmarchyPasswordField *self)
{
  return self->revealed;
}

void
omarchy_password_field_set_revealed (OmarchyPasswordField *self, gboolean revealed)
{
  if (self->revealed == !!revealed)
    return;
  self->revealed = !!revealed;
  gtk_text_set_visibility (GTK_TEXT (self->text), self->revealed);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), p_props[P_PROP_REVEALED]);
}

const char *
omarchy_password_field_get_error (OmarchyPasswordField *self)
{
  return self->error;
}

void
omarchy_password_field_set_error (OmarchyPasswordField *self, const char *error)
{
  if (!g_strcmp0 (self->error, error))
    return;
  g_free (self->error);
  self->error = g_strdup (error ? error : "");
  gtk_widget_queue_resize (GTK_WIDGET (self));
  if (*self->error)
    omarchy_password_field_shake (self);
  g_object_notify_by_pspec (G_OBJECT (self), p_props[P_PROP_ERROR]);
}

/**
 * omarchy_password_field_get_text_widget:
 * @self: the field
 *
 * Returns: (transfer none): the GtkText inside, for what the editable API doesn't cover
 */
GtkText *
omarchy_password_field_get_text_widget (OmarchyPasswordField *self)
{
  return GTK_TEXT (self->text);
}

/* ========================================================================== TextArea === */

/**
 * OmarchyTextArea:
 *
 * A multi-line TextField: the same chrome, padding (`controlPaddingX` × `inputPaddingY`), type
 * and `textFaint` placeholder, wrapping its text. It is #OmarchyTextArea:min-rows lines tall at
 * least and grows with its text up to #OmarchyTextArea:max-rows, then scrolls. The text lives
 * in a #GtkTextBuffer ([method@Omarchy.TextArea.get_buffer]).
 */
struct _OmarchyTextArea {
  GtkWidget parent_instance;
  GtkWidget *scroller, *view;
  char *placeholder;
  int min_rows, max_rows;
  gboolean cursor;
};

enum { A_PROP_0, A_PROP_TEXT, A_PROP_PLACEHOLDER, A_PROP_MIN_ROWS, A_PROP_MAX_ROWS, A_PROP_CURSOR, A_N_PROPS };
static GParamSpec *a_props[A_N_PROPS];

G_DEFINE_FINAL_TYPE (OmarchyTextArea, omarchy_text_area, GTK_TYPE_WIDGET)

static int
area_line_count (OmarchyTextArea *self, int width)
{
  float in[4];
  input_insets (in);
  g_autofree char *text = omarchy_text_area_get_text (self);
  g_autoptr (PangoLayout) l = omarchy_layout (GTK_WIDGET (self), text, omarchy_tokens ()->body, FALSE);
  pango_layout_set_width (l, MAX (1, width - (int) (in[1] + in[3])) * PANGO_SCALE);
  pango_layout_set_wrap (l, PANGO_WRAP_WORD_CHAR);
  return MAX (1, pango_layout_get_line_count (l));
}

static void
area_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchyTextArea *self = OMARCHY_TEXT_AREA (w);
  float in[4];
  input_insets (in);
  if (o == GTK_ORIENTATION_HORIZONTAL) {
    *min = (int) (in[1] + in[3]) + omarchy_space (40);
    *nat = MAX (*min, omarchy_space (240));
    return;
  }
  int width = for_size >= 0 ? for_size : omarchy_space (240);
  int rows = CLAMP (area_line_count (self, width), self->min_rows, MAX (self->min_rows, self->max_rows));
  *min = *nat = rows * omarchy_line_height (omarchy_tokens ()->body) + (int) (in[0] + in[2]);
}

static GtkSizeRequestMode
area_request_mode (GtkWidget *w)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
area_allocate (GtkWidget *w, int width, int height, int baseline)
{
  OmarchyTextArea *self = OMARCHY_TEXT_AREA (w);
  float in[4];
  input_insets (in);
  GtkTextView *v = GTK_TEXT_VIEW (self->view);
  /* The view scrolls under the whole field (as Qt's Flickable fills it); the padding is its margin. */
  gtk_text_view_set_left_margin (v, (int) in[3]);
  gtk_text_view_set_right_margin (v, (int) in[1]);
  gtk_text_view_set_top_margin (v, (int) in[0]);
  gtk_text_view_set_bottom_margin (v, (int) in[2]);
  gtk_widget_size_allocate (self->scroller, &(GtkAllocation) { 0, 0, width, height }, -1);
}

static void
area_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyTextArea *self = OMARCHY_TEXT_AREA (w);
  const OmarchyTokens *p = omarchy_paint ();
  float W = gtk_widget_get_width (w), H = gtk_widget_get_height (w);
  const GdkRGBA *fill;
  const OmarchyBorder *border;
  input_look (w, self->cursor, &fill, &border);
  omarchy_draw_surface (s, &GRAPHENE_RECT_INIT (0, 0, W, H), p->radius, fill, border);
  GtkTextBuffer *b = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view));
  if (gtk_text_buffer_get_char_count (b) == 0 && self->placeholder && *self->placeholder) {
    float in[4];
    input_insets (in);
    g_autoptr (PangoLayout) l = omarchy_layout (w, self->placeholder, p->body, FALSE);
    pango_layout_set_width (l, MAX (1, (int) (W - in[1] - in[3])) * PANGO_SCALE);
    pango_layout_set_wrap (l, PANGO_WRAP_WORD_CHAR);
    if (gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL)
      pango_layout_set_alignment (l, PANGO_ALIGN_RIGHT);
    omarchy_draw_layout (s, l, in[3], in[0], &p->text_faint);
  }
  gtk_widget_snapshot_child (w, self->scroller, s);
}

static gboolean
area_grab_focus (GtkWidget *w)
{
  return gtk_widget_grab_focus (OMARCHY_TEXT_AREA (w)->view);
}

static void
area_changed (GtkTextBuffer *b, OmarchyTextArea *self)
{
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), a_props[A_PROP_TEXT]);
}

static void
omarchy_text_area_init (OmarchyTextArea *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  self->min_rows = 3;
  self->max_rows = 6;
  self->placeholder = g_strdup ("");
  self->view = gtk_text_view_new ();
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (self->view), GTK_WRAP_WORD_CHAR);
  gtk_text_view_set_pixels_above_lines (GTK_TEXT_VIEW (self->view), 0);
  gtk_text_view_set_pixels_below_lines (GTK_TEXT_VIEW (self->view), 0);
  self->scroller = gtk_scrolled_window_new ();
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (self->scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (self->scroller), FALSE);
  gtk_scrolled_window_set_has_frame (GTK_SCROLLED_WINDOW (self->scroller), FALSE);
  gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (self->scroller), self->view);
  gtk_widget_set_parent (self->scroller, w);
  g_signal_connect_object (gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view)), "changed", G_CALLBACK (area_changed),
                           self, 0);
  omarchy_widget_follow (w);
}

static void
area_dispose (GObject *o)
{
  OmarchyTextArea *self = OMARCHY_TEXT_AREA (o);
  g_clear_pointer (&self->scroller, gtk_widget_unparent);
  self->view = NULL;
  G_OBJECT_CLASS (omarchy_text_area_parent_class)->dispose (o);
}

static void
area_finalize (GObject *o)
{
  g_free (OMARCHY_TEXT_AREA (o)->placeholder);
  G_OBJECT_CLASS (omarchy_text_area_parent_class)->finalize (o);
}

static void
area_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyTextArea *self = OMARCHY_TEXT_AREA (o);
  switch (id) {
  case A_PROP_TEXT: omarchy_text_area_set_text (self, g_value_get_string (v)); break;
  case A_PROP_PLACEHOLDER: omarchy_text_area_set_placeholder_text (self, g_value_get_string (v)); break;
  case A_PROP_MIN_ROWS: omarchy_text_area_set_min_rows (self, g_value_get_int (v)); break;
  case A_PROP_MAX_ROWS: omarchy_text_area_set_max_rows (self, g_value_get_int (v)); break;
  case A_PROP_CURSOR: omarchy_text_area_set_cursor (self, g_value_get_boolean (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
area_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyTextArea *self = OMARCHY_TEXT_AREA (o);
  switch (id) {
  case A_PROP_TEXT: g_value_take_string (v, omarchy_text_area_get_text (self)); break;
  case A_PROP_PLACEHOLDER: g_value_set_string (v, self->placeholder); break;
  case A_PROP_MIN_ROWS: g_value_set_int (v, self->min_rows); break;
  case A_PROP_MAX_ROWS: g_value_set_int (v, self->max_rows); break;
  case A_PROP_CURSOR: g_value_set_boolean (v, self->cursor); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_text_area_class_init (OmarchyTextAreaClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->dispose = area_dispose;
  oc->finalize = area_finalize;
  oc->set_property = area_set_property;
  oc->get_property = area_get_property;
  wc->measure = area_measure;
  wc->get_request_mode = area_request_mode;
  wc->size_allocate = area_allocate;
  wc->snapshot = area_snapshot;
  wc->grab_focus = area_grab_focus;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  a_props[A_PROP_TEXT] = g_param_spec_string ("text", NULL, NULL, "", f);
  a_props[A_PROP_PLACEHOLDER] = g_param_spec_string ("placeholder-text", NULL, NULL, "", f);
  /**
   * OmarchyTextArea:min-rows:
   *
   * The fewest lines it is tall.
   */
  a_props[A_PROP_MIN_ROWS] = g_param_spec_int ("min-rows", NULL, NULL, 1, G_MAXINT, 3, f);
  /**
   * OmarchyTextArea:max-rows:
   *
   * The most lines it grows to before it scrolls.
   */
  a_props[A_PROP_MAX_ROWS] = g_param_spec_int ("max-rows", NULL, NULL, 1, G_MAXINT, 6, f);
  /**
   * OmarchyTextArea:cursor:
   *
   * Force the hover-cursor look, as a keyboard-driven panel's cursor does.
   */
  a_props[A_PROP_CURSOR] = g_param_spec_boolean ("cursor", NULL, NULL, FALSE, f);
  g_object_class_install_properties (oc, A_N_PROPS, a_props);
  gtk_widget_class_set_css_name (wc, "omarchy-text-area");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_TEXT_BOX);
}

/**
 * omarchy_text_area_new:
 *
 * Returns: (transfer floating): an empty text area
 */
GtkWidget *
omarchy_text_area_new (void)
{
  return g_object_new (OMARCHY_TYPE_TEXT_AREA, NULL);
}

/**
 * omarchy_text_area_get_text:
 * @self: the area
 *
 * Returns: (transfer full): the whole text
 */
char *
omarchy_text_area_get_text (OmarchyTextArea *self)
{
  GtkTextBuffer *b = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view));
  GtkTextIter start, end;
  gtk_text_buffer_get_bounds (b, &start, &end);
  return gtk_text_buffer_get_text (b, &start, &end, FALSE);
}

void
omarchy_text_area_set_text (OmarchyTextArea *self, const char *text)
{
  gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view)), text ? text : "", -1);
}

const char *
omarchy_text_area_get_placeholder_text (OmarchyTextArea *self)
{
  return self->placeholder;
}

void
omarchy_text_area_set_placeholder_text (OmarchyTextArea *self, const char *text)
{
  if (!g_strcmp0 (self->placeholder, text))
    return;
  g_free (self->placeholder);
  self->placeholder = g_strdup (text ? text : "");
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), a_props[A_PROP_PLACEHOLDER]);
}

int
omarchy_text_area_get_min_rows (OmarchyTextArea *self)
{
  return self->min_rows;
}

void
omarchy_text_area_set_min_rows (OmarchyTextArea *self, int rows)
{
  rows = MAX (1, rows);
  if (self->min_rows == rows)
    return;
  self->min_rows = rows;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), a_props[A_PROP_MIN_ROWS]);
}

int
omarchy_text_area_get_max_rows (OmarchyTextArea *self)
{
  return self->max_rows;
}

void
omarchy_text_area_set_max_rows (OmarchyTextArea *self, int rows)
{
  rows = MAX (1, rows);
  if (self->max_rows == rows)
    return;
  self->max_rows = rows;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), a_props[A_PROP_MAX_ROWS]);
}

OMARCHY_BOOL_ACCESSORS (OmarchyTextArea, text_area, cursor, a_props, A_PROP_CURSOR, FALSE)

/**
 * omarchy_text_area_get_buffer:
 * @self: the area
 *
 * Returns: (transfer none): the text's buffer
 */
GtkTextBuffer *
omarchy_text_area_get_buffer (OmarchyTextArea *self)
{
  return gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->view));
}

/**
 * omarchy_text_area_get_text_view:
 * @self: the area
 *
 * Returns: (transfer none): the GtkTextView inside
 */
GtkTextView *
omarchy_text_area_get_text_view (OmarchyTextArea *self)
{
  return GTK_TEXT_VIEW (self->view);
}
