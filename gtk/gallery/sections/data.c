/* data: a Table 480 wide (Name flex, Kind 90, Size 70 right-aligned, Modified 100; sorted by
 * Name ascending; Cargo.toml selected, the screenshot under the cursor) beside a Tree 220 wide
 * (omarchy-ui › qt › Button.qml, Table.qml selected; rune folded; spec › tokens.md; README.md
 * under the cursor), top-aligned, `huge` apart (spec/components/data.md, qt/gallery/sections/data.qml). */
#include "gallery.h"

static GtkWidget *
table (void)
{
  GtkWidget *w = omarchy_table_new ();
  OmarchyTable *t = OMARCHY_TABLE (w);
  omarchy_table_append_column (t, "Name", 0, FALSE);
  omarchy_table_append_column (t, "Kind", 90, FALSE);
  omarchy_table_append_column (t, "Size", 70, TRUE);
  omarchy_table_append_column (t, "Modified", 100, FALSE);
  const char *rows[][5] = {
    { "assets", "Folder", "—", "2026-11-02", NULL },
    { "Cargo.toml", "TOML", "1.2 KB", "2026-11-07", NULL },
    { "README.md", "Markdown", "4.8 KB", "2026-11-05", NULL },
    { "screenshot-of-the-new-settings-panel.png", "PNG image", "1.4 MB", "2026-11-08", NULL },
    { "src", "Folder", "—", "2026-11-08", NULL },
    { "theme.toml", "TOML", "812 B", "2026-10-30", NULL },
  };
  for (guint i = 0; i < G_N_ELEMENTS (rows); i++)
    omarchy_table_append_row (t, rows[i]);
  omarchy_table_set_sort_column (t, 0);
  omarchy_table_set_sort_ascending (t, TRUE);
  omarchy_table_set_selected_row (t, 1);
  omarchy_table_set_cursor_row (t, 3);
  omarchy_table_set_has_cursor (t, TRUE);
  return w;
}

static GtkWidget *
tree (void)
{
  GtkWidget *w = omarchy_tree_new ();
  OmarchyTree *t = OMARCHY_TREE (w);
  const char *open = gallery_glyph (0xF0770), *folder = gallery_glyph (0xF024B), *file = gallery_glyph (0xF0219);
  omarchy_tree_append (t, NULL, "root", "omarchy-ui", open);
  omarchy_tree_append (t, "root", "qt", "qt", open);
  omarchy_tree_append (t, "qt", "qt/button", "Button.qml", file);
  omarchy_tree_append (t, "qt", "qt/table", "Table.qml", file);
  omarchy_tree_append (t, "root", "rune", "rune", folder);
  omarchy_tree_append (t, "rune", "rune/lib", "lib.rs", file);
  omarchy_tree_append (t, "root", "spec", "spec", open);
  omarchy_tree_append (t, "spec", "spec/tokens", "tokens.md", file);
  omarchy_tree_append (t, "root", "readme", "README.md", file);
  const char *expanded[] = { "root", "qt", "spec", NULL };
  omarchy_tree_set_expanded (t, expanded);
  omarchy_tree_set_selected_id (t, "qt/table");
  omarchy_tree_set_cursor_id (t, "readme");
  omarchy_tree_set_has_cursor (t, TRUE);
  return w;
}

GtkWidget *
gallery_section_data (void)
{
  GtkWidget *section = gallery_section ("Data");
  GtkWidget *row = gallery_row ("huge");
  gallery_add (row, gallery_top (gallery_width (table (), 480)));
  gallery_add (row, gallery_top (gallery_width (tree (), 220)));
  gallery_add (section, gallery_top (row));
  return section;
}
