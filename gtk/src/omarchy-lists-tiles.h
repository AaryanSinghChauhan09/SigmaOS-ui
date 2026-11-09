/* omarchy-lists-tiles.h: Badge, StatusDot, Avatar and TileGrid (spec/components/lists.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

#include "omarchy-lists-cursor.h"

G_BEGIN_DECLS

#define OMARCHY_TYPE_BADGE (omarchy_badge_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyBadge, omarchy_badge, OMARCHY, BADGE, GtkWidget)

GtkWidget *omarchy_badge_new (const char *text);
const char *omarchy_badge_get_text (OmarchyBadge *self);
void omarchy_badge_set_text (OmarchyBadge *self, const char *text);
const char *omarchy_badge_get_tone (OmarchyBadge *self);
void omarchy_badge_set_tone (OmarchyBadge *self, const char *tone);
gboolean omarchy_badge_get_filled (OmarchyBadge *self);
void omarchy_badge_set_filled (OmarchyBadge *self, gboolean filled);

#define OMARCHY_TYPE_STATUS_DOT (omarchy_status_dot_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyStatusDot, omarchy_status_dot, OMARCHY, STATUS_DOT, GtkWidget)

GtkWidget *omarchy_status_dot_new (const char *tone);
const char *omarchy_status_dot_get_tone (OmarchyStatusDot *self);
void omarchy_status_dot_set_tone (OmarchyStatusDot *self, const char *tone);
int omarchy_status_dot_get_size (OmarchyStatusDot *self);
void omarchy_status_dot_set_size (OmarchyStatusDot *self, int size);
int omarchy_status_dot_get_ring (OmarchyStatusDot *self);
void omarchy_status_dot_set_ring (OmarchyStatusDot *self, int ring);
const char *omarchy_status_dot_get_ground (OmarchyStatusDot *self);
void omarchy_status_dot_set_ground (OmarchyStatusDot *self, const char *ground);
void omarchy_status_dot_set_ground_color (OmarchyStatusDot *self, const GdkRGBA *color);

#define OMARCHY_TYPE_AVATAR (omarchy_avatar_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyAvatar, omarchy_avatar, OMARCHY, AVATAR, GtkWidget)

GtkWidget *omarchy_avatar_new (const char *name);
const char *omarchy_avatar_get_name (OmarchyAvatar *self);
void omarchy_avatar_set_name (OmarchyAvatar *self, const char *name);
const char *omarchy_avatar_get_source (OmarchyAvatar *self);
void omarchy_avatar_set_source (OmarchyAvatar *self, const char *source);
const char *omarchy_avatar_get_icon_name (OmarchyAvatar *self);
void omarchy_avatar_set_icon_name (OmarchyAvatar *self, const char *icon_name);
const char *omarchy_avatar_get_glyph (OmarchyAvatar *self);
void omarchy_avatar_set_glyph (OmarchyAvatar *self, const char *glyph);
const char *omarchy_avatar_get_size (OmarchyAvatar *self);
void omarchy_avatar_set_size (OmarchyAvatar *self, const char *size);
int omarchy_avatar_get_pixel_size (OmarchyAvatar *self);
void omarchy_avatar_set_pixel_size (OmarchyAvatar *self, int pixel_size);
gboolean omarchy_avatar_get_round (OmarchyAvatar *self);
void omarchy_avatar_set_round (OmarchyAvatar *self, gboolean round);
const char *omarchy_avatar_get_status (OmarchyAvatar *self);
void omarchy_avatar_set_status (OmarchyAvatar *self, const char *status);
const char *omarchy_avatar_get_ground (OmarchyAvatar *self);
void omarchy_avatar_set_ground (OmarchyAvatar *self, const char *ground);
gboolean omarchy_avatar_get_pictured (OmarchyAvatar *self);
gboolean omarchy_avatar_get_iconed (OmarchyAvatar *self);
char *omarchy_avatar_initial (const char *name);

#define OMARCHY_TYPE_TILE_GRID (omarchy_tile_grid_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTileGrid, omarchy_tile_grid, OMARCHY, TILE_GRID, GtkWidget)

GtkWidget *omarchy_tile_grid_new (void);
void omarchy_tile_grid_append (OmarchyTileGrid *self, const char *label, const char *glyph);
void omarchy_tile_grid_append_avatar (OmarchyTileGrid *self, const char *label, const char *name, const char *source,
                                      const char *glyph);
void omarchy_tile_grid_remove_all (OmarchyTileGrid *self);
guint omarchy_tile_grid_get_n_tiles (OmarchyTileGrid *self);
int omarchy_tile_grid_get_columns (OmarchyTileGrid *self);
void omarchy_tile_grid_set_columns (OmarchyTileGrid *self, int columns);
int omarchy_tile_grid_get_cell_width (OmarchyTileGrid *self);
void omarchy_tile_grid_set_cell_width (OmarchyTileGrid *self, int cell_width);
int omarchy_tile_grid_get_cursor_index (OmarchyTileGrid *self);
void omarchy_tile_grid_set_cursor_index (OmarchyTileGrid *self, int index);
int omarchy_tile_grid_get_current_index (OmarchyTileGrid *self);
void omarchy_tile_grid_set_current_index (OmarchyTileGrid *self, int index);
OmarchyCursorModel *omarchy_tile_grid_get_cursor_model (OmarchyTileGrid *self);

G_END_DECLS
