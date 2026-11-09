/* omarchy-theme-object.c: OmarchyTheme, following the desktop through libomarchy-theme's main-loop
 * watch (spec/following.md): the changed hook, the revision and the color fade. */
#include "omarchy-private.h"

#include <glib-unix.h>
#include <math.h>
#include <omarchy-theme.h>

/**
 * OmarchyTheme:
 *
 * The live Omarchy theme: the tokens of `spec/tokens.md`, resolved by libomarchy-theme and
 * followed through its watch on the GLib main loop.
 *
 * There is one, [func@Omarchy.Theme.get_default]. [func@Omarchy.init] makes it and restyles
 * GTK from it; the kit's widgets draw from it. An app reads tokens from it, and connects to
 * [signal@Omarchy.Theme::changed] for what the kit can't reach (a custom renderer, a cache).
 *
 * Accessors return the target tokens: during the theme-switch fade they already read the new
 * theme. [method@Omarchy.Theme.get_paint_color] returns the color on screen mid-fade.
 */

struct _OmarchyTheme {
  GObject parent_instance;

  OmarchyThemeWatch *watch;
  guint watch_source;
  guint64 revision;

  char *json;
  JsonNode *root;
  GVariant *variant;
  OmarchyTokens tokens; /* the target */
  OmarchyTokens paint;  /* on screen: the target, or a frame of the fade */
  OmarchyTokens from;   /* where the fade started */
  gint64 fade_start;
  guint fade_source;

  char *theme_name;
  char *icon_theme;
  char *language;
  GHashTable *strings;
};

enum {
  PROP_0,
  PROP_REVISION,
  PROP_TOKENS_JSON,
  PROP_FADING,
  PROP_DARK,
  PROP_REDUCE_MOTION,
  PROP_RADIUS,
  PROP_THEME_NAME,
  PROP_ICON_THEME,
  PROP_LANGUAGE,
  N_PROPS
};

enum { SIGNAL_CHANGED, SIGNAL_REPAINT, SIGNAL_RESTYLE, N_SIGNALS };

static GParamSpec *props[N_PROPS];
static guint signals[N_SIGNALS];
static OmarchyTheme *default_theme;

G_DEFINE_FINAL_TYPE (OmarchyTheme, omarchy_theme, G_TYPE_OBJECT)

static char *
take_string (char *s)
{
  char *out = g_strdup (s);
  omarchy_theme_free (s);
  return out;
}

static void
load_strings (OmarchyTheme *self)
{
  g_clear_pointer (&self->strings, g_hash_table_unref);
  g_clear_pointer (&self->language, g_free);
  self->strings = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_autofree char *json = take_string (omarchy_theme_strings_json ());
  g_autoptr (JsonNode) root = json ? json_from_string (json, NULL) : NULL;
  if (root && JSON_NODE_HOLDS_OBJECT (root)) {
    JsonObject *o = json_node_get_object (root);
    if (json_object_has_member (o, "language"))
      self->language = g_strdup (json_object_get_string_member (o, "language"));
    JsonObject *s = json_object_has_member (o, "strings") ? json_object_get_object_member (o, "strings") : NULL;
    if (s) {
      JsonObjectIter it;
      const char *k;
      JsonNode *v;
      json_object_iter_init (&it, s);
      while (json_object_iter_next (&it, &k, &v))
        if (JSON_NODE_HOLDS_VALUE (v))
          g_hash_table_insert (self->strings, g_strdup (k), g_strdup (json_node_get_string (v)));
    }
  }
  if (!self->language)
    self->language = g_strdup ("en");
}

static void
load_names (OmarchyTheme *self)
{
  g_free (self->theme_name);
  g_free (self->icon_theme);
  self->theme_name = take_string (omarchy_theme_name ());
  self->icon_theme = take_string (omarchy_theme_icon_theme ());
  load_strings (self);
}

static gboolean
take_json (OmarchyTheme *self, const char *json)
{
  OmarchyTokens t = { 0 };
  if (!omarchy_tokens_parse (json, &t))
    return FALSE;
  omarchy_tokens_copy (&self->tokens, &t);
  omarchy_tokens_clear (&t);
  g_free (self->json);
  self->json = g_strdup (json);
  g_clear_pointer (&self->root, json_node_unref);
  self->root = json_from_string (json, NULL);
  g_clear_pointer (&self->variant, g_variant_unref);
  self->variant = json_gvariant_deserialize (self->root, NULL, NULL);
  if (self->variant)
    g_variant_ref_sink (self->variant);
  return TRUE;
}

static void
stop_fade (OmarchyTheme *self)
{
  if (self->fade_source) {
    g_source_remove (self->fade_source);
    self->fade_source = 0;
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FADING]);
  }
}

static gboolean
fade_tick (gpointer data)
{
  OmarchyTheme *self = data;
  double duration = MAX (1, self->tokens.motion_theme) * 1000.0;
  double x = (g_get_monotonic_time () - self->fade_start) / duration;
  if (x >= 1) {
    /* Land exactly on the tokens, with no interpolation drift. */
    omarchy_tokens_copy (&self->paint, &self->tokens);
    self->fade_source = 0;
    omarchy_css_update_colors (&self->paint);
    g_signal_emit (self, signals[SIGNAL_REPAINT], 0);
    g_object_notify_by_pspec (G_OBJECT (self), props[PROP_FADING]);
    return G_SOURCE_REMOVE;
  }
  omarchy_tokens_lerp (&self->paint, &self->from, &self->tokens, omarchy_cubic_bezier (self->tokens.ease, x));
  omarchy_css_update_colors (&self->paint);
  g_signal_emit (self, signals[SIGNAL_REPAINT], 0);
  return G_SOURCE_CONTINUE;
}

/* A delivered change (spec/following.md): the new tokens take effect, layout switches at once,
 * the hook runs with them, then the colors fade from what was on screen. */
static void
apply_change (OmarchyTheme *self, const char *json)
{
  g_autoptr (GObject) hold = g_object_ref (G_OBJECT (self));
  OmarchyTokens on_screen = { 0 };
  omarchy_tokens_copy (&on_screen, &self->paint);
  gboolean was_dark = self->tokens.dark;
  int was_radius = self->tokens.radius;
  gboolean was_reduced = self->tokens.reduce_motion;

  if (!take_json (self, json)) {
    omarchy_tokens_clear (&on_screen);
    return;
  }
  load_names (self);
  self->revision = omarchy_theme_watch_revision (self->watch);

  stop_fade (self);
  gboolean fade = !self->tokens.reduce_motion && self->tokens.motion_theme > 0
                  && !omarchy_tokens_same_colors (&on_screen, &self->tokens);
  if (fade) {
    /* Layout lands now; the colors on screen stay until the fade moves them. */
    omarchy_tokens_copy (&self->from, &on_screen);
    omarchy_tokens_lerp (&self->paint, &self->from, &self->tokens, 0);
  } else {
    omarchy_tokens_copy (&self->paint, &self->tokens);
  }
  omarchy_tokens_clear (&on_screen);

  omarchy_css_update_structure (&self->tokens);
  omarchy_css_update_colors (&self->paint);
  g_signal_emit (self, signals[SIGNAL_RESTYLE], 0);

  GObject *o = G_OBJECT (self);
  g_object_freeze_notify (o);
  g_object_notify_by_pspec (o, props[PROP_REVISION]);
  g_object_notify_by_pspec (o, props[PROP_TOKENS_JSON]);
  g_object_notify_by_pspec (o, props[PROP_THEME_NAME]);
  g_object_notify_by_pspec (o, props[PROP_ICON_THEME]);
  g_object_notify_by_pspec (o, props[PROP_LANGUAGE]);
  if (was_dark != self->tokens.dark)
    g_object_notify_by_pspec (o, props[PROP_DARK]);
  if (was_radius != self->tokens.radius)
    g_object_notify_by_pspec (o, props[PROP_RADIUS]);
  if (was_reduced != self->tokens.reduce_motion)
    g_object_notify_by_pspec (o, props[PROP_REDUCE_MOTION]);
  g_object_thaw_notify (o);

  g_signal_emit (self, signals[SIGNAL_CHANGED], 0, self->variant);

  if (fade) {
    self->fade_start = g_get_monotonic_time ();
    self->fade_source = g_timeout_add_full (G_PRIORITY_HIGH_IDLE + 10, 8, fade_tick, self, NULL);
    g_source_set_name_by_id (self->fade_source, "[omarchy-gtk] theme fade");
    g_object_notify_by_pspec (o, props[PROP_FADING]);
  }
  g_signal_emit (self, signals[SIGNAL_REPAINT], 0);
}

static gboolean
on_watch (int fd, GIOCondition cond, gpointer data)
{
  OmarchyTheme *self = data;
  char *json = omarchy_theme_watch_take (self->watch);
  if (json) {
    g_autofree char *copy = g_strdup (json);
    omarchy_theme_free (json);
    apply_change (self, copy);
  }
  return G_SOURCE_CONTINUE;
}

static void
omarchy_theme_init (OmarchyTheme *self)
{
  /* The watch first, then the starting tokens from it, so no change falls in between. */
  self->watch = omarchy_theme_watch_new ();
  char *json = self->watch ? omarchy_theme_watch_current_json (self->watch) : omarchy_theme_tokens_json ();
  g_autofree char *copy = g_strdup (json);
  omarchy_theme_free (json);
  if (!take_json (self, copy))
    g_warning ("omarchy-gtk: libomarchy-theme gave no tokens");
  omarchy_tokens_copy (&self->paint, &self->tokens);
  load_names (self);
  if (self->watch && omarchy_theme_watch_fd (self->watch) >= 0) {
    self->watch_source = g_unix_fd_add (omarchy_theme_watch_fd (self->watch), G_IO_IN, on_watch, self);
    g_source_set_name_by_id (self->watch_source, "[omarchy-gtk] theme watch");
  }
}

static void
omarchy_theme_finalize (GObject *object)
{
  OmarchyTheme *self = OMARCHY_THEME (object);
  stop_fade (self);
  g_clear_handle_id (&self->watch_source, g_source_remove);
  g_clear_pointer (&self->watch, omarchy_theme_watch_free);
  g_clear_pointer (&self->json, g_free);
  g_clear_pointer (&self->root, json_node_unref);
  g_clear_pointer (&self->variant, g_variant_unref);
  omarchy_tokens_clear (&self->tokens);
  omarchy_tokens_clear (&self->paint);
  omarchy_tokens_clear (&self->from);
  g_clear_pointer (&self->theme_name, g_free);
  g_clear_pointer (&self->icon_theme, g_free);
  g_clear_pointer (&self->language, g_free);
  g_clear_pointer (&self->strings, g_hash_table_unref);
  G_OBJECT_CLASS (omarchy_theme_parent_class)->finalize (object);
}

static void
omarchy_theme_get_property (GObject *object, guint id, GValue *value, GParamSpec *pspec)
{
  OmarchyTheme *self = OMARCHY_THEME (object);
  switch (id) {
  case PROP_REVISION: g_value_set_uint64 (value, self->revision); break;
  case PROP_TOKENS_JSON: g_value_set_string (value, self->json); break;
  case PROP_FADING: g_value_set_boolean (value, self->fade_source != 0); break;
  case PROP_DARK: g_value_set_boolean (value, self->tokens.dark); break;
  case PROP_REDUCE_MOTION: g_value_set_boolean (value, self->tokens.reduce_motion); break;
  case PROP_RADIUS: g_value_set_int (value, self->tokens.radius); break;
  case PROP_THEME_NAME: g_value_set_string (value, self->theme_name); break;
  case PROP_ICON_THEME: g_value_set_string (value, self->icon_theme); break;
  case PROP_LANGUAGE: g_value_set_string (value, self->language); break;
  default: G_OBJECT_WARN_INVALID_PROPERTY_ID (object, id, pspec);
  }
}

static void
omarchy_theme_class_init (OmarchyThemeClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = omarchy_theme_finalize;
  object_class->get_property = omarchy_theme_get_property;

  /**
   * OmarchyTheme:revision:
   *
   * 0 for the tokens loaded first, plus one for each change delivered: a cache key.
   */
  props[PROP_REVISION] = g_param_spec_uint64 ("revision", NULL, NULL, 0, G_MAXUINT64, 0,
                                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:tokens-json:
   *
   * The tokens as the canonical JSON of `spec/tokens.md`, on one line.
   */
  props[PROP_TOKENS_JSON] = g_param_spec_string ("tokens-json", NULL, NULL, NULL,
                                                 G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:fading:
   *
   * Whether the colors are fading from one theme to the next right now.
   */
  props[PROP_FADING] = g_param_spec_boolean ("fading", NULL, NULL, FALSE,
                                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:dark:
   *
   * Whether the theme is dark (`mode` is not `"light"`).
   */
  props[PROP_DARK] = g_param_spec_boolean ("dark", NULL, NULL, TRUE,
                                           G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:reduce-motion:
   *
   * Hyprland's animations are off (or `OMARCHY_UI_REDUCE_MOTION=1`): nothing animates.
   */
  props[PROP_REDUCE_MOTION] = g_param_spec_boolean ("reduce-motion", NULL, NULL, FALSE,
                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:radius:
   *
   * Hyprland's rounding, for panels, cards and controls (usually 0).
   */
  props[PROP_RADIUS] = g_param_spec_int ("radius", NULL, NULL, 0, G_MAXINT, 0,
                                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:theme-name:
   *
   * The Omarchy theme's name (`tokyo-night`).
   */
  props[PROP_THEME_NAME] = g_param_spec_string ("theme-name", NULL, NULL, NULL,
                                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:icon-theme:
   *
   * The freedesktop icon theme the Omarchy theme names (`Yaru-blue` by default).
   */
  props[PROP_ICON_THEME] = g_param_spec_string ("icon-theme", NULL, NULL, NULL,
                                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * OmarchyTheme:language:
   *
   * The display language the kit's strings are in (`en`, `de`).
   */
  props[PROP_LANGUAGE] = g_param_spec_string ("language", NULL, NULL, NULL,
                                              G_PARAM_READABLE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);
  g_object_class_install_properties (object_class, N_PROPS, props);

  /**
   * OmarchyTheme::changed:
   * @self: the theme
   * @tokens: the new tokens, in the shape of the canonical JSON (an `a{sv}` dictionary)
   *
   * The changed hook (spec/following.md): once per real change, never for the initial load,
   * on the main thread, after the new tokens are in effect (every accessor reads them), with
   * the target tokens (never a frame of the fade). Re-read the icon theme and strings here
   * too: a change can be to those alone.
   */
  signals[SIGNAL_CHANGED] = g_signal_new ("changed", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                          NULL, G_TYPE_NONE, 1, G_TYPE_VARIANT);
  /**
   * OmarchyTheme::repaint:
   * @self: the theme
   *
   * The colors on screen moved: once after each change, then on every frame of the fade.
   * Custom drawing that follows the fade reads [method@Omarchy.Theme.get_paint_color] and
   * queues a draw here.
   */
  signals[SIGNAL_REPAINT] = g_signal_new ("repaint", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                          NULL, G_TYPE_NONE, 0);
  /**
   * OmarchyTheme::restyle:
   * @self: the theme
   *
   * Sizes, rounding or fonts may have changed (just before ::changed): relayout.
   */
  signals[SIGNAL_RESTYLE] = g_signal_new ("restyle", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL,
                                          NULL, G_TYPE_NONE, 0);
}

/**
 * omarchy_theme_get_default:
 *
 * The theme, made (and following the desktop) on first use.
 *
 * Returns: (transfer none): the theme
 */
OmarchyTheme *
omarchy_theme_get_default (void)
{
  if (!default_theme)
    default_theme = g_object_new (OMARCHY_TYPE_THEME, NULL);
  return default_theme;
}

const OmarchyTokens *
omarchy_tokens (void)
{
  return &omarchy_theme_get_default ()->tokens;
}

const OmarchyTokens *
omarchy_paint (void)
{
  return &omarchy_theme_get_default ()->paint;
}

/**
 * omarchy_theme_get_revision:
 * @self: the theme
 *
 * Returns: 0 for the tokens loaded first, plus one per change: a cache key
 */
guint64
omarchy_theme_get_revision (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), 0);
  return self->revision;
}

/**
 * omarchy_theme_get_tokens_json:
 * @self: the theme
 *
 * Returns: (transfer none): the tokens as canonical JSON on one line
 */
const char *
omarchy_theme_get_tokens_json (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return self->json;
}

/**
 * omarchy_theme_get_tokens:
 * @self: the theme
 *
 * Returns: (transfer none): the tokens as an `a{sv}` dictionary in the canonical JSON's shape
 */
GVariant *
omarchy_theme_get_tokens (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return self->variant;
}

/**
 * omarchy_theme_get_fading:
 * @self: the theme
 *
 * Returns: whether the theme-switch fade is running
 */
gboolean
omarchy_theme_get_fading (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), FALSE);
  return self->fade_source != 0;
}

/**
 * omarchy_theme_get_dark:
 * @self: the theme
 *
 * Returns: whether the theme is dark
 */
gboolean
omarchy_theme_get_dark (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), TRUE);
  return self->tokens.dark;
}

/**
 * omarchy_theme_get_reduce_motion:
 * @self: the theme
 *
 * Returns: whether motion is reduced (every duration 0)
 */
gboolean
omarchy_theme_get_reduce_motion (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), FALSE);
  return self->tokens.reduce_motion;
}

/**
 * omarchy_theme_get_radius:
 * @self: the theme
 *
 * Returns: Hyprland's rounding
 */
int
omarchy_theme_get_radius (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), 0);
  return self->tokens.radius;
}

/**
 * omarchy_theme_get_radius_small:
 * @self: the theme
 *
 * Returns: the rounding for small parts, `min(4, radius)`
 */
int
omarchy_theme_get_radius_small (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), 0);
  return MIN (4, self->tokens.radius);
}

/**
 * omarchy_theme_get_font_family:
 * @self: the theme
 *
 * Returns: (transfer none): the family fontconfig's `monospace` resolves to now
 */
const char *
omarchy_theme_get_font_family (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return self->tokens.resolved_family;
}

/**
 * omarchy_theme_get_color:
 * @self: the theme
 * @path: a color's path in the canonical JSON: `palette.background`, `roles.textDim`,
 *   `controls.hover.fill`, `surfaces.popups.border.color`
 *
 * Returns: (transfer full) (nullable): the color (the target, during a fade), or %NULL for a
 *   path that names no color
 */
GdkRGBA *
omarchy_theme_get_color (OmarchyTheme *self, const char *path)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  const GdkRGBA *c = omarchy_tokens_color_at (&self->tokens, path);
  return c ? gdk_rgba_copy (c) : NULL;
}

/**
 * omarchy_theme_get_paint_color:
 * @self: the theme
 * @path: a color's path, as for [method@Omarchy.Theme.get_color]
 *
 * Returns: (transfer full) (nullable): the color on screen: mid-fade, a frame between the two
 *   themes
 */
GdkRGBA *
omarchy_theme_get_paint_color (OmarchyTheme *self, const char *path)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  const GdkRGBA *c = omarchy_tokens_color_at (&self->paint, path);
  return c ? gdk_rgba_copy (c) : NULL;
}

static JsonNode *
node_at (JsonNode *root, const char *path)
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

/**
 * omarchy_theme_get_number:
 * @self: the theme
 * @path: a number's path in the canonical JSON: `radius`, `spacing.controlGap`, `font.body`,
 *   `motion.theme`
 *
 * Returns: the number, or 0 for a path that names none
 */
double
omarchy_theme_get_number (OmarchyTheme *self, const char *path)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), 0);
  JsonNode *n = self->root ? node_at (self->root, path) : NULL;
  if (!n || !JSON_NODE_HOLDS_VALUE (n))
    return 0;
  GType t = json_node_get_value_type (n);
  if (t == G_TYPE_INT64 || t == G_TYPE_DOUBLE)
    return json_node_get_double (n);
  if (t == G_TYPE_BOOLEAN)
    return json_node_get_boolean (n);
  return 0;
}

/**
 * omarchy_theme_get_font_size:
 * @self: the theme
 * @token: a type-scale token: `caption`, `bodySmall`, `body`, `subtitle`, `title`, `heading`,
 *   `display`, `displayLarge`, `iconSmall`, `icon`, `iconLarge`
 *
 * Returns: its size in pixels
 */
int
omarchy_theme_get_font_size (OmarchyTheme *self, const char *token)
{
  g_autofree char *path = g_strconcat ("font.", token, NULL);
  return (int) omarchy_theme_get_number (self, path);
}

/**
 * omarchy_theme_get_spacing:
 * @self: the theme
 * @token: a spacing token: `xs` … `huge`, `controlGap`, `controlHeight`, `panelPadding`, …
 *
 * Returns: its size in pixels, on the theme's spacing scale
 */
int
omarchy_theme_get_spacing (OmarchyTheme *self, const char *token)
{
  g_autofree char *path = g_strconcat ("spacing.", token, NULL);
  return (int) omarchy_theme_get_number (self, path);
}

/**
 * omarchy_theme_space:
 * @self: the theme
 * @px: a size at the default scale
 *
 * `Style.space(px)`: @px on the theme's spacing scale, `max(1, round(px × scale))`.
 *
 * Returns: the scaled size
 */
int
omarchy_theme_space (OmarchyTheme *self, double px)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), (int) px);
  double n = px * self->tokens.spacing_scale;
  return n <= 0 ? 0 : MAX (1, (int) lround (n));
}

/**
 * omarchy_theme_get_font:
 * @self: the theme
 * @token: a type-scale token, as for [method@Omarchy.Theme.get_font_size]
 * @bold: bold (for selection and titles only)
 *
 * Returns: (transfer full): the Omarchy font at that size
 */
PangoFontDescription *
omarchy_theme_get_font (OmarchyTheme *self, const char *token, gboolean bold)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return omarchy_font (omarchy_theme_get_font_size (self, token), bold);
}

/**
 * omarchy_theme_get_theme_name:
 * @self: the theme
 *
 * Returns: (transfer none): the Omarchy theme's name
 */
const char *
omarchy_theme_get_theme_name (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return self->theme_name;
}

/**
 * omarchy_theme_get_icon_theme:
 * @self: the theme
 *
 * Returns: (transfer none): the freedesktop icon theme the Omarchy theme names
 */
const char *
omarchy_theme_get_icon_theme (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return self->icon_theme;
}

/**
 * omarchy_theme_lookup_icon:
 * @self: the theme
 * @name: an icon name (`firefox`, `org.gnome.Nautilus`)
 * @size: the size in device pixels
 *
 * The file @name resolves to in the theme's icon theme, by the lookup Omarchy's launcher uses.
 *
 * Returns: (transfer full) (nullable) (type filename): the path, or %NULL
 */
char *
omarchy_theme_lookup_icon (OmarchyTheme *self, const char *name, int size)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  char *p = omarchy_theme_icon_lookup (name, size);
  return p ? take_string (p) : NULL;
}

/**
 * omarchy_theme_lookup_icon_file:
 * @self: the theme
 * @name: an icon name
 * @size: the size in device pixels
 *
 * Returns: (transfer full) (nullable): the file @name resolves to, or %NULL
 */
GFile *
omarchy_theme_lookup_icon_file (OmarchyTheme *self, const char *name, int size)
{
  g_autofree char *p = omarchy_theme_lookup_icon (self, name, size);
  return p ? g_file_new_for_path (p) : NULL;
}

/**
 * omarchy_theme_get_language:
 * @self: the theme
 *
 * Returns: (transfer none): the display language (`en`)
 */
const char *
omarchy_theme_get_language (OmarchyTheme *self)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), NULL);
  return self->language;
}

/**
 * omarchy_theme_translate:
 * @self: the theme
 * @key: a key in the kit's catalog (`common.cancel`)
 * @fallback: (nullable): what to show when the catalog has no @key
 *
 * The kit's string for @key in the display language, as Qt's `I18n.t(key, english)`.
 *
 * Returns: (transfer none) (nullable): the string, or @fallback
 */
const char *
omarchy_theme_translate (OmarchyTheme *self, const char *key, const char *fallback)
{
  g_return_val_if_fail (OMARCHY_IS_THEME (self), fallback);
  const char *s = self->strings ? g_hash_table_lookup (self->strings, key) : NULL;
  return s && *s ? s : fallback;
}

/**
 * omarchy_theme_format:
 * @self: the theme
 * @key: a key in the kit's catalog
 * @fallback: (nullable): what to use when the catalog has no @key
 * @arg: (nullable): what replaces `%1`
 *
 * Returns: (transfer full): the string with `%1` filled in
 */
char *
omarchy_theme_format (OmarchyTheme *self, const char *key, const char *fallback, const char *arg)
{
  const char *s = omarchy_theme_translate (self, key, fallback);
  if (!s)
    return g_strdup ("");
  g_autoptr (GString) out = g_string_new (s);
  g_string_replace (out, "%1", arg ? arg : "", 0);
  return g_string_free (g_steal_pointer (&out), FALSE);
}

const char *
omarchy_t (const char *key, const char *fallback)
{
  return omarchy_theme_translate (omarchy_theme_get_default (), key, fallback);
}
