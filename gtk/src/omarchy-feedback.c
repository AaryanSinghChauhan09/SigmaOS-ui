/* omarchy-feedback.c: Meter (Meter.qml), ProgressBar (ProgressBar.qml) and Spinner (Spinner.qml),
 * drawn as the Qt kit draws them. EmptyState, Banner and Toast are in omarchy-feedback-messages.c. */
#include "omarchy-private.h"

#include <math.h>
#include <string.h>

/* The tone's fill color, on screen now (spec/components/feedback.md, Tone). */
static GdkRGBA
tone_color (const char *tone, gboolean alarming)
{
  const OmarchyTokens *p = omarchy_paint ();
  if (alarming)
    return p->urgent;
  if (!g_strcmp0 (tone, "info"))
    return p->accent;
  if (!g_strcmp0 (tone, "success"))
    return p->success;
  if (!g_strcmp0 (tone, "warning"))
    return p->warning;
  if (!g_strcmp0 (tone, "error"))
    return p->urgent;
  return p->foreground;
}

static double
ease_in_out_quad (double t)
{
  return t < 0.5 ? 2 * t * t : 1 - pow (-2 * t + 2, 2) / 2;
}

/* ================================================================================= Meter === */

/**
 * OmarchyMeter:
 *
 * A determinate level on a track: battery, volume, usage against a limit. An optional header
 * (#OmarchyMeter:label at the start, #OmarchyMeter:value-text at the end, `md` above the bar),
 * then a `meterThickness` track with a fill from the leading edge in the
 * #OmarchyMeter:tone color (urgent when #OmarchyMeter:alarming) and optional marker ticks.
 * The fill's width eases 160 ms out-cubic (none under reduced motion).
 *
 * #OmarchyMeter:indeterminate draws a quarter-track segment sweeping across instead, held still
 * in the middle under reduced motion (ProgressBar's mode; Qt's Meter has it too).
 */
typedef struct {
  double value;
  char *tone;
  gboolean alarming;
  GArray *markers;
  char *label;
  char *value_text;
  gboolean indeterminate;
  double thickness; /* < 0: meterThickness */

  double shown, from; /* the fill's fraction on screen, and where its ease started */
  gint64 start;
  guint fill_tick;
  guint sweep_tick;
  gint64 sweep_start;
} OmarchyMeterPrivate;

enum { M_PROP_0, M_PROP_VALUE, M_PROP_TONE, M_PROP_ALARMING, M_PROP_LABEL, M_PROP_VALUE_TEXT, M_PROP_INDETERMINATE,
       M_PROP_THICKNESS, M_N_PROPS };
static GParamSpec *m_props[M_N_PROPS];

G_DEFINE_TYPE_WITH_PRIVATE (OmarchyMeter, omarchy_meter, GTK_TYPE_WIDGET)

#define METER_PRIV(self) ((OmarchyMeterPrivate *) omarchy_meter_get_instance_private (OMARCHY_METER (self)))

typedef struct {
  float thickness, tick_w, tick_h, bar_h, radius, tick_radius;
  float header_h, bar_y;
  gboolean header;
} MeterGeo;

static MeterGeo
meter_geo (OmarchyMeter *self)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  const OmarchyTokens *t = omarchy_tokens ();
  MeterGeo g = { 0 };
  g.thickness = priv->thickness >= 0 ? (float) priv->thickness : t->meter_thickness;
  g.tick_w = MAX (2, roundf (g.thickness * 0.5f));
  g.tick_h = roundf (g.thickness * 2.5f);
  gboolean ticks = priv->markers->len > 0;
  g.bar_h = ticks ? MAX (g.thickness, g.tick_h) : g.thickness;
  g.radius = MIN (omarchy_radius_small (), g.thickness / 2);
  g.tick_radius = MIN (omarchy_radius_small (), g.tick_w / 2);
  g.header = (priv->label && *priv->label) || (priv->value_text && *priv->value_text);
  g.header_h = g.header ? omarchy_line_height (t->body) : 0;
  g.bar_y = g.header ? g.header_h + t->md : 0;
  return g;
}

static void
meter_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  MeterGeo g = meter_geo (OMARCHY_METER (w));
  if (o == GTK_ORIENTATION_HORIZONTAL) {
    *min = 0;
    *nat = omarchy_space (200);
  } else {
    *min = *nat = (int) ceilf (g.bar_y + g.bar_h);
  }
}

static void
meter_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyMeter *self = OMARCHY_METER (w);
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  const OmarchyTokens *p = omarchy_paint ();
  MeterGeo g = meter_geo (self);
  float W = gtk_widget_get_width (w);
  gboolean rtl = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL;
  GdkRGBA fill = tone_color (priv->tone, priv->alarming);

  if (g.header) {
    /* Value text at the end; the label takes what's left, less a controlGap, and elides. */
    int vw = 0;
    if (priv->value_text && *priv->value_text) {
      g_autoptr (PangoLayout) v = omarchy_layout (w, priv->value_text, p->body, FALSE);
      omarchy_layout_size (v, &vw, NULL);
      omarchy_draw_layout (s, v, rtl ? 0 : W - vw, 0, &p->text_dim);
    }
    if (priv->label && *priv->label) {
      g_autoptr (PangoLayout) l = omarchy_layout (w, priv->label, p->body, FALSE);
      float room = MAX (0, W - vw - p->control_gap);
      int lw;
      omarchy_layout_size (l, &lw, NULL);
      if (lw > room) {
        pango_layout_set_width (l, (int) (room * PANGO_SCALE));
        pango_layout_set_ellipsize (l, PANGO_ELLIPSIZE_END);
        lw = (int) room;
      }
      omarchy_draw_layout (s, l, rtl ? W - lw : 0, 0, &p->foreground);
    }
  }

  float track_y = g.bar_y + roundf ((g.bar_h - g.thickness) / 2);
  graphene_rect_t track = GRAPHENE_RECT_INIT (0, track_y, W, g.thickness);
  omarchy_draw_fill (s, &track, g.radius, &p->track);

  if (priv->indeterminate) {
    float seg = roundf (W * 0.25f);
    float x;
    if (priv->sweep_tick) {
      GdkFrameClock *clock = gtk_widget_get_frame_clock (w);
      gint64 now = clock ? gdk_frame_clock_get_frame_time (clock) : g_get_monotonic_time ();
      double f = fmod ((now - priv->sweep_start) / 1400000.0, 1.0);
      x = (float) (-seg + (W + seg) * ease_in_out_quad (f));
    } else {
      x = (W - seg) / 2;
    }
    if (rtl)
      x = W - seg - x;
    /* Clipped to the track's shape. */
    GskRoundedRect clip;
    gsk_rounded_rect_init_from_rect (&clip, &track, MIN (g.radius, g.thickness / 2));
    gtk_snapshot_push_rounded_clip (s, &clip);
    omarchy_draw_fill (s, &GRAPHENE_RECT_INIT (x, track_y, seg, g.thickness), g.radius, &fill);
    gtk_snapshot_pop (s);
    return;
  }

  float fw = (float) (W * CLAMP (priv->shown, 0, 1));
  omarchy_draw_fill (s, &GRAPHENE_RECT_INIT (rtl ? W - fw : 0, track_y, fw, g.thickness), g.radius, &fill);

  for (guint i = 0; i < priv->markers->len; i++) {
    double m = CLAMP (g_array_index (priv->markers, double, i), 0, 1);
    float along = (float) ((W - g.tick_w) * m);
    float x = rtl ? W - g.tick_w - along : along;
    float y = g.bar_y + roundf ((g.bar_h - g.tick_h) / 2);
    omarchy_draw_fill (s, &GRAPHENE_RECT_INIT (x, y, g.tick_w, g.tick_h), g.tick_radius, &fill);
  }
}

static gboolean
meter_fill_tick (GtkWidget *w, GdkFrameClock *clock, gpointer data)
{
  OmarchyMeterPrivate *priv = METER_PRIV (w);
  double x = (gdk_frame_clock_get_frame_time (clock) - priv->start) / 160000.0;
  double e = 1 - pow (1 - CLAMP (x, 0, 1), 3);
  priv->shown = x >= 1 ? priv->value : priv->from + (priv->value - priv->from) * e;
  gtk_widget_queue_draw (w);
  if (x >= 1) {
    priv->fill_tick = 0;
    return G_SOURCE_REMOVE;
  }
  return G_SOURCE_CONTINUE;
}

static void
meter_show_value (OmarchyMeter *self, gboolean animate)
{
  GtkWidget *w = GTK_WIDGET (self);
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (!animate || omarchy_tokens ()->reduce_motion || !gtk_widget_get_mapped (w)) {
    if (priv->fill_tick) {
      gtk_widget_remove_tick_callback (w, priv->fill_tick);
      priv->fill_tick = 0;
    }
    priv->shown = priv->value;
    gtk_widget_queue_draw (w);
    return;
  }
  priv->from = priv->shown;
  priv->start = g_get_monotonic_time ();
  GdkFrameClock *clock = gtk_widget_get_frame_clock (w);
  if (clock)
    priv->start = gdk_frame_clock_get_frame_time (clock);
  if (!priv->fill_tick)
    priv->fill_tick = gtk_widget_add_tick_callback (w, meter_fill_tick, NULL, NULL);
}

static gboolean
meter_sweep_tick (GtkWidget *w, GdkFrameClock *clock, gpointer data)
{
  gtk_widget_queue_draw (w);
  return G_SOURCE_CONTINUE;
}

/* The indeterminate sweep runs while it's indeterminate, on screen, and motion is allowed. */
static void
meter_update_sweep (OmarchyMeter *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  gboolean run = priv->indeterminate && gtk_widget_get_mapped (w) && !omarchy_tokens ()->reduce_motion;
  if (run && !priv->sweep_tick) {
    GdkFrameClock *clock = gtk_widget_get_frame_clock (w);
    priv->sweep_start = clock ? gdk_frame_clock_get_frame_time (clock) : g_get_monotonic_time ();
    priv->sweep_tick = gtk_widget_add_tick_callback (w, meter_sweep_tick, NULL, NULL);
  } else if (!run && priv->sweep_tick) {
    gtk_widget_remove_tick_callback (w, priv->sweep_tick);
    priv->sweep_tick = 0;
  }
  gtk_widget_queue_draw (w);
}

static void
meter_map (GtkWidget *w)
{
  GTK_WIDGET_CLASS (omarchy_meter_parent_class)->map (w);
  meter_update_sweep (OMARCHY_METER (w));
}

static void
meter_unmap (GtkWidget *w)
{
  OmarchyMeterPrivate *priv = METER_PRIV (w);
  if (priv->fill_tick) {
    gtk_widget_remove_tick_callback (w, priv->fill_tick);
    priv->fill_tick = 0;
    priv->shown = priv->value;
  }
  if (priv->sweep_tick) {
    gtk_widget_remove_tick_callback (w, priv->sweep_tick);
    priv->sweep_tick = 0;
  }
  GTK_WIDGET_CLASS (omarchy_meter_parent_class)->unmap (w);
}

static void
meter_update_accessible (OmarchyMeter *self)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, priv->label,
                                  GTK_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.0, GTK_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.0, -1);
  if (priv->indeterminate) {
    gtk_accessible_reset_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_NOW);
    gtk_accessible_update_state (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_STATE_BUSY, TRUE, -1);
  } else {
    gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_NOW, priv->value, -1);
    if (priv->value_text && *priv->value_text)
      gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_VALUE_TEXT, priv->value_text, -1);
    gtk_accessible_update_state (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_STATE_BUSY, FALSE, -1);
  }
}

static void
omarchy_meter_init (OmarchyMeter *self)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  priv->tone = g_strdup ("neutral");
  priv->label = g_strdup ("");
  priv->value_text = g_strdup ("");
  priv->thickness = -1;
  priv->markers = g_array_new (FALSE, FALSE, sizeof (double));
  omarchy_widget_follow (GTK_WIDGET (self));
  /* Reduced motion switched on or off: start or stop the sweep. */
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (meter_update_sweep), self,
                           G_CONNECT_SWAPPED);
  meter_update_accessible (self);
}

static void
meter_finalize (GObject *o)
{
  OmarchyMeterPrivate *priv = METER_PRIV (o);
  g_free (priv->tone);
  g_free (priv->label);
  g_free (priv->value_text);
  g_array_unref (priv->markers);
  G_OBJECT_CLASS (omarchy_meter_parent_class)->finalize (o);
}

static void
meter_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyMeter *self = OMARCHY_METER (o);
  switch (id) {
  case M_PROP_VALUE: omarchy_meter_set_value (self, g_value_get_double (v)); break;
  case M_PROP_TONE: omarchy_meter_set_tone (self, g_value_get_string (v)); break;
  case M_PROP_ALARMING: omarchy_meter_set_alarming (self, g_value_get_boolean (v)); break;
  case M_PROP_LABEL: omarchy_meter_set_label (self, g_value_get_string (v)); break;
  case M_PROP_VALUE_TEXT: omarchy_meter_set_value_text (self, g_value_get_string (v)); break;
  case M_PROP_INDETERMINATE: omarchy_meter_set_indeterminate (self, g_value_get_boolean (v)); break;
  case M_PROP_THICKNESS: omarchy_meter_set_thickness (self, g_value_get_double (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
meter_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyMeterPrivate *priv = METER_PRIV (o);
  switch (id) {
  case M_PROP_VALUE: g_value_set_double (v, priv->value); break;
  case M_PROP_TONE: g_value_set_string (v, priv->tone); break;
  case M_PROP_ALARMING: g_value_set_boolean (v, priv->alarming); break;
  case M_PROP_LABEL: g_value_set_string (v, priv->label); break;
  case M_PROP_VALUE_TEXT: g_value_set_string (v, priv->value_text); break;
  case M_PROP_INDETERMINATE: g_value_set_boolean (v, priv->indeterminate); break;
  case M_PROP_THICKNESS: g_value_set_double (v, priv->thickness); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_meter_class_init (OmarchyMeterClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->finalize = meter_finalize;
  oc->set_property = meter_set_property;
  oc->get_property = meter_get_property;
  wc->measure = meter_measure;
  wc->snapshot = meter_snapshot;
  wc->map = meter_map;
  wc->unmap = meter_unmap;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  /**
   * OmarchyMeter:value:
   *
   * The level, 0 to 1 (clamped when drawn).
   */
  m_props[M_PROP_VALUE] = g_param_spec_double ("value", NULL, NULL, -G_MAXDOUBLE, G_MAXDOUBLE, 0, f);
  /**
   * OmarchyMeter:tone:
   *
   * The fill's color: "neutral" (foreground), "info" (accent), "success", "warning" or "error"
   * (urgent).
   */
  m_props[M_PROP_TONE] = g_param_spec_string ("tone", NULL, NULL, "neutral", f);
  /**
   * OmarchyMeter:alarming:
   *
   * The fill (and markers) turn urgent whatever the tone.
   */
  m_props[M_PROP_ALARMING] = g_param_spec_boolean ("alarming", NULL, NULL, FALSE, f);
  /**
   * OmarchyMeter:label:
   *
   * The header's text at the start (and the accessible name).
   */
  m_props[M_PROP_LABEL] = g_param_spec_string ("label", NULL, NULL, "", f);
  /**
   * OmarchyMeter:value-text:
   *
   * The header's text at the end, in textDim ("65%").
   */
  m_props[M_PROP_VALUE_TEXT] = g_param_spec_string ("value-text", NULL, NULL, "", f);
  /**
   * OmarchyMeter:indeterminate:
   *
   * Work of unknown length: a quarter-track segment sweeps across every 1.4 s.
   */
  m_props[M_PROP_INDETERMINATE] = g_param_spec_boolean ("indeterminate", NULL, NULL, FALSE, f);
  /**
   * OmarchyMeter:thickness:
   *
   * The track's height in pixels, or -1 for `meterThickness`.
   */
  m_props[M_PROP_THICKNESS] = g_param_spec_double ("thickness", NULL, NULL, -1, G_MAXDOUBLE, -1, f);
  g_object_class_install_properties (oc, M_N_PROPS, m_props);
  gtk_widget_class_set_css_name (wc, "omarchy-meter");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_PROGRESS_BAR);
}

/**
 * omarchy_meter_new:
 * @value: the level, 0 to 1
 *
 * Returns: (transfer floating): a meter
 */
GtkWidget *
omarchy_meter_new (double value)
{
  return g_object_new (OMARCHY_TYPE_METER, "value", value, NULL);
}

double
omarchy_meter_get_value (OmarchyMeter *self)
{
  return METER_PRIV (self)->value;
}

void
omarchy_meter_set_value (OmarchyMeter *self, double value)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (priv->value == value)
    return;
  priv->value = value;
  meter_show_value (self, TRUE);
  meter_update_accessible (self);
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_VALUE]);
}

const char *
omarchy_meter_get_tone (OmarchyMeter *self)
{
  return METER_PRIV (self)->tone;
}

void
omarchy_meter_set_tone (OmarchyMeter *self, const char *tone)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (!tone || !*tone)
    tone = "neutral";
  if (!g_strcmp0 (priv->tone, tone))
    return;
  g_free (priv->tone);
  priv->tone = g_strdup (tone);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_TONE]);
}

gboolean
omarchy_meter_get_alarming (OmarchyMeter *self)
{
  return METER_PRIV (self)->alarming;
}

void
omarchy_meter_set_alarming (OmarchyMeter *self, gboolean alarming)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (priv->alarming == !!alarming)
    return;
  priv->alarming = !!alarming;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_ALARMING]);
}

/**
 * omarchy_meter_add_marker:
 * @self: the meter
 * @fraction: where along the track, 0 to 1
 *
 * A tick across the track in the fill's color (the first and last sit inside the ends).
 */
void
omarchy_meter_add_marker (OmarchyMeter *self, double fraction)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  g_array_append_val (priv->markers, fraction);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

void
omarchy_meter_clear_markers (OmarchyMeter *self)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  g_array_set_size (priv->markers, 0);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

guint
omarchy_meter_get_n_markers (OmarchyMeter *self)
{
  return METER_PRIV (self)->markers->len;
}

/**
 * omarchy_meter_get_marker:
 * @self: the meter
 * @index: which marker, below [method@Omarchy.Meter.get_n_markers]
 *
 * Returns: the marker's fraction, or -1 past the end
 */
double
omarchy_meter_get_marker (OmarchyMeter *self, guint index)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  return index < priv->markers->len ? g_array_index (priv->markers, double, index) : -1;
}

const char *
omarchy_meter_get_label (OmarchyMeter *self)
{
  return METER_PRIV (self)->label;
}

void
omarchy_meter_set_label (OmarchyMeter *self, const char *label)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (!g_strcmp0 (priv->label, label))
    return;
  g_free (priv->label);
  priv->label = g_strdup (label ? label : "");
  meter_update_accessible (self);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_LABEL]);
}

const char *
omarchy_meter_get_value_text (OmarchyMeter *self)
{
  return METER_PRIV (self)->value_text;
}

void
omarchy_meter_set_value_text (OmarchyMeter *self, const char *value_text)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (!g_strcmp0 (priv->value_text, value_text))
    return;
  g_free (priv->value_text);
  priv->value_text = g_strdup (value_text ? value_text : "");
  meter_update_accessible (self);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_VALUE_TEXT]);
}

gboolean
omarchy_meter_get_indeterminate (OmarchyMeter *self)
{
  return METER_PRIV (self)->indeterminate;
}

void
omarchy_meter_set_indeterminate (OmarchyMeter *self, gboolean indeterminate)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  if (priv->indeterminate == !!indeterminate)
    return;
  priv->indeterminate = !!indeterminate;
  meter_update_sweep (self);
  meter_update_accessible (self);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_INDETERMINATE]);
}

double
omarchy_meter_get_thickness (OmarchyMeter *self)
{
  return METER_PRIV (self)->thickness;
}

/**
 * omarchy_meter_set_thickness:
 * @self: the meter
 * @thickness: the track's height, or -1 for `meterThickness`
 */
void
omarchy_meter_set_thickness (OmarchyMeter *self, double thickness)
{
  OmarchyMeterPrivate *priv = METER_PRIV (self);
  thickness = thickness < 0 ? -1 : thickness;
  if (priv->thickness == thickness)
    return;
  priv->thickness = thickness;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), m_props[M_PROP_THICKNESS]);
}

/* =========================================================================== ProgressBar === */

/**
 * OmarchyProgressBar:
 *
 * Progress of a task: an #OmarchyMeter whose fill is the accent ("info" tone), as the OSD and
 * the apps' export bars draw it, with #OmarchyMeter:indeterminate for work of unknown length.
 */
struct _OmarchyProgressBar {
  OmarchyMeter parent_instance;
};

G_DEFINE_FINAL_TYPE (OmarchyProgressBar, omarchy_progress_bar, OMARCHY_TYPE_METER)

static void
omarchy_progress_bar_init (OmarchyProgressBar *self)
{
  omarchy_meter_set_tone (OMARCHY_METER (self), "info");
}

static void
omarchy_progress_bar_class_init (OmarchyProgressBarClass *klass)
{
  gtk_widget_class_set_css_name (GTK_WIDGET_CLASS (klass), "omarchy-progress-bar");
}

/**
 * omarchy_progress_bar_new:
 * @value: done so far, 0 to 1
 *
 * Returns: (transfer floating): a determinate progress bar
 */
GtkWidget *
omarchy_progress_bar_new (double value)
{
  return g_object_new (OMARCHY_TYPE_PROGRESS_BAR, "value", value, NULL);
}

/**
 * omarchy_progress_bar_new_indeterminate:
 *
 * Returns: (transfer floating): a progress bar for work of unknown length
 */
GtkWidget *
omarchy_progress_bar_new_indeterminate (void)
{
  return g_object_new (OMARCHY_TYPE_PROGRESS_BAR, "indeterminate", TRUE, NULL);
}

/* =============================================================================== Spinner === */

/**
 * OmarchySpinner:
 *
 * Busy: a Nerd Font glyph (`󰦖` by default) turning clockwise once every 900 ms about its own
 * middle, or with #OmarchySpinner:pulse breathing between half and full opacity, 420 ms each
 * way. The box is the glyph's line height, square, the ink centered across it. Under reduced
 * motion it stands upright at full opacity (a pulse rests at half).
 */
struct _OmarchySpinner {
  GtkWidget parent_instance;
  char *glyph;
  char *size;
  int pixel_size;
  gboolean pulse;
  gboolean running;
  gboolean has_color;
  GdkRGBA color;
  guint tick;
  gint64 start;
};

enum { S_PROP_0, S_PROP_GLYPH, S_PROP_SIZE, S_PROP_PULSE, S_PROP_RUNNING, S_PROP_COLOR, S_N_PROPS };
static GParamSpec *s_props[S_N_PROPS];

G_DEFINE_FINAL_TYPE (OmarchySpinner, omarchy_spinner, GTK_TYPE_WIDGET)

static int
spinner_px (OmarchySpinner *self)
{
  if (self->pixel_size > 0)
    return self->pixel_size;
  int px = omarchy_theme_get_font_size (omarchy_theme_get_default (), self->size);
  return px > 0 ? px : omarchy_tokens ()->icon;
}

static void
spinner_measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *mb, int *nb)
{
  *min = *nat = omarchy_line_height (spinner_px (OMARCHY_SPINNER (w)));
}

static gboolean
spinner_animating (OmarchySpinner *self)
{
  return self->tick != 0;
}

static void
spinner_snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchySpinner *self = OMARCHY_SPINNER (w);
  const OmarchyTokens *p = omarchy_paint ();
  if (!self->glyph || !*self->glyph)
    return;
  int px = spinner_px (self);
  float W = gtk_widget_get_width (w), H = gtk_widget_get_height (w);
  g_autoptr (PangoLayout) l = omarchy_layout (w, self->glyph, px, FALSE);
  PangoRectangle ink;
  pango_layout_get_extents (l, &ink, NULL);
  /* The ink centered across the box (TextMetrics.tightBoundingRect), the line box centered down. */
  float x = roundf ((W - ink.width / (float) PANGO_SCALE) / 2 - ink.x / (float) PANGO_SCALE);
  float y = roundf ((H - omarchy_line_height (px)) / 2);
  GdkRGBA c = self->has_color ? self->color : p->foreground;

  double angle = 0, opacity = self->pulse ? 0.5 : 1;
  if (spinner_animating (self)) {
    GdkFrameClock *clock = gtk_widget_get_frame_clock (w);
    gint64 now = clock ? gdk_frame_clock_get_frame_time (clock) : g_get_monotonic_time ();
    double ms = (now - self->start) / 1000.0;
    if (self->pulse) {
      double ph = fmod (ms, 840.0);
      double e = ease_in_out_quad (ph < 420 ? ph / 420.0 : (ph - 420) / 420.0);
      opacity = ph < 420 ? 0.5 + 0.5 * e : 1 - 0.5 * e;
    } else {
      angle = 360.0 * fmod (ms, 900.0) / 900.0;
    }
  }
  c.alpha *= (float) opacity;
  if (angle != 0) {
    gtk_snapshot_save (s);
    gtk_snapshot_translate (s, &GRAPHENE_POINT_INIT (W / 2, H / 2));
    gtk_snapshot_rotate (s, (float) angle);
    gtk_snapshot_translate (s, &GRAPHENE_POINT_INIT (-W / 2, -H / 2));
  }
  omarchy_draw_layout (s, l, x, y, &c);
  if (angle != 0)
    gtk_snapshot_restore (s);
}

static gboolean
spinner_tick (GtkWidget *w, GdkFrameClock *clock, gpointer data)
{
  gtk_widget_queue_draw (w);
  return G_SOURCE_CONTINUE;
}

static void
spinner_update (OmarchySpinner *self)
{
  GtkWidget *w = GTK_WIDGET (self);
  gboolean run = self->running && gtk_widget_get_mapped (w) && !omarchy_tokens ()->reduce_motion;
  if (run && !self->tick) {
    GdkFrameClock *clock = gtk_widget_get_frame_clock (w);
    self->start = clock ? gdk_frame_clock_get_frame_time (clock) : g_get_monotonic_time ();
    self->tick = gtk_widget_add_tick_callback (w, spinner_tick, NULL, NULL);
  } else if (!run && self->tick) {
    gtk_widget_remove_tick_callback (w, self->tick);
    self->tick = 0;
  }
  gtk_widget_queue_draw (w);
}

static void
spinner_map (GtkWidget *w)
{
  GTK_WIDGET_CLASS (omarchy_spinner_parent_class)->map (w);
  spinner_update (OMARCHY_SPINNER (w));
}

static void
spinner_unmap (GtkWidget *w)
{
  OmarchySpinner *self = OMARCHY_SPINNER (w);
  if (self->tick) {
    gtk_widget_remove_tick_callback (w, self->tick);
    self->tick = 0;
  }
  GTK_WIDGET_CLASS (omarchy_spinner_parent_class)->unmap (w);
}

static void
omarchy_spinner_init (OmarchySpinner *self)
{
  self->glyph = g_strdup ("\U000F0996");
  self->size = g_strdup ("icon");
  self->running = TRUE;
  omarchy_widget_follow (GTK_WIDGET (self));
  g_signal_connect_object (omarchy_theme_get_default (), "restyle", G_CALLBACK (spinner_update), self,
                           G_CONNECT_SWAPPED);
  gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL,
                                  omarchy_t ("common.busy", "Busy"), -1);
  gtk_accessible_update_state (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_STATE_BUSY, TRUE, -1);
}

static void
spinner_finalize (GObject *o)
{
  OmarchySpinner *self = OMARCHY_SPINNER (o);
  g_free (self->glyph);
  g_free (self->size);
  G_OBJECT_CLASS (omarchy_spinner_parent_class)->finalize (o);
}

static void
spinner_set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchySpinner *self = OMARCHY_SPINNER (o);
  switch (id) {
  case S_PROP_GLYPH: omarchy_spinner_set_glyph (self, g_value_get_string (v)); break;
  case S_PROP_SIZE: omarchy_spinner_set_size (self, g_value_get_string (v)); break;
  case S_PROP_PULSE: omarchy_spinner_set_pulse (self, g_value_get_boolean (v)); break;
  case S_PROP_RUNNING: omarchy_spinner_set_running (self, g_value_get_boolean (v)); break;
  case S_PROP_COLOR: omarchy_spinner_set_color (self, g_value_get_boxed (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
spinner_get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchySpinner *self = OMARCHY_SPINNER (o);
  switch (id) {
  case S_PROP_GLYPH: g_value_set_string (v, self->glyph); break;
  case S_PROP_SIZE: g_value_set_string (v, self->size); break;
  case S_PROP_PULSE: g_value_set_boolean (v, self->pulse); break;
  case S_PROP_RUNNING: g_value_set_boolean (v, self->running); break;
  case S_PROP_COLOR: g_value_set_boxed (v, self->has_color ? &self->color : NULL); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_spinner_class_init (OmarchySpinnerClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->finalize = spinner_finalize;
  oc->set_property = spinner_set_property;
  oc->get_property = spinner_get_property;
  wc->measure = spinner_measure;
  wc->snapshot = spinner_snapshot;
  wc->map = spinner_map;
  wc->unmap = spinner_unmap;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  /**
   * OmarchySpinner:glyph:
   *
   * The Nerd Font glyph that turns or breathes (`󰦖`, U+F0996, by default).
   */
  s_props[S_PROP_GLYPH] = g_param_spec_string ("glyph", NULL, NULL, "\U000F0996", f);
  /**
   * OmarchySpinner:size:
   *
   * A font-size token ("icon", "iconLarge", …); `icon` by default.
   */
  s_props[S_PROP_SIZE] = g_param_spec_string ("size", NULL, NULL, "icon", f);
  /**
   * OmarchySpinner:pulse:
   *
   * Breathe between half and full opacity instead of turning.
   */
  s_props[S_PROP_PULSE] = g_param_spec_boolean ("pulse", NULL, NULL, FALSE, f);
  /**
   * OmarchySpinner:running:
   *
   * Stop the animation without hiding the glyph.
   */
  s_props[S_PROP_RUNNING] = g_param_spec_boolean ("running", NULL, NULL, TRUE, f);
  /**
   * OmarchySpinner:color:
   *
   * The glyph's color; %NULL for the foreground (following the theme).
   */
  s_props[S_PROP_COLOR] = g_param_spec_boxed ("color", NULL, NULL, GDK_TYPE_RGBA, f);
  g_object_class_install_properties (oc, S_N_PROPS, s_props);
  gtk_widget_class_set_css_name (wc, "omarchy-spinner");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_PROGRESS_BAR);
}

/**
 * omarchy_spinner_new:
 *
 * Returns: (transfer floating): a turning `󰦖` at the `icon` size
 */
GtkWidget *
omarchy_spinner_new (void)
{
  return g_object_new (OMARCHY_TYPE_SPINNER, NULL);
}

OMARCHY_STRING_ACCESSORS (OmarchySpinner, spinner, glyph, s_props, S_PROP_GLYPH, TRUE)

const char *
omarchy_spinner_get_size (OmarchySpinner *self)
{
  return self->size;
}

void
omarchy_spinner_set_size (OmarchySpinner *self, const char *token)
{
  if (!token || !*token)
    token = "icon";
  self->pixel_size = 0;
  if (!g_strcmp0 (self->size, token))
    return;
  g_free (self->size);
  self->size = g_strdup (token);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), s_props[S_PROP_SIZE]);
}

/**
 * omarchy_spinner_set_pixel_size:
 * @self: the spinner
 * @px: the glyph's size in pixels, or 0 to go back to #OmarchySpinner:size
 *
 * A size no token names. Prefer a token.
 */
void
omarchy_spinner_set_pixel_size (OmarchySpinner *self, int px)
{
  self->pixel_size = MAX (0, px);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

gboolean
omarchy_spinner_get_pulse (OmarchySpinner *self)
{
  return self->pulse;
}

void
omarchy_spinner_set_pulse (OmarchySpinner *self, gboolean pulse)
{
  if (self->pulse == !!pulse)
    return;
  self->pulse = !!pulse;
  GdkFrameClock *clock = gtk_widget_get_frame_clock (GTK_WIDGET (self));
  self->start = clock ? gdk_frame_clock_get_frame_time (clock) : g_get_monotonic_time ();
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), s_props[S_PROP_PULSE]);
}

gboolean
omarchy_spinner_get_running (OmarchySpinner *self)
{
  return self->running;
}

void
omarchy_spinner_set_running (OmarchySpinner *self, gboolean running)
{
  if (self->running == !!running)
    return;
  self->running = !!running;
  spinner_update (self);
  g_object_notify_by_pspec (G_OBJECT (self), s_props[S_PROP_RUNNING]);
}

/**
 * omarchy_spinner_set_color:
 * @self: the spinner
 * @color: (nullable): a color, or %NULL for the foreground
 *
 * A fixed color. It doesn't follow the theme.
 */
void
omarchy_spinner_set_color (OmarchySpinner *self, const GdkRGBA *color)
{
  self->has_color = color != NULL;
  if (color)
    self->color = *color;
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), s_props[S_PROP_COLOR]);
}
