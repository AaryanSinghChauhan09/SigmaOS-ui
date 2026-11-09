/* omarchy-label.c: Qt's `Text` with the kit's font: a token size, bold only for titles and
 * selection, a palette role for color. Boxed as Qt boxes text (logical width, the font's line
 * height), so stacks of labels line up with the Qt kit's. */
#include "omarchy-private.h"

#include <math.h>
#include <string.h>

/**
 * OmarchyLabel:
 *
 * Text in the Omarchy font at a type-scale token (`body`, `caption`, …) and in a palette role
 * (`foreground`, `dim`, `faint`, `accent`, …), following the theme and its fade.
 */
struct _OmarchyLabel {
  GtkWidget parent_instance;
  char *text;
  char *size;
  int pixel_size; /* overrides size when > 0 */
  gboolean bold;
  char *role;
  gboolean has_color;
  GdkRGBA color;
  gboolean wrap;
  gboolean ellipsize;
  float xalign;
  int top_padding;
};

enum { PROP_0, PROP_TEXT, PROP_SIZE, PROP_BOLD, PROP_ROLE, PROP_WRAP, PROP_ELLIPSIZE, PROP_XALIGN, N_PROPS };
static GParamSpec *props[N_PROPS];

G_DEFINE_FINAL_TYPE (OmarchyLabel, omarchy_label, GTK_TYPE_WIDGET)

static int
token_px (const char *token)
{
  const OmarchyTokens *t = omarchy_tokens ();
  if (!token || !*token)
    return t->body;
#define T(name, field) if (strcmp (token, name) == 0) return t->field;
  T ("caption", caption) T ("bodySmall", body_small) T ("body", body) T ("subtitle", subtitle) T ("title", title)
  T ("heading", heading) T ("display", display) T ("displayLarge", display_large) T ("iconSmall", icon_small)
  T ("icon", icon) T ("iconLarge", icon_large)
#undef T
  return t->body;
}

GdkRGBA
omarchy_role_color (const char *role)
{
  const OmarchyTokens *p = omarchy_paint ();
  if (!role || !*role || !strcmp (role, "foreground"))
    return p->foreground;
#define R(name, value) if (!strcmp (role, name)) return value;
  R ("dim", p->text_dim) R ("faint", p->text_faint) R ("accent", p->accent) R ("urgent", p->urgent)
  R ("success", p->success) R ("warning", p->warning) R ("muted", p->muted) R ("background", p->background)
  R ("selected", p->selected.text) R ("popup", p->popups.text) R ("tooltip", p->tooltip.text) R ("track", p->track)
#undef R
  return p->foreground;
}

static int
label_px (OmarchyLabel *self)
{
  return self->pixel_size > 0 ? self->pixel_size : token_px (self->size);
}

static PangoLayout *
make_layout (OmarchyLabel *self, int width)
{
  PangoLayout *l = omarchy_layout (GTK_WIDGET (self), self->text, label_px (self), self->bold);
  if (width >= 0 && (self->wrap || self->ellipsize)) {
    pango_layout_set_width (l, width * PANGO_SCALE);
    if (self->wrap)
      pango_layout_set_wrap (l, PANGO_WRAP_WORD_CHAR);
    else
      pango_layout_set_ellipsize (l, PANGO_ELLIPSIZE_END);
  }
  return l;
}

static void
measure (GtkWidget *w, GtkOrientation o, int for_size, int *min, int *nat, int *min_base, int *nat_base)
{
  OmarchyLabel *self = OMARCHY_LABEL (w);
  int line = omarchy_line_height (label_px (self));
  if (o == GTK_ORIENTATION_HORIZONTAL) {
    g_autoptr (PangoLayout) l = make_layout (self, -1);
    int width;
    omarchy_layout_size (l, &width, NULL);
    *nat = width;
    *min = (self->wrap || self->ellipsize) ? MIN (width, line) : width;
  } else {
    int h = line;
    if (self->wrap && for_size >= 0) {
      g_autoptr (PangoLayout) l = make_layout (self, for_size);
      int lines = MAX (1, pango_layout_get_line_count (l));
      h = lines == 1 ? line : (int) ceil (pango_layout_get_height (l) / (double) PANGO_SCALE);
      if (lines > 1) {
        PangoRectangle logical;
        pango_layout_get_extents (l, NULL, &logical);
        h = MAX (line, (int) ceil (logical.height / (double) PANGO_SCALE));
      }
    }
    *min = *nat = h + self->top_padding;
  }
}

static GtkSizeRequestMode
request_mode (GtkWidget *w)
{
  return OMARCHY_LABEL (w)->wrap ? GTK_SIZE_REQUEST_HEIGHT_FOR_WIDTH : GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
snapshot (GtkWidget *w, GtkSnapshot *s)
{
  OmarchyLabel *self = OMARCHY_LABEL (w);
  if (!self->text || !*self->text)
    return;
  int width = gtk_widget_get_width (w);
  g_autoptr (PangoLayout) l = make_layout (self, width);
  int tw;
  omarchy_layout_size (l, &tw, NULL);
  float x = 0;
  if (!self->wrap && tw < width) {
    float align = gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL ? 1 - self->xalign : self->xalign;
    x = roundf ((width - tw) * align);
  } else if (self->wrap) {
    pango_layout_set_alignment (l, self->xalign >= 0.99 ? PANGO_ALIGN_RIGHT : self->xalign >= 0.49 ? PANGO_ALIGN_CENTER : PANGO_ALIGN_LEFT);
  }
  GdkRGBA c = self->has_color ? self->color : omarchy_role_color (self->role);
  if (!gtk_widget_is_sensitive (w))
    c = omarchy_paint ()->text_faint;
  omarchy_draw_layout (s, l, x, self->top_padding, &c);
}

static void
omarchy_label_init (OmarchyLabel *self)
{
  self->text = g_strdup ("");
  self->size = g_strdup ("body");
  self->role = g_strdup ("foreground");
  self->xalign = 0;
  gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, "", -1);
  omarchy_widget_follow (GTK_WIDGET (self));
}

static void
finalize (GObject *o)
{
  OmarchyLabel *self = OMARCHY_LABEL (o);
  g_free (self->text);
  g_free (self->size);
  g_free (self->role);
  G_OBJECT_CLASS (omarchy_label_parent_class)->finalize (o);
}

static void
set_property (GObject *o, guint id, const GValue *v, GParamSpec *pspec)
{
  OmarchyLabel *self = OMARCHY_LABEL (o);
  switch (id) {
  case PROP_TEXT: omarchy_label_set_text (self, g_value_get_string (v)); break;
  case PROP_SIZE: omarchy_label_set_size (self, g_value_get_string (v)); break;
  case PROP_BOLD: omarchy_label_set_bold (self, g_value_get_boolean (v)); break;
  case PROP_ROLE: omarchy_label_set_role (self, g_value_get_string (v)); break;
  case PROP_WRAP: omarchy_label_set_wrap (self, g_value_get_boolean (v)); break;
  case PROP_ELLIPSIZE: omarchy_label_set_ellipsize (self, g_value_get_boolean (v)); break;
  case PROP_XALIGN: omarchy_label_set_xalign (self, g_value_get_float (v)); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
get_property (GObject *o, guint id, GValue *v, GParamSpec *pspec)
{
  OmarchyLabel *self = OMARCHY_LABEL (o);
  switch (id) {
  case PROP_TEXT: g_value_set_string (v, self->text); break;
  case PROP_SIZE: g_value_set_string (v, self->size); break;
  case PROP_BOLD: g_value_set_boolean (v, self->bold); break;
  case PROP_ROLE: g_value_set_string (v, self->role); break;
  case PROP_WRAP: g_value_set_boolean (v, self->wrap); break;
  case PROP_ELLIPSIZE: g_value_set_boolean (v, self->ellipsize); break;
  case PROP_XALIGN: g_value_set_float (v, self->xalign); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (o, id, pspec);
  }
}

static void
omarchy_label_class_init (OmarchyLabelClass *klass)
{
  GObjectClass *oc = G_OBJECT_CLASS (klass);
  GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);
  oc->finalize = finalize;
  oc->set_property = set_property;
  oc->get_property = get_property;
  wc->measure = measure;
  wc->snapshot = snapshot;
  wc->get_request_mode = request_mode;
  const GParamFlags f = G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY;
  props[PROP_TEXT] = g_param_spec_string ("text", NULL, NULL, "", f);
  /**
   * OmarchyLabel:size:
   *
   * The type-scale token: `caption`, `bodySmall`, `body`, `subtitle`, `title`, `heading`,
   * `display`, `displayLarge`, `icon`, `iconSmall`, `iconLarge`.
   */
  props[PROP_SIZE] = g_param_spec_string ("size", NULL, NULL, "body", f);
  props[PROP_BOLD] = g_param_spec_boolean ("bold", NULL, NULL, FALSE, f);
  /**
   * OmarchyLabel:role:
   *
   * The palette role it's drawn in: `foreground`, `dim`, `faint`, `accent`, `urgent`, `success`,
   * `warning`, `muted`, `background`, `selected`, `popup`, `tooltip`.
   */
  props[PROP_ROLE] = g_param_spec_string ("role", NULL, NULL, "foreground", f);
  props[PROP_WRAP] = g_param_spec_boolean ("wrap", NULL, NULL, FALSE, f);
  props[PROP_ELLIPSIZE] = g_param_spec_boolean ("ellipsize", NULL, NULL, FALSE, f);
  props[PROP_XALIGN] = g_param_spec_float ("xalign", NULL, NULL, 0, 1, 0, f);
  g_object_class_install_properties (oc, N_PROPS, props);
  gtk_widget_class_set_css_name (wc, "omarchy-label");
  gtk_widget_class_set_accessible_role (wc, GTK_ACCESSIBLE_ROLE_LABEL);
}

/**
 * omarchy_label_new:
 * @text: (nullable): the text
 *
 * Returns: (transfer floating): a label in `body` and the foreground
 */
GtkWidget *
omarchy_label_new (const char *text)
{
  return g_object_new (OMARCHY_TYPE_LABEL, "text", text ? text : "", NULL);
}

const char *
omarchy_label_get_text (OmarchyLabel *self)
{
  return self->text;
}

void
omarchy_label_set_text (OmarchyLabel *self, const char *text)
{
  if (!g_strcmp0 (self->text, text))
    return;
  g_free (self->text);
  self->text = g_strdup (text ? text : "");
  gtk_accessible_update_property (GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, self->text, -1);
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_TEXT]);
}

const char *
omarchy_label_get_size (OmarchyLabel *self)
{
  return self->size;
}

void
omarchy_label_set_size (OmarchyLabel *self, const char *token)
{
  if (!g_strcmp0 (self->size, token))
    return;
  g_free (self->size);
  self->size = g_strdup (token ? token : "body");
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIZE]);
}

/**
 * omarchy_label_set_pixel_size:
 * @self: the label
 * @px: a size in pixels, or 0 to use #OmarchyLabel:size
 *
 * For a size no token names. Prefer a token.
 */
void
omarchy_label_set_pixel_size (OmarchyLabel *self, int px)
{
  self->pixel_size = px;
  gtk_widget_queue_resize (GTK_WIDGET (self));
}

gboolean
omarchy_label_get_bold (OmarchyLabel *self)
{
  return self->bold;
}

void
omarchy_label_set_bold (OmarchyLabel *self, gboolean bold)
{
  if (self->bold == !!bold)
    return;
  self->bold = !!bold;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_BOLD]);
}

const char *
omarchy_label_get_role (OmarchyLabel *self)
{
  return self->role;
}

void
omarchy_label_set_role (OmarchyLabel *self, const char *role)
{
  self->has_color = FALSE;
  if (!g_strcmp0 (self->role, role))
    return;
  g_free (self->role);
  self->role = g_strdup (role ? role : "foreground");
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ROLE]);
}

/**
 * omarchy_label_set_color:
 * @self: the label
 * @color: (nullable): a color, or %NULL to go back to the role
 *
 * A fixed color instead of a role. It doesn't follow the theme: prefer a role.
 */
void
omarchy_label_set_color (OmarchyLabel *self, const GdkRGBA *color)
{
  self->has_color = color != NULL;
  if (color)
    self->color = *color;
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

gboolean
omarchy_label_get_wrap (OmarchyLabel *self)
{
  return self->wrap;
}

void
omarchy_label_set_wrap (OmarchyLabel *self, gboolean wrap)
{
  self->wrap = !!wrap;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_WRAP]);
}

void
omarchy_label_set_ellipsize (OmarchyLabel *self, gboolean ellipsize)
{
  self->ellipsize = !!ellipsize;
  gtk_widget_queue_resize (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_ELLIPSIZE]);
}

void
omarchy_label_set_xalign (OmarchyLabel *self, float xalign)
{
  self->xalign = CLAMP (xalign, 0, 1);
  gtk_widget_queue_draw (GTK_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_XALIGN]);
}

/**
 * omarchy_label_set_top_padding:
 * @self: the label
 * @px: room above the text
 *
 * Room above the line, as Qt's `topPadding` (SectionHeader reserves the font's overshoot).
 */
void
omarchy_label_set_top_padding (OmarchyLabel *self, int px)
{
  self->top_padding = MAX (0, px);
  gtk_widget_queue_resize (GTK_WIDGET (self));
}
