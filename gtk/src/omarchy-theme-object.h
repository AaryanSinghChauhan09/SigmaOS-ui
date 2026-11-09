/* omarchy-theme-object.h: OmarchyTheme, the live Omarchy theme as a GObject. */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_THEME (omarchy_theme_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTheme, omarchy_theme, OMARCHY, THEME, GObject)

OmarchyTheme *omarchy_theme_get_default (void);

guint64 omarchy_theme_get_revision (OmarchyTheme *self);
const char *omarchy_theme_get_tokens_json (OmarchyTheme *self);
GVariant *omarchy_theme_get_tokens (OmarchyTheme *self);
gboolean omarchy_theme_get_fading (OmarchyTheme *self);
gboolean omarchy_theme_get_dark (OmarchyTheme *self);
gboolean omarchy_theme_get_reduce_motion (OmarchyTheme *self);
int omarchy_theme_get_radius (OmarchyTheme *self);
int omarchy_theme_get_radius_small (OmarchyTheme *self);
const char *omarchy_theme_get_font_family (OmarchyTheme *self);

GdkRGBA *omarchy_theme_get_color (OmarchyTheme *self, const char *path);
GdkRGBA *omarchy_theme_get_paint_color (OmarchyTheme *self, const char *path);
double omarchy_theme_get_number (OmarchyTheme *self, const char *path);
int omarchy_theme_get_font_size (OmarchyTheme *self, const char *token);
int omarchy_theme_get_spacing (OmarchyTheme *self, const char *token);
int omarchy_theme_space (OmarchyTheme *self, double px);
PangoFontDescription *omarchy_theme_get_font (OmarchyTheme *self, const char *token, gboolean bold);

const char *omarchy_theme_get_theme_name (OmarchyTheme *self);
const char *omarchy_theme_get_icon_theme (OmarchyTheme *self);
char *omarchy_theme_lookup_icon (OmarchyTheme *self, const char *name, int size);
GFile *omarchy_theme_lookup_icon_file (OmarchyTheme *self, const char *name, int size);
const char *omarchy_theme_get_language (OmarchyTheme *self);
const char *omarchy_theme_translate (OmarchyTheme *self, const char *key, const char *fallback);
char *omarchy_theme_format (OmarchyTheme *self, const char *key, const char *fallback, const char *arg);

G_END_DECLS
