/* omarchy-tokens.c: reading libomarchy-theme's canonical JSON into OmarchyTokens. */
#include "omarchy-tokens.h"

#include <math.h>
#include <string.h>

static JsonNode *
at (JsonNode *root, const char *path)
{
  JsonNode *node = root;
  g_auto (GStrv) parts = g_strsplit (path, ".", -1);
  for (char **p = parts; *p && node; p++) {
    if (!JSON_NODE_HOLDS_OBJECT (node))
      return NULL;
    JsonObject *o = json_node_get_object (node);
    node = json_object_has_member (o, *p) ? json_object_get_member (o, *p) : NULL;
  }
  return node;
}

static gboolean
parse_hex (const char *s, GdkRGBA *out)
{
  /* "#rrggbbaa" */
  if (!s || s[0] != '#' || strlen (s) != 9)
    return gdk_rgba_parse (out, s ? s : "");
  unsigned v[4];
  for (int i = 0; i < 4; i++) {
    char b[3] = { s[1 + i * 2], s[2 + i * 2], 0 };
    v[i] = (unsigned) strtoul (b, NULL, 16);
  }
  out->red = v[0] / 255.f;
  out->green = v[1] / 255.f;
  out->blue = v[2] / 255.f;
  out->alpha = v[3] / 255.f;
  return TRUE;
}

static void
color (JsonNode *root, const char *path, GdkRGBA *out)
{
  JsonNode *n = at (root, path);
  if (!n || !JSON_NODE_HOLDS_VALUE (n) || !parse_hex (json_node_get_string (n), out))
    *out = (GdkRGBA) { 0, 0, 0, 0 };
}

static int
integer (JsonNode *root, const char *path)
{
  JsonNode *n = at (root, path);
  if (!n || !JSON_NODE_HOLDS_VALUE (n))
    return 0;
  if (json_node_get_value_type (n) == G_TYPE_DOUBLE)
    return (int) lround (json_node_get_double (n));
  return (int) json_node_get_int (n);
}

static double
real (JsonNode *root, const char *path)
{
  JsonNode *n = at (root, path);
  if (!n || !JSON_NODE_HOLDS_VALUE (n))
    return 0;
  return json_node_get_double (n);
}

static void
quad (JsonNode *root, const char *path, double out[4])
{
  JsonNode *n = at (root, path);
  for (int i = 0; i < 4; i++)
    out[i] = 0;
  if (!n || !JSON_NODE_HOLDS_ARRAY (n))
    return;
  JsonArray *a = json_node_get_array (n);
  for (guint i = 0; i < 4 && i < json_array_get_length (a); i++)
    out[i] = json_array_get_double_element (a, i);
}

static void
border (JsonNode *root, const char *path, OmarchyBorder *out)
{
  memset (out, 0, sizeof *out);
  JsonNode *n = at (root, path);
  if (!n || !JSON_NODE_HOLDS_OBJECT (n))
    return;
  color (n, "color", &out->color);
  JsonNode *w = at (n, "widths");
  if (w && JSON_NODE_HOLDS_ARRAY (w)) {
    JsonArray *a = json_node_get_array (w);
    for (guint i = 0; i < 4 && i < json_array_get_length (a); i++)
      out->widths[i] = (float) json_array_get_double_element (a, i);
  }
  JsonNode *g = at (n, "gradient");
  if (g && JSON_NODE_HOLDS_OBJECT (g)) {
    JsonNode *cs = at (g, "colors");
    if (cs && JSON_NODE_HOLDS_ARRAY (cs)) {
      JsonArray *a = json_node_get_array (cs);
      guint len = MIN (json_array_get_length (a), OMARCHY_MAX_STOPS);
      for (guint i = 0; i < len; i++)
        parse_hex (json_array_get_string_element (a, i), &out->stops[i]);
      out->n_stops = (int) len;
    }
    out->angle = real (g, "angle");
  }
}

static char *
string (JsonNode *root, const char *path, const char *fallback)
{
  JsonNode *n = at (root, path);
  if (n && JSON_NODE_HOLDS_VALUE (n) && json_node_get_value_type (n) == G_TYPE_STRING)
    return g_strdup (json_node_get_string (n));
  return g_strdup (fallback);
}

static void
state (JsonNode *root, const char *name, OmarchyStateLook *out)
{
  char path[64];
  g_snprintf (path, sizeof path, "controls.%s.fill", name);
  color (root, path, &out->fill);
  g_snprintf (path, sizeof path, "controls.%s.text", name);
  color (root, path, &out->text);
  g_snprintf (path, sizeof path, "controls.%s.border", name);
  border (root, path, &out->border);
}

static void
surface (JsonNode *root, const char *name, OmarchySurfaceLook *out)
{
  char path[64];
  g_snprintf (path, sizeof path, "surfaces.%s.background", name);
  color (root, path, &out->background);
  g_snprintf (path, sizeof path, "surfaces.%s.text", name);
  color (root, path, &out->text);
  g_snprintf (path, sizeof path, "surfaces.%s.border", name);
  border (root, path, &out->border);
}

gboolean
omarchy_tokens_parse (const char *json, OmarchyTokens *out)
{
  if (!json)
    return FALSE;
  g_autoptr (JsonNode) root = json_from_string (json, NULL);
  if (!root || !JSON_NODE_HOLDS_OBJECT (root))
    return FALSE;

  OmarchyTokens t = { 0 };
  g_autofree char *mode = string (root, "mode", "dark");
  t.dark = g_strcmp0 (mode, "light") != 0;
  color (root, "palette.foreground", &t.foreground);
  color (root, "palette.background", &t.background);
  color (root, "palette.accent", &t.accent);
  color (root, "palette.urgent", &t.urgent);
  color (root, "palette.muted", &t.muted);
  color (root, "palette.success", &t.success);
  color (root, "palette.warning", &t.warning);
  color (root, "roles.textDim", &t.text_dim);
  color (root, "roles.textFaint", &t.text_faint);
  color (root, "roles.track", &t.track);
  color (root, "roles.scrim", &t.scrim);
  t.radius = MAX (0, integer (root, "radius"));
  t.gaps_out = integer (root, "gapsOut");
  JsonNode *rm = at (root, "reduceMotion");
  t.reduce_motion = rm && JSON_NODE_HOLDS_VALUE (rm) && json_node_get_boolean (rm);

  t.family = string (root, "font.family", "monospace");
  t.resolved_family = string (root, "font.resolvedFamily", t.family);
#define F(field, key) t.field = integer (root, "font." key)
  F (base_size, "baseSize");
  F (caption, "caption");
  F (body_small, "bodySmall");
  F (body, "body");
  F (subtitle, "subtitle");
  F (title, "title");
  F (heading, "heading");
  F (display, "display");
  F (display_large, "displayLarge");
  F (icon_small, "iconSmall");
  F (icon, "icon");
  F (icon_large, "iconLarge");
#undef F

  t.spacing_scale = real (root, "spacing.scale");
#define S(field, key) t.field = integer (root, "spacing." key)
  S (hairline, "hairline");
  S (xxs, "xxs");
  S (xs, "xs");
  S (sm, "sm");
  S (md, "md");
  S (lg, "lg");
  S (xl, "xl");
  S (xxl, "xxl");
  S (xxxl, "xxxl");
  S (huge, "huge");
  S (control_gap, "controlGap");
  S (control_padding_x, "controlPaddingX");
  S (control_padding_y, "controlPaddingY");
  S (input_padding_y, "inputPaddingY");
  S (control_height, "controlHeight");
  S (popup_row_height, "popupRowHeight");
  S (row_gap, "rowGap");
  S (row_padding_x, "rowPaddingX");
  S (label_gap, "labelGap");
  S (panel_gap, "panelGap");
  S (panel_padding, "panelPadding");
  S (popup_padding, "popupPadding");
  S (dropdown_width, "dropdownWidth");
  S (searchable_dropdown_width, "searchableDropdownWidth");
  S (number_field_width, "numberFieldWidth");
  S (searchable_popup_min_height, "searchablePopupMinHeight");
  S (meter_thickness, "meterThickness");
  S (scrollbar_thickness, "scrollbarThickness");
#undef S

  state (root, "normal", &t.normal);
  state (root, "hover", &t.hover);
  state (root, "focus", &t.focus);
  state (root, "selected", &t.selected);
  color (root, "controls.pressed.fill", &t.pressed);
  color (root, "controls.selection.fill", &t.selection);

  surface (root, "popups", &t.popups);
  surface (root, "tooltip", &t.tooltip);
  surface (root, "menu", &t.menu);
  surface (root, "notifications", &t.notifications);
  surface (root, "polkit", &t.polkit);
  color (root, "surfaces.menu.scrim", &t.menu_scrim);
  color (root, "surfaces.menu.selectedBackground", &t.menu_selected_background);
  color (root, "surfaces.menu.selectedText", &t.menu_selected_text);
  border (root, "surfaces.menu.selectedBorder", &t.menu_selected_border);
  color (root, "surfaces.notifications.countdown", &t.notifications_countdown);
  color (root, "surfaces.polkit.textError", &t.polkit_text_error);
  color (root, "surfaces.polkit.accent", &t.polkit_accent);
  color (root, "surfaces.polkit.scrim", &t.polkit_scrim);
  border (root, "surfaces.polkit.borderError", &t.polkit_border_error);

  t.motion_fast = integer (root, "motion.fast");
  t.motion_press = integer (root, "motion.press");
  t.motion_theme = integer (root, "motion.theme");
  quad (root, "motion.easeOut", t.ease_out);
  quad (root, "motion.ease", t.ease);

  omarchy_tokens_clear (out);
  *out = t;
  return TRUE;
}

void
omarchy_tokens_clear (OmarchyTokens *t)
{
  g_clear_pointer (&t->family, g_free);
  g_clear_pointer (&t->resolved_family, g_free);
}

void
omarchy_tokens_copy (OmarchyTokens *dst, const OmarchyTokens *src)
{
  if (dst == src)
    return;
  omarchy_tokens_clear (dst);
  *dst = *src;
  dst->family = g_strdup (src->family);
  dst->resolved_family = g_strdup (src->resolved_family);
}

static GdkRGBA
lerp_rgba (const GdkRGBA *a, const GdkRGBA *b, double f)
{
  if (f >= 1)
    return *b;
  return (GdkRGBA) {
    a->red + (b->red - a->red) * f,
    a->green + (b->green - a->green) * f,
    a->blue + (b->blue - a->blue) * f,
    a->alpha + (b->alpha - a->alpha) * f,
  };
}

void
omarchy_tokens_lerp (OmarchyTokens *dst, const OmarchyTokens *a, const OmarchyTokens *b, double f)
{
  OmarchyTokens out;
  memcpy (&out, b, sizeof out);
#define LERP(path, field) out.field = lerp_rgba (&a->field, &b->field, f);
  OMARCHY_TOKEN_COLORS (LERP)
#undef LERP
#define STOPS(path, field)                                                    \
  if (a->field.n_stops == b->field.n_stops)                                   \
    for (int i = 0; i < b->field.n_stops; i++)                                \
      out.field.stops[i] = lerp_rgba (&a->field.stops[i], &b->field.stops[i], f);
  OMARCHY_TOKEN_BORDERS (STOPS)
#undef STOPS
  /* The strings stay owned by b; dst gets copies. */
  out.family = b->family;
  out.resolved_family = b->resolved_family;
  if (dst == b) {
    memcpy (dst, &out, sizeof out);
    return;
  }
  OmarchyTokens tmp = out;
  tmp.family = g_strdup (b->family);
  tmp.resolved_family = g_strdup (b->resolved_family);
  omarchy_tokens_clear (dst);
  *dst = tmp;
}

const GdkRGBA *
omarchy_tokens_color_at (const OmarchyTokens *t, const char *path)
{
#define AT(p, field) if (g_strcmp0 (path, p) == 0) return &t->field;
  OMARCHY_TOKEN_COLORS (AT)
#undef AT
  return NULL;
}

const OmarchyBorder *
omarchy_tokens_border_at (const OmarchyTokens *t, const char *path)
{
#define AT(p, field) if (g_strcmp0 (path, p) == 0) return &t->field;
  OMARCHY_TOKEN_BORDERS (AT)
#undef AT
  return NULL;
}

gboolean
omarchy_tokens_same_colors (const OmarchyTokens *a, const OmarchyTokens *b)
{
#define SAME(p, field) if (!gdk_rgba_equal (&a->field, &b->field)) return FALSE;
  OMARCHY_TOKEN_COLORS (SAME)
#undef SAME
  return TRUE;
}

/* ---- colors ------------------------------------------------------------------------------ */

static long
q_round (float d)
{
  return d >= 0 ? (long) (d + 0.5f) : (long) (d - 0.5f);
}

GdkRGBA
omarchy_rgba_darker (const GdkRGBA *c, double factor_f)
{
  long factor = lround (factor_f * 100.0);
  if (factor < 100)
    return *c;
  const float MAX = 65535.0f;
  int r8 = (int) lroundf (c->red * 255), g8 = (int) lroundf (c->green * 255), b8 = (int) lroundf (c->blue * 255);
  float r = r8 * 257.0f / MAX, g = g8 * 257.0f / MAX, b = b8 * 257.0f / MAX;
  float max = fmaxf (r, fmaxf (g, b)), min = fminf (r, fminf (g, b));
  float delta = max - min;
  unsigned value = (unsigned) q_round (max * MAX);
  long hue, sat;
#define FUZZY_EQ(x, y) (fabsf ((x) - (y)) * 100000.0f <= fminf (fabsf (x), fabsf (y)))
  if (fabsf (delta) <= 0.00001f) {
    hue = 65535;
    sat = 0;
  } else {
    sat = q_round ((delta / max) * MAX);
    float h;
    if (FUZZY_EQ (r, max))
      h = (g - b) / delta;
    else if (FUZZY_EQ (g, max))
      h = 2.0f + (b - r) / delta;
    else
      h = 4.0f + (r - g) / delta;
    h *= 60.0f;
    if (h < 0)
      h += 360.0f;
    hue = q_round (h * 100.0f);
  }
#undef FUZZY_EQ
  unsigned v16 = (value * 100) / (unsigned) factor;
  float h = hue == 65535 ? 0.0f : hue / 6000.0f;
  float s = sat / MAX;
  float v = v16 / MAX;
  float rr, gg, bb;
  if (s == 0.0f) {
    rr = gg = bb = v;
  } else {
    int i = (int) h;
    float f = h - i;
    float p = v * (1.0f - s);
    if (i & 1) {
      float q = v * (1.0f - s * f);
      if (i == 1) { rr = q; gg = v; bb = p; }
      else if (i == 3) { rr = p; gg = q; bb = v; }
      else { rr = v; gg = p; bb = q; }
    } else {
      float t = v * (1.0f - s * (1.0f - f));
      if (i == 0) { rr = v; gg = t; bb = p; }
      else if (i == 2) { rr = p; gg = v; bb = t; }
      else { rr = t; gg = p; bb = v; }
    }
  }
#define BYTE(x) (round (CLAMP (q_round ((x) * MAX), 0, 65535) / 65535.0 * 255.0) / 255.0)
  return (GdkRGBA) { BYTE (rr), BYTE (gg), BYTE (bb), c->alpha };
#undef BYTE
}

GdkRGBA
omarchy_rgba_alpha (const GdkRGBA *c, double alpha)
{
  return (GdkRGBA) { c->red, c->green, c->blue, CLAMP (alpha, 0, 1) };
}

GdkRGBA
omarchy_rgba_mix (const GdkRGBA *a, const GdkRGBA *b, double f)
{
  return lerp_rgba (a, b, CLAMP (f, 0, 1));
}

char *
omarchy_rgba_hex (const GdkRGBA *c)
{
  return g_strdup_printf ("#%02x%02x%02x%02x", (int) lround (c->red * 255), (int) lround (c->green * 255),
                          (int) lround (c->blue * 255), (int) lround (c->alpha * 255));
}

void
omarchy_rgba_css (const GdkRGBA *c, char *buf, gsize len)
{
  char a[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_formatd (a, sizeof a, "%.4f", CLAMP (c->alpha, 0, 1));
  g_snprintf (buf, len, "rgba(%d,%d,%d,%s)", (int) lround (CLAMP (c->red, 0, 1) * 255),
              (int) lround (CLAMP (c->green, 0, 1) * 255), (int) lround (CLAMP (c->blue, 0, 1) * 255), a);
}

/* ---- curves ------------------------------------------------------------------------------ */

static double
bez (double a, double b, double t)
{
  /* One axis of a cubic Bézier from (0,0) to (1,1) with control values a, b. */
  double u = 1 - t;
  return 3 * u * u * t * a + 3 * u * t * t * b + t * t * t;
}

double
omarchy_cubic_bezier (const double p[4], double x)
{
  if (x <= 0)
    return 0;
  if (x >= 1)
    return 1;
  /* Solve bez(p0, p2, t) = x by bisection, then evaluate y. */
  double lo = 0, hi = 1, t = x;
  for (int i = 0; i < 40; i++) {
    t = (lo + hi) / 2;
    double v = bez (p[0], p[2], t);
    if (v < x)
      lo = t;
    else
      hi = t;
  }
  return bez (p[1], p[3], t);
}
