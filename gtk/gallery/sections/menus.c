/* menus: a Menu held open (sections, glyphs, hints, a check, a disabled row, separators, a
 * submenu row, the cursor on "Open…"), a ContextMenu held open (radio rows, a destructive row)
 * and a Tooltip with its shortcut, `huge` apart; a row of Keycaps and a KeyCombo `lg` apart; a
 * ShortcutSheet (spec/components/navigation.md). (qt/gallery/sections/menus.qml) */
#include "gallery.h"

static OmarchyMenuItem *
item (GtkWidget *menu, const char *text, gunichar glyph, const char *shortcut)
{
  OmarchyMenuItem *i = omarchy_menu_append_item (OMARCHY_MENU (menu), text);
  if (glyph)
    omarchy_menu_item_set_icon (i, gallery_glyph (glyph));
  if (shortcut)
    omarchy_menu_item_set_shortcut (i, shortcut);
  return i;
}

static GtkWidget *
file_menu (void)
{
  GtkWidget *m = omarchy_menu_new ();
  OmarchyMenu *menu = OMARCHY_MENU (m);
  omarchy_menu_set_held (menu, TRUE);
  omarchy_menu_append_section (menu, "File");
  item (m, "New", 0xF15B, "Ctrl+N");
  omarchy_menu_item_set_cursor (item (m, "Open…", 0xF07B, "Ctrl+O"), TRUE);
  item (m, "Save", 0xF0C7, "Ctrl+S");
  gtk_widget_set_sensitive (GTK_WIDGET (item (m, "Duplicate", 0xF0C5, NULL)), FALSE);
  omarchy_menu_append_separator (menu);
  omarchy_menu_append_section (menu, "View");
  OmarchyMenuItem *sidebar = item (m, "Sidebar", 0, NULL);
  omarchy_menu_item_set_checkable (sidebar, TRUE);
  omarchy_menu_item_set_checked (sidebar, TRUE);
  omarchy_menu_item_set_checkable (item (m, "Word wrap", 0, NULL), TRUE);
  omarchy_menu_append_separator (menu);
  omarchy_menu_item_set_has_submenu (item (m, "Export", 0xF064, NULL), TRUE);
  return m;
}

static GtkWidget *
context_menu (void)
{
  GtkWidget *m = omarchy_context_menu_new ();
  OmarchyMenu *menu = OMARCHY_MENU (m);
  omarchy_menu_set_held (menu, TRUE);
  item (m, "Cut", 0, "Ctrl+X");
  item (m, "Copy", 0, "Ctrl+C");
  item (m, "Paste", 0, "Ctrl+V");
  omarchy_menu_append_separator (menu);
  OmarchyMenuItem *by_name = item (m, "By name", 0, NULL);
  omarchy_menu_item_set_checkable (by_name, TRUE);
  omarchy_menu_item_set_exclusive (by_name, TRUE);
  omarchy_menu_item_set_checked (by_name, TRUE);
  OmarchyMenuItem *by_date = item (m, "By date", 0, NULL);
  omarchy_menu_item_set_checkable (by_date, TRUE);
  omarchy_menu_item_set_exclusive (by_date, TRUE);
  omarchy_menu_append_separator (menu);
  omarchy_menu_item_set_destructive (item (m, "Delete", 0, "Del"), TRUE);
  return m;
}

GtkWidget *
gallery_section_menus (void)
{
  GtkWidget *section = gallery_section ("Menus");

  GtkWidget *row = gallery_row ("huge");
  gallery_add (row, gallery_top (file_menu ()));
  gallery_add (row, gallery_top (context_menu ()));
  GtkWidget *tip = omarchy_tooltip_new ("Save");
  omarchy_tooltip_set_shortcut (OMARCHY_TOOLTIP (tip), "Ctrl+S");
  gallery_add (row, gallery_top (tip));
  gallery_add (section, row);

  GtkWidget *keys = gallery_row ("lg");
  static const char *const names[] = { "Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K" };
  for (gsize i = 0; i < G_N_ELEMENTS (names); i++)
    gallery_add (keys, omarchy_keycap_new (names[i]));
  gallery_add (keys, omarchy_key_combo_new ("Ctrl+Shift+P"));
  gallery_add (section, keys);

  GtkWidget *sheet = omarchy_shortcut_sheet_new ();
  OmarchyShortcutSheet *ss = OMARCHY_SHORTCUT_SHEET (sheet);
  omarchy_shortcut_sheet_add_group (ss, "File");
  omarchy_shortcut_sheet_add_shortcut (ss, "Ctrl+N", "New file");
  omarchy_shortcut_sheet_add_shortcut (ss, "Ctrl+O", "Open");
  omarchy_shortcut_sheet_add_shortcut (ss, "Ctrl+S", "Save");
  omarchy_shortcut_sheet_add_group (ss, "View");
  omarchy_shortcut_sheet_add_shortcut (ss, "Ctrl+B", "Sidebar");
  omarchy_shortcut_sheet_add_shortcut (ss, "Ctrl+Shift+P", "Commands");
  omarchy_shortcut_sheet_add_shortcut (ss, "F11", "Full screen");
  gallery_add (section, sheet);
  return section;
}
