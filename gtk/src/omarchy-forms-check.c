/* omarchy-forms-check.c: Checkbox (Checkbox.qml), Radio (Radio.qml) and RadioGroup
 * (RadioGroup.qml), plus the helpers the forms files share.
 *
 * Checkbox and Radio flip #OmarchyCheckbox:checked themselves on a click or Space/Return and then
 * emit ::toggled (GTK's way; Qt's caller flips it). A Radio only ever checks itself. */
#include "omarchy-forms-private.h"

#include <math.h>

/* ============================================================================= helpers === */

void
omarchy_forms_draw_check (GtkWidget *w, GtkSnapshot *s, const graphene_rect_t *box, const GdkRGBA *color)
{
  int px = (int) lround (box->size.height * 0.85);
  g_autoptr (PangoLayout) l = omarchy_layout (w, "\xe2\x9c\x93", px, TRUE);
  PangoRectangle r;
  pango_layout_get_extents (l, NULL, &r);
  float tw = ceilf (r.width / (float) PANGO_SCALE);
  int th = omarchy_line_height (px);
  /* Qt centers the text's box (its advance, ceiled) on a whole pixel, rounding down. */
  omarchy_draw_layout (s, l, box->origin.x + floorf ((box->size.width - tw) / 2.0f),
                       box->origin.y + roundf ((box->size.height - th) / 2.0f), color);
}

int
omarchy_forms_text_height (PangoLayout *l, int px)
{
  if (pango_layout_get_line_count (l) <= 1)
    return omarchy_line_height (px);
  PangoRectangle r;
  pango_layout_get_extents (l, NULL, &r);
  return (int) ceil (r.height / (double) PANGO_SCALE);
}

double
omarchy_forms_out_cubic (double t)
{
  t = CLAMP (t, 0, 1) - 1;
  return t * t * t + 1;
}

double
omarchy_forms_out_quad (double t)
{
  t = CLAMP (t, 0, 1);
  return -t * (t - 2);
}

double
omarchy_forms_in_out_quad (double t)
{
  t = CLAMP (t, 0, 1) * 2;
  if (t < 1)
    return t * t / 2;
  t -= 1;
  return -0.5 * (t * (t - 2) - 1);
}

static void
hover_enter (GtkEventControllerMotion *c, double x, double y, gpointer data)
{
  GtkWidget *w = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (c));
  g_signal_emit (w, GPOINTER_TO_UINT (data), 0, TRUE);
}

static void
hover_leave (GtkEventControllerMotion *c, gpointer data)
{
  GtkWidget *w = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (c));
  g_signal_emit (w, GPOINTER_TO_UINT (data), 0, FALSE);
}

void
omarchy_forms_add_hover_signal (GtkWidget *w, guint signal_id)
{
  GtkEventController *m = gtk_event_controller_motion_new ();
  g_signal_connect (m, "enter", G_CALLBACK (hover_enter), GUINT_TO_POINTER (signal_id));
  g_signal_connect (m, "leave", G_CALLBACK (hover_leave), GUINT_TO_POINTER (signal_id));
  gtk_widget_add_controller (w, m);
}

/* ============================================================================ Checkbox === */

/**
 * OmarchyCheckbox:
 *
 * A check row: a `space(16)` box (`radius_small` corners), then a `body` label and an optional
 * `caption` description in `textDim`, `xxs` apart, `rowGap` from the box. Checked fills the
 * box with the selected state and marks it with a bold accent ✓; the cursor, the pointer and
 * keyboard focus give the box the hover look. Insensitive dims the box to half and the words
 * to `textFaint`. #OmarchyCheckbox:exclusive draws a radio (a circle with an accent dot), as
 * #OmarchyRadio does.
 */
typedef struct {
  char *label, *description;
  gboolean checked, exclusive, cursor;
} OmarchyCheckboxPrivate;

enum { C_PROP_0, C_PROP_LABEL, C_PROP_DESCRIPTION, C_PROP_CHECKED, C_PROP_EXCLUSIVE, C_PROP_CURSOR, C_N_PROPS };
static GParamSpec *c_props[C_N_PROPS];
enum { C_TOGGLED, C_HOVERED, C_N_SIGNALS };
static guint c_signals[C_N_SIGNALS];

G_DEFINE_TYPE_WITH_PRIVATE (OmarchyCheckbox, omarchy_checkbox, GTK_TYPE_WIDGET)

#define PRIV(self) ((OmarchyCheckboxPrivate *) omarchy_checkbox_get_instance_private (OMARCHY_CHECKBOX (self)))

typedef struct {
  int box, label_line, box_y, text_y, words_x;
} CheckMetrics;

static CheckMetrics
check_metrics (void)
{
  const OmarchyTokens *t = omarchy_tokens ();
  CheckMetrics m;
  m.box = omarchy_space (16);
  m.label_line = omarchy_line_height (t->body);
  m.box_y = MAX (0, (int) lround ((m.label_line - m.box) / 2.0));
  m.text_y = MAX (0, (int) lround ((m.box - m.label_line) / 2.0));
  m.words_x = m.box + t->row_gap;
  return m;
}

static PangoLayout *
check_description (OmarchyCheckbox *self, int width)
{
  OmarchyCheckboxPrivate *priv = PRIV (self);
  PangoLayout *l = omarchy_layout (GTK_WIDGET (self), priv->description, omarchy_tokens ()->caption, FALSE);
  if (width >= 0) {
    pango_layout_set_width (l, MAX (1, width) * PANGO_SCALE);
    pango_layout_set_wrap (l, PANGO_WRAP_WORD_CHAR);
  }
  return l;
}

static int
check_words_height (OmarchyCheckbox *self, int words_w)
{
  OmarchyCheckboxPrivate *priv = PRIV (self);
  const OmarchyTokens *t = omarchy_tokens ();
  int h = omarchy_line_height (t->body);
  if (priv->description && *priv->description) {
    g_autoptr (PangoLayout) d = check_description (self, words_w);
    h += t->xxs + omarchy_forms_text_height (d, t->caption);
  }
  return h;
}

static void
check_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  OmarchyCheckbox *self = OMARCHY_CHECKBOX (w);
  OmarchyCheckboxPrivate *priv = PRIV (self);
  const OmarchyTokens *t = omarchy_tokens ();
  CheckMetrics m = check_metrics ();
  if (o == GTK_ORIENTATION_HORIZONTAL) {
    g_autoptr (PangoLayout) l = omarchy_layout (w, priv->label, t->body, FALSE);
    PangoRectangle r;
    pango_layout_get_extents (l, NULL, &r);
    int lw = (int) ceil (r.width / (double) PANGO_SCALE), dw = 0;
    if (priv->description && *priv->description) {
      g_autoptr (PangoLayout) d = check_description (self, -1);
      pango_layout_get_extents (d, NULL, &r);
      dw = (int) ceil (r.width / (double) PANGO_SCALE);
    }
    *min = m.box;
    *nat = m.words_x + MAX (lw, dw);
    return;
  }
  int words_w = for_size >= 0 ? for_size - m.words_x : -1;
  *min = *nat = MAX (m.box_y + m.box, m.text_y + check_words_height (self, words_w));
}

static GtkSizeRequestMode
check_request_mode (GtkWidget *w)
{
  return GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
check_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyCheckbox *self = OMARCHY_CHECKBOX (w);
  OmarchyCheckboxPrivate *priv = PRIV (self);
  const OmarchyTokens *p = omarchy_paint ();
  CheckMetrics m = check_metrics ();
  float W = gtk_widget_get_width (w);
  gboolean enabled = gtk_widget_is_sensitive (w);
  gboolean rtl = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL;
  gboolean hot = enabled && (priv->cursor || omarchy_widget_hovered (w) || omarchy_widget_focus_shown (w));

  graphene_rect_t box = GRAPHENE_RECT_INIT (rtl ? W - m.box : 0, m.box_y, m.box, m.box);
  const GdkRGBA *fill = hot ? &p->hover.fill : priv->checked ? &p->selected.fill : &p->normal.fill;
  const OmarchyBorder *border = hot ? &p->hover.border : priv->checked ? &p->selected.border : &p->normal.border;
  if (!enabled)
    gtk_snapshot_push_opacity (s, 0.5);
  omarchy_draw_surface (s, &box, priv->exclusive ? m.box / 2.0f : omarchy_radius_small (), fill, border);
  if (priv->checked && !priv->exclusive) {
    omarchy_forms_draw_check (w, s, &box, &p->accent);
  } else if (priv->checked) {
    int dot = (int) lround (m.box / 2.0);
    graphene_rect_t r = GRAPHENE_RECT_INIT (box.origin.x + roundf ((m.box - dot) / 2.0f),
                                            box.origin.y + roundf ((m.box - dot) / 2.0f), dot, dot);
    omarchy_draw_fill (s, &r, dot / 2.0f, &p->accent);
  }
  if (!enabled)
    gtk_snapshot_pop (s);

  int words_w = MAX (1, (int) W - m.words_x);
  float x = rtl ? 0 : m.words_x;
  g_autoptr (PangoLayout) l = omarchy_layout (w, priv->label, p->body, FALSE);
  pango_layout_set_width (l, words_w * PANGO_SCALE);
  pango_layout_set_ellipsize (l, PANGO_ELLIPSIZE_END);
  if (rtl)
    pango_layout_set_alignment (l, PANGO_ALIGN_RIGHT);
  omarchy_draw_layout (s, l, x, m.text_y, enabled ? &p->foreground : &p->text_faint);
  if (priv->description && *priv->description) {
    g_autoptr (PangoLayout) d = check_description (self, words_w);
    if (rtl)
      pango_layout_set_alignment (d, PANGO_ALIGN_RIGHT);
    omarchy_draw_layout (s, d, x, m.text_y + m.label_line + p->xxs, enabled ? &p->text_dim : &p->text_faint);
  }
}

static void
check_flip (OmarchyCheckbox *self)
{
  OmarchyCheckboxPrivate *priv = PRIV (self);
  if (!gtk_widget_is_sensitive (GTK_WIDGET (self)))
    return;
  omarchy_checkbox_set_checked (self, priv->exclusive ? TRUE : !priv->checked);
  g_signal_emit (self, c_signals[C_TOGGLED], 0);
}

static void
check_clicked (GtkWidget *w, guint button, double x, double y)
{
  if (button == GDK_BUTTON_PRIMARY)
    check_flip (OMARCHY_CHECKBOX (w));
}

static gboolean
check_key (GtkWidget *w, guint keyval, GdkModifierType state)
{
  if (!omarchy_key_is_activate (keyval))
    return FALSE;
  check_flip (OMARCHY_CHECKBOX (w));
  return TRUE;
}

static void
omarchy_checkbox_init (OmarchyCheckbox *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  OmarchyCheckboxPrivate *priv = PRIV (self);
  priv->label = g_strdup ("");
  priv->description = g_strdup ("");
  gtk_widget_set_focusable (w, TRUE);
  gtk_widget_set_focus_on_click (w, FALSE);
  omarchy_widget_follow (w);
  omarchy_widget_add_click (w, check_clicked);
  omarchy_widget_add_keys (w, check_key);
  omarchy_forms_add_hover_signal (w, c_signals[C_HOVERED]);
}

static void
check_finalize (GObject *o)
{
  OmarchyCheckboxPrivate *priv = PRIV (o);
  g_free (priv->label);
  g_free (priv->description);
  G_OBJECT_CLASS (omarchy_checkbox_parent_class)->finalize (o);
}

static void
check_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyCheckbox *self = OMARCHY_CHECKBOX (o);
  switch (id) {
  case C_PROP_LABEL: omarchy_checkbox_set_label (self, g_value_get_string (v)); break;
  case C_PROP_DESCRIPTION: omarchy_checkbox_set_description (self, g_value_get_string (v)); break;
  case C_PROP_CHECKED: omarchy_checkbox_set_checked (self, g_value_get_boolean (v)); break;
  case C_PROP_EXCLUSIVE: omarchy_checkbox_set_exclusive (self, g_value_get_boolean (v)); break;
  case C_PROP_CURSOR: omarchy_checkbox_set_cursor (self, g_value_get_boolean (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
check_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyCheckboxPrivate *priv = PRIV (o);
  switch (id) {
  case C_PROP_LABEL: g_value_set_string (v, priv->label); break;
  case C_PROP_DESCRIPTION: g_value_set_string (v, priv->description); break;
  case C_PROP_CHECKED: g_value_set_boolean (v, priv->checked); break;
  case C_PROP_EXCLUSIVE: g_value_set_boolean (v, priv->exclusive); break;
  case C_PROP_CURSOR: g_value_set_boolean (v, priv->cursor); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_checkbox_class_init (OmarchyCheckboxClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->finalize = check_finalize;
  oc->set_property = check_set_property;
  oc->get_property = check_get_property;
  wc->measure = check_measure;
  wc->snapshot = check_snapshot;
  wc->get_request_mode = check_request_mode;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  c_props[C_PROP_LABEL] = g_param_spec_string ("label", NULL, NULL, "", f);
  c_props[C_PROP_DESCRIPTION] = g_param_spec_string ("description", NULL, NULL, "", f);
  c_props[C_PROP_CHECKED] = g_param_spec_boolean ("checked", NULL, NULL, FALSE, f);
  /**
   * OmarchyCheckbox:exclusive:
   *
   * Draw a radio: a circle with an accent dot. A click only checks it.
   */
  c_props[C_PROP_EXCLUSIVE] = g_param_spec_boolean ("exclusive", NULL, NULL, FALSE, f);
  /**
   * OmarchyCheckbox:cursor:
   *
   * Force the hover look on the box, as a keyboard-driven panel's cursor does.
   */
  c_props[C_PROP_CURSOR] = g_param_spec_boolean ("cursor", NULL, NULL, FALSE, f);
  g_object_class_install_properties (oc, C_N_PROPS, c_props);
  /**
   * OmarchyCheckbox::toggled:
   *
   * After a click or Space/Return flipped #OmarchyCheckbox:checked (a radio: checked it).
   */
  c_signals[C_TOGGLED] = g_signal_new ("toggled", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                       G_TYPE_NONE, 0);
  /**
   * OmarchyCheckbox::hovered:
   * @self: the checkbox
   * @hovered: the pointer is over it
   */
  c_signals[C_HOVERED] = g_signal_new ("hovered", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                       G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
  gtk_widget_class_set_css_name (wc, "omarchy-checkbox");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_CHECKBOX);
}

/**
 * omarchy_checkbox_new:
 * @label: (nullable): the label
 *
 * Returns: (transfer floating): a checkbox, unchecked
 */
GtkWidget *
omarchy_checkbox_new (const char *label)
{
  return g_object_new (OMARCHY_TYPE_CHECKBOX, "label", label ? label : "", NULL);
}

#define CHECK_STRING(field, PROP)                                                   \
  const char *omarchy_checkbox_get_##field (OmarchyCheckbox *self)                  \
  {                                                                                 \
    return PRIV (self)->field;                                                      \
  }                                                                                 \
  void omarchy_checkbox_set_##field (OmarchyCheckbox *self, const char *v)          \
  {                                                                                 \
    OmarchyCheckboxPrivate *priv = PRIV (self);                                     \
    if (!g_strcmp0 (priv->field, v))                                                \
      return;                                                                       \
    g_free (priv->field);                                                           \
    priv->field = g_strdup (v ? v : "");                                            \
    gtk_widget_queue_resize (GTK_WIDGET (self));                                    \
    g_object_notify_by_pspec (G_OBJECT (self), c_props[PROP]);                      \
  }

#define CHECK_BOOL(field, PROP)                                                     \
  gboolean omarchy_checkbox_get_##field (OmarchyCheckbox *self)                     \
  {                                                                                 \
    return PRIV (self)->field;                                                      \
  }                                                                                 \
  void omarchy_checkbox_set_##field (OmarchyCheckbox *self, gboolean v)             \
  {                                                                                 \
    OmarchyCheckboxPrivate *priv = PRIV (self);                                     \
    if (priv->field == !!v)                                                         \
      return;                                                                       \
    priv->field = !!v;                                                              \
    gtk_widget_queue_draw (GTK_WIDGET (self));                                      \
    g_object_notify_by_pspec (G_OBJECT (self), c_props[PROP]);                      \
  }

CHECK_STRING (label, C_PROP_LABEL)
CHECK_STRING (description, C_PROP_DESCRIPTION)
CHECK_BOOL (cursor, C_PROP_CURSOR)

gboolean
omarchy_checkbox_get_checked (OmarchyCheckbox *self)
{
  return PRIV (self)->checked;
}

void
omarchy_checkbox_set_checked (OmarchyCheckbox *self, gboolean checked)
{
  OmarchyCheckboxPrivate *priv = PRIV (self);
  if (priv->checked == !!checked)
    return;
  priv->checked = !!checked;
  gtk_accessible_update_state (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_STATE_CHECKED,
                               priv->checked ? GTK_ACCESSIBLE_TRISTATE_TRUE : GTK_ACCESSIBLE_TRISTATE_FALSE, -1);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), c_props[C_PROP_CHECKED]);
}

gboolean
omarchy_checkbox_get_exclusive (OmarchyCheckbox *self)
{
  return PRIV (self)->exclusive;
}

void
omarchy_checkbox_set_exclusive (OmarchyCheckbox *self, gboolean exclusive)
{
  OmarchyCheckboxPrivate *priv = PRIV (self);
  if (priv->exclusive == !!exclusive)
    return;
  priv->exclusive = !!exclusive;
  gtk_accessible_update_state (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_STATE_CHECKED,
                               priv->checked ? GTK_ACCESSIBLE_TRISTATE_TRUE : GTK_ACCESSIBLE_TRISTATE_FALSE, -1);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), c_props[C_PROP_EXCLUSIVE]);
}

/* =============================================================================== Radio === */

/**
 * OmarchyRadio:
 *
 * One choice of several: an #OmarchyCheckbox with #OmarchyCheckbox:exclusive, a circle with an
 * accent dot when checked. Group them with #OmarchyRadioGroup, which owns the value and the
 * arrow keys.
 */
struct _OmarchyRadio {
  OmarchyCheckbox parent_instance;
};

G_DEFINE_FINAL_TYPE (OmarchyRadio, omarchy_radio, OMARCHY_TYPE_CHECKBOX)

static void
omarchy_radio_init (OmarchyRadio *self)
{
  PRIV (self)->exclusive = TRUE;
  gtk_accessible_update_state (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_STATE_CHECKED, GTK_ACCESSIBLE_TRISTATE_FALSE, -1);
}

static void
omarchy_radio_class_init (OmarchyRadioClass *klass)
{
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_css_name (wc, "omarchy-radio");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_RADIO);
}

/**
 * omarchy_radio_new:
 * @label: (nullable): the label
 *
 * Returns: (transfer floating): a radio, unchecked
 */
GtkWidget *
omarchy_radio_new (const char *label)
{
  return g_object_new (OMARCHY_TYPE_RADIO, "label", label ? label : "", NULL);
}

/* ========================================================================== RadioGroup === */

/**
 * OmarchyRadioGroup:
 *
 * A column of #OmarchyRadio rows `lg` apart, one checked. Picking one (a click, or Up/Down,
 * Left/Right, j/k while the group has focus) sets #OmarchyRadioGroup:value and emits ::changed.
 * With keyboard focus the checked row wears the cursor; #OmarchyRadioGroup:cursor-index forces
 * it on a row, for panels that drive their own cursor.
 */
struct _OmarchyRadioGroup {
  GtkWidget parent_instance;
  GPtrArray *values;
  char *value;
  int cursor_index;
};

enum { RG_PROP_0, RG_PROP_VALUE, RG_PROP_CURSOR_INDEX, RG_N_PROPS };
static GParamSpec *rg_props[RG_N_PROPS];
enum { RG_CHANGED, RG_N_SIGNALS };
static guint rg_signals[RG_N_SIGNALS];

G_DEFINE_FINAL_TYPE (OmarchyRadioGroup, omarchy_radio_group, GTK_TYPE_WIDGET)

static int
group_index_of (OmarchyRadioGroup *self, const char *value)
{
  for (guint i = 0; i < self->values->len; i++)
    if (!g_strcmp0 (g_ptr_array_index (self->values, i), value))
      return (int) i;
  return -1;
}

static void
group_sync (OmarchyRadioGroup *self)
{
  gboolean focus = omarchy_widget_focus_shown (GTK_WIDGET (self));
  int i = 0;
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self)); c; c = gtk_widget_get_next_sibling (c), i++) {
    gboolean checked = !g_strcmp0 (g_ptr_array_index (self->values, i), self->value);
    omarchy_checkbox_set_checked (OMARCHY_CHECKBOX (c), checked);
    omarchy_checkbox_set_cursor (OMARCHY_CHECKBOX (c), self->cursor_index == i || (focus && checked));
  }
}

static void
group_choose (OmarchyRadioGroup *self, int index)
{
  if (index < 0 || index >= (int) self->values->len)
    return;
  const char *v = g_ptr_array_index (self->values, index);
  if (!g_strcmp0 (v, self->value)) {
    group_sync (self);
    return;
  }
  omarchy_radio_group_set_value (self, v);
  g_signal_emit (self, rg_signals[RG_CHANGED], 0, v);
}

static void
group_radio_toggled (OmarchyCheckbox *radio, OmarchyRadioGroup *self)
{
  int i = 0;
  for (GtkWidget *c = gtk_widget_get_first_child (GTK_WIDGET (self)); c; c = gtk_widget_get_next_sibling (c), i++)
    if (c == GTK_WIDGET (radio))
      group_choose (self, i);
}

static gboolean
group_key (GtkWidget *w, guint keyval, GdkModifierType state)
{
  OmarchyRadioGroup *self = OMARCHY_RADIO_GROUP (w);
  int i = group_index_of (self, self->value);
  int n = (int) self->values->len;
  if (keyval == GDK_KEY_Down || keyval == GDK_KEY_KP_Down || keyval == GDK_KEY_Right || keyval == GDK_KEY_KP_Right
      || keyval == GDK_KEY_j) {
    group_choose (self, MIN (n - 1, i + 1));
    return TRUE;
  }
  if (keyval == GDK_KEY_Up || keyval == GDK_KEY_KP_Up || keyval == GDK_KEY_Left || keyval == GDK_KEY_KP_Left
      || keyval == GDK_KEY_k) {
    group_choose (self, MAX (0, i - 1));
    return TRUE;
  }
  return FALSE;
}

static void
group_restyle (OmarchyRadioGroup *self)
{
  GtkLayoutManager *lm = gtk_widget_get_layout_manager (GTK_WIDGET (self));
  gtk_box_layout_set_spacing (GTK_BOX_LAYOUT (lm), omarchy_tokens ()->lg);
}

static void
group_state_changed (GtkWidget *w, GtkStateFlags old, gpointer data)
{
  group_sync (OMARCHY_RADIO_GROUP (w));
}

static void
omarchy_radio_group_init (OmarchyRadioGroup *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  self->values = g_ptr_array_new_with_free_func (g_free);
  self->value = g_strdup ("");
  self->cursor_index = -1;
  GtkLayoutManager *lm = gtk_widget_get_layout_manager (w);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (lm), GTK_ORIENTATION_VERTICAL);
  group_restyle (self);
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (group_restyle), self,
                           G_CONNECT_SWAPPED);
  gtk_widget_set_focusable (w, TRUE);
  gtk_widget_set_focus_on_click (w, FALSE);
  omarchy_widget_add_keys (w, group_key);
  g_signal_connect (w, "state-flags-changed", G_CALLBACK (group_state_changed), NULL);
}

static void
group_dispose (GObject *o)
{
  omarchy_widget_dispose_children (GTK_WIDGET (o));
  G_OBJECT_CLASS (omarchy_radio_group_parent_class)->dispose (o);
}

static void
group_finalize (GObject *o)
{
  OmarchyRadioGroup *self = OMARCHY_RADIO_GROUP (o);
  g_ptr_array_unref (self->values);
  g_free (self->value);
  G_OBJECT_CLASS (omarchy_radio_group_parent_class)->finalize (o);
}

static void
group_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyRadioGroup *self = OMARCHY_RADIO_GROUP (o);
  switch (id) {
  case RG_PROP_VALUE: omarchy_radio_group_set_value (self, g_value_get_string (v)); break;
  case RG_PROP_CURSOR_INDEX: omarchy_radio_group_set_cursor_index (self, g_value_get_int (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
group_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyRadioGroup *self = OMARCHY_RADIO_GROUP (o);
  switch (id) {
  case RG_PROP_VALUE: g_value_set_string (v, self->value); break;
  case RG_PROP_CURSOR_INDEX: g_value_set_int (v, self->cursor_index); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_radio_group_class_init (OmarchyRadioGroupClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->dispose = group_dispose;
  oc->finalize = group_finalize;
  oc->set_property = group_set_property;
  oc->get_property = group_get_property;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  rg_props[RG_PROP_VALUE] = g_param_spec_string ("value", NULL, NULL, "", f);
  /**
   * OmarchyRadioGroup:cursor-index:
   *
   * The row wearing the cursor look, or -1.
   */
  rg_props[RG_PROP_CURSOR_INDEX] = g_param_spec_int ("cursor-index", NULL, NULL, -1, G_MAXINT, -1, f);
  g_object_class_install_properties (oc, RG_N_PROPS, rg_props);
  /**
   * OmarchyRadioGroup::changed:
   * @self: the radiogroup
   * @value: the chosen option's value
   *
   * A click or a key picked another option.
   */
  rg_signals[RG_CHANGED] = g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, NULL,
                                       G_TYPE_NONE, 1, G_TYPE_STRING);
  gtk_widget_class_set_layout_manager_type (wc, GTK_TYPE_BOX_LAYOUT);
  gtk_widget_class_set_css_name (wc, "omarchy-radio-group");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_RADIO_GROUP);
}

/**
 * omarchy_radio_group_new:
 *
 * Returns: (transfer floating): an empty group; add options with
 *   [method@Omarchy.RadioGroup.append]
 */
GtkWidget *
omarchy_radio_group_new (void)
{
  return g_object_new (OMARCHY_TYPE_RADIO_GROUP, NULL);
}

/**
 * omarchy_radio_group_append:
 * @self: the group
 * @value: the option's value
 * @label: (nullable): what it reads (@value when %NULL)
 * @description: (nullable): a line under it
 */
void
omarchy_radio_group_append (OmarchyRadioGroup *self, const char *value, const char *label, const char *description)
{
  g_ptr_array_add (self->values, g_strdup (value ? value : ""));
  GtkWidget *radio = omarchy_radio_new (label ? label : value);
  if (description)
    omarchy_checkbox_set_description (OMARCHY_CHECKBOX (radio), description);
  gtk_widget_set_focusable (radio, FALSE);
  g_signal_connect (radio, "toggled", G_CALLBACK (group_radio_toggled), self);
  gtk_widget_set_parent (radio, GTK_WIDGET (self));
  group_sync (self);
}

void
omarchy_radio_group_remove_all (OmarchyRadioGroup *self)
{
  omarchy_widget_dispose_children (GTK_WIDGET (self));
  g_ptr_array_set_size (self->values, 0);
}

const char *
omarchy_radio_group_get_value (OmarchyRadioGroup *self)
{
  return self->value;
}

void
omarchy_radio_group_set_value (OmarchyRadioGroup *self, const char *value)
{
  if (!g_strcmp0 (self->value, value))
    return;
  g_free (self->value);
  self->value = g_strdup (value ? value : "");
  group_sync (self);
  g_object_notify_by_pspec (G_OBJECT (self), rg_props[RG_PROP_VALUE]);
}

int
omarchy_radio_group_get_cursor_index (OmarchyRadioGroup *self)
{
  return self->cursor_index;
}

void
omarchy_radio_group_set_cursor_index (OmarchyRadioGroup *self, int index)
{
  index = MAX (-1, index);
  if (self->cursor_index == index)
    return;
  self->cursor_index = index;
  group_sync (self);
  g_object_notify_by_pspec (G_OBJECT (self), rg_props[RG_PROP_CURSOR_INDEX]);
}
