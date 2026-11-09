/* omarchy-tokens.h: the canonical token JSON (spec/tokens.md) as a C struct, private to the
 * library. Parsed from libomarchy-theme's JSON, never resolved here. */
#pragma once

#include <gdk/gdk.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

#define OMARCHY_MAX_STOPS 6

/* A border spec: color, per-side widths (top, right, bottom, left) and an optional gradient. */
typedef struct {
  GdkRGBA color;
  float widths[4];
  int n_stops; /* 0 or 1: no gradient */
  GdkRGBA stops[OMARCHY_MAX_STOPS];
  double angle;
} OmarchyBorder;

typedef struct {
  GdkRGBA fill;
  OmarchyBorder border;
  GdkRGBA text;
} OmarchyStateLook;

typedef struct {
  GdkRGBA background;
  GdkRGBA text;
  OmarchyBorder border;
} OmarchySurfaceLook;

typedef struct {
  gboolean dark;
  GdkRGBA foreground, background, accent, urgent, muted, success, warning;
  GdkRGBA text_dim, text_faint, track, scrim;
  int radius, gaps_out;
  gboolean reduce_motion;

  char *family;          /* the family token ("monospace") */
  char *resolved_family; /* what fontconfig resolves it to ("JetBrainsMono Nerd Font") */
  int base_size, caption, body_small, body, subtitle, title, heading, display, display_large;
  int icon_small, icon, icon_large;

  double spacing_scale;
  int hairline, xxs, xs, sm, md, lg, xl, xxl, xxxl, huge;
  int control_gap, control_padding_x, control_padding_y, input_padding_y, control_height;
  int popup_row_height, row_gap, row_padding_x, label_gap, panel_gap, panel_padding, popup_padding;
  int dropdown_width, searchable_dropdown_width, number_field_width, searchable_popup_min_height;
  int meter_thickness, scrollbar_thickness;

  OmarchyStateLook normal, hover, focus, selected;
  GdkRGBA pressed, selection;

  OmarchySurfaceLook popups, tooltip, menu, notifications, polkit;
  GdkRGBA menu_scrim, menu_selected_background, menu_selected_text;
  OmarchyBorder menu_selected_border;
  GdkRGBA notifications_countdown;
  GdkRGBA polkit_text_error, polkit_accent, polkit_scrim;
  OmarchyBorder polkit_border_error;

  int motion_fast, motion_press, motion_theme;
  double ease_out[4], ease[4];
} OmarchyTokens;

/* Every color in the struct, by its canonical JSON path. X(path, field). */
#define OMARCHY_TOKEN_COLORS(X)                                                   \
  X("palette.foreground", foreground)                                             \
  X("palette.background", background)                                             \
  X("palette.accent", accent)                                                     \
  X("palette.urgent", urgent)                                                     \
  X("palette.muted", muted)                                                       \
  X("palette.success", success)                                                   \
  X("palette.warning", warning)                                                   \
  X("roles.textDim", text_dim)                                                    \
  X("roles.textFaint", text_faint)                                                \
  X("roles.track", track)                                                         \
  X("roles.scrim", scrim)                                                         \
  X("controls.normal.fill", normal.fill)                                          \
  X("controls.normal.text", normal.text)                                          \
  X("controls.normal.border.color", normal.border.color)                          \
  X("controls.hover.fill", hover.fill)                                            \
  X("controls.hover.text", hover.text)                                            \
  X("controls.hover.border.color", hover.border.color)                            \
  X("controls.focus.fill", focus.fill)                                            \
  X("controls.focus.text", focus.text)                                            \
  X("controls.focus.border.color", focus.border.color)                            \
  X("controls.selected.fill", selected.fill)                                      \
  X("controls.selected.text", selected.text)                                      \
  X("controls.selected.border.color", selected.border.color)                      \
  X("controls.pressed.fill", pressed)                                             \
  X("controls.selection.fill", selection)                                         \
  X("surfaces.popups.background", popups.background)                              \
  X("surfaces.popups.text", popups.text)                                          \
  X("surfaces.popups.border.color", popups.border.color)                          \
  X("surfaces.tooltip.background", tooltip.background)                            \
  X("surfaces.tooltip.text", tooltip.text)                                        \
  X("surfaces.tooltip.border.color", tooltip.border.color)                        \
  X("surfaces.menu.background", menu.background)                                  \
  X("surfaces.menu.text", menu.text)                                              \
  X("surfaces.menu.border.color", menu.border.color)                              \
  X("surfaces.menu.scrim", menu_scrim)                                            \
  X("surfaces.menu.selectedBackground", menu_selected_background)                 \
  X("surfaces.menu.selectedText", menu_selected_text)                             \
  X("surfaces.menu.selectedBorder.color", menu_selected_border.color)             \
  X("surfaces.notifications.background", notifications.background)                \
  X("surfaces.notifications.text", notifications.text)                            \
  X("surfaces.notifications.border.color", notifications.border.color)            \
  X("surfaces.notifications.countdown", notifications_countdown)                  \
  X("surfaces.polkit.background", polkit.background)                              \
  X("surfaces.polkit.text", polkit.text)                                          \
  X("surfaces.polkit.border.color", polkit.border.color)                          \
  X("surfaces.polkit.textError", polkit_text_error)                               \
  X("surfaces.polkit.borderError.color", polkit_border_error.color)               \
  X("surfaces.polkit.accent", polkit_accent)                                      \
  X("surfaces.polkit.scrim", polkit_scrim)

/* Every border spec (their gradient stops fade too). X(path, field). */
#define OMARCHY_TOKEN_BORDERS(X)                                                  \
  X("controls.normal.border", normal.border)                                      \
  X("controls.hover.border", hover.border)                                        \
  X("controls.focus.border", focus.border)                                        \
  X("controls.selected.border", selected.border)                                  \
  X("surfaces.popups.border", popups.border)                                      \
  X("surfaces.tooltip.border", tooltip.border)                                    \
  X("surfaces.menu.border", menu.border)                                          \
  X("surfaces.menu.selectedBorder", menu_selected_border)                         \
  X("surfaces.notifications.border", notifications.border)                        \
  X("surfaces.polkit.border", polkit.border)                                      \
  X("surfaces.polkit.borderError", polkit_border_error)

/* Parse the canonical JSON. FALSE (and *out untouched) when it isn't. */
gboolean omarchy_tokens_parse (const char *json, OmarchyTokens *out);
void omarchy_tokens_copy (OmarchyTokens *dst, const OmarchyTokens *src);
void omarchy_tokens_clear (OmarchyTokens *t);
/* dst's colors = a + (b - a) * f, per channel (alpha included) in sRGB; everything else b's. */
void omarchy_tokens_lerp (OmarchyTokens *dst, const OmarchyTokens *a, const OmarchyTokens *b, double f);
/* The color at a canonical JSON path ("palette.background"), or NULL. */
const GdkRGBA *omarchy_tokens_color_at (const OmarchyTokens *t, const char *path);
/* The border spec at a canonical JSON path ("controls.hover.border"), or NULL. */
const OmarchyBorder *omarchy_tokens_border_at (const OmarchyTokens *t, const char *path);
/* TRUE when every color in a and b is the same. */
gboolean omarchy_tokens_same_colors (const OmarchyTokens *a, const OmarchyTokens *b);

/* Qt's QColor::darker(factor × 100), exactly as the core and Qt compute it. */
GdkRGBA omarchy_rgba_darker (const GdkRGBA *c, double factor);
GdkRGBA omarchy_rgba_alpha (const GdkRGBA *c, double alpha);
GdkRGBA omarchy_rgba_mix (const GdkRGBA *a, const GdkRGBA *b, double f);
/* "#rrggbbaa" */
char *omarchy_rgba_hex (const GdkRGBA *c);
/* "rgba(r,g,b,a)" for CSS */
void omarchy_rgba_css (const GdkRGBA *c, char *buf, gsize len);

/* The CSS `ease`-family curve at x in [0, 1]. */
double omarchy_cubic_bezier (const double p[4], double x);

G_END_DECLS
