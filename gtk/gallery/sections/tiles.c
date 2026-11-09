/* tiles: Badges `controlGap` apart; StatusDots and medium Avatars with docked dots; Avatars by
 * size and fallback; Avatars by icon name (the fixture icon tree under review); a TileGrid of 4
 * columns with the cursor on Browser (spec/components/lists.md, qt/gallery/sections/tiles.qml). */
#include "gallery.h"

/* The sample picture, qt/gallery/assets/avatar.png, from the repo root (where the review runs
 * the gallery) or beside the binary in build/gtk. */
static const char *
asset (const char *name)
{
  static char *paths[8];
  static int n;
  g_autofree char *rel = g_build_filename ("qt", "gallery", "assets", name, NULL);
  char *path = NULL;
  if (g_file_test (rel, G_FILE_TEST_EXISTS)) {
    path = g_canonicalize_filename (rel, NULL);
  } else {
    g_autofree char *exe = g_file_read_link ("/proc/self/exe", NULL);
    g_autofree char *dir = exe ? g_path_get_dirname (exe) : g_get_current_dir ();
    path = g_build_filename (dir, "..", "..", "qt", "gallery", "assets", name, NULL);
  }
  g_free (paths[n % 8]);
  paths[n++ % 8] = path;
  return path;
}

static GtkWidget *
badge (const char *text, const char *tone, gboolean filled)
{
  GtkWidget *b = omarchy_badge_new (text);
  omarchy_badge_set_tone (OMARCHY_BADGE (b), tone);
  omarchy_badge_set_filled (OMARCHY_BADGE (b), filled);
  return b;
}

static GtkWidget *
avatar (const char *name, const char *size, gboolean round)
{
  GtkWidget *a = omarchy_avatar_new (name);
  if (size)
    omarchy_avatar_set_size (OMARCHY_AVATAR (a), size);
  omarchy_avatar_set_round (OMARCHY_AVATAR (a), round);
  return a;
}

static GtkWidget *
pictured (const char *name, const char *source, const char *size, gboolean round)
{
  GtkWidget *a = avatar (name, size, round);
  omarchy_avatar_set_source (OMARCHY_AVATAR (a), source);
  return a;
}

static GtkWidget *
docked (GtkWidget *a, const char *status)
{
  omarchy_avatar_set_status (OMARCHY_AVATAR (a), status);
  omarchy_avatar_set_ground (OMARCHY_AVATAR (a), "background");
  return a;
}

static GtkWidget *
iconed (const char *name, const char *icon, const char *size)
{
  GtkWidget *a = avatar (name, size, FALSE);
  omarchy_avatar_set_icon_name (OMARCHY_AVATAR (a), icon);
  return a;
}

GtkWidget *
gallery_section_tiles (void)
{
  GtkWidget *section = gallery_section ("Tiles");
  const char *photo = asset ("avatar.png");

  GtkWidget *row = gallery_row ("controlGap");
  gallery_add (row, badge ("New", "", FALSE));
  gallery_add (row, badge ("3", "accent", TRUE));
  gallery_add (row, badge ("12", "urgent", TRUE));
  gallery_add (row, badge ("Online", "success", FALSE));
  gallery_add (row, badge ("Low", "warning", FALSE));
  gallery_add (row, badge ("Offline", "urgent", FALSE));
  gallery_add (section, row);

  row = gallery_row ("xxl");
  const char *tones[] = { "success", "warning", "urgent", "muted", "accent" };
  for (int i = 0; i < 5; i++)
    gallery_add (row, omarchy_status_dot_new (tones[i]));
  gallery_add (row, docked (avatar ("Ryan", NULL, TRUE), "success"));
  gallery_add (row, docked (pictured ("Photo", photo, NULL, FALSE), "warning"));
  GtkWidget *server = avatar ("Server", NULL, FALSE);
  omarchy_avatar_set_glyph (OMARCHY_AVATAR (server), gallery_glyph (0xF048B));
  gallery_add (row, docked (server, "urgent"));
  gallery_add (section, row);

  row = gallery_row ("xxl");
  gallery_add (row, avatar ("ada", "small", FALSE));
  gallery_add (row, avatar ("Basecamp", NULL, FALSE));
  gallery_add (row, avatar ("Omarchy", "large", TRUE));
  GtkWidget *robot = avatar ("Robot", NULL, FALSE);
  omarchy_avatar_set_glyph (OMARCHY_AVATAR (robot), gallery_glyph (0xF06A9));
  gallery_add (row, robot);
  gallery_add (row, pictured ("Photo", photo, "small", FALSE));
  gallery_add (row, pictured ("Photo", photo, NULL, TRUE));
  gallery_add (row, pictured ("Photo", photo, "large", FALSE));
  gallery_add (row, pictured ("Missing", asset ("missing.png"), NULL, FALSE));
  gallery_add (section, row);

  /* By icon name, from the icon theme (the fixture icon theme under review). */
  row = gallery_row ("xxl");
  gallery_add (row, iconed ("Files", "org.gnome.Nautilus", NULL));
  gallery_add (row, iconed ("Terminal", "utilities-terminal", NULL));
  gallery_add (row, iconed ("Firefox", "firefox", NULL));
  gallery_add (row, iconed ("Chromium", "chromium", "small"));
  gallery_add (row, iconed ("Steam", "steam", "large"));
  gallery_add (row, iconed ("Btop", "btop", "small"));
  GtkWidget *nothing = iconed ("Nothing", "no-such-app", NULL);
  omarchy_avatar_set_glyph (OMARCHY_AVATAR (nothing), gallery_glyph (0xF120));
  gallery_add (row, nothing);
  gallery_add (section, row);

  GtkWidget *grid = omarchy_tile_grid_new ();
  OmarchyTileGrid *g = OMARCHY_TILE_GRID (grid);
  omarchy_tile_grid_set_columns (g, 4);
  omarchy_tile_grid_append (g, "Terminal", gallery_glyph (0xF120));
  omarchy_tile_grid_append (g, "Files", gallery_glyph (0xF07B));
  omarchy_tile_grid_append (g, "Browser", gallery_glyph (0xF0AC));
  omarchy_tile_grid_append (g, "Music", gallery_glyph (0xF001));
  omarchy_tile_grid_append (g, "Settings", gallery_glyph (0xF013));
  omarchy_tile_grid_append (g, "Calendar", gallery_glyph (0xF073));
  omarchy_tile_grid_append (g, "Screen Recorder", gallery_glyph (0xF03D));
  omarchy_tile_grid_append_avatar (g, "Photos", "Photos", photo, NULL);
  omarchy_tile_grid_set_cursor_index (g, 2);
  gallery_add (section, grid);
  return section;
}
