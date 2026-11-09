/* messages: two EmptyStates centered in 350 × 150 boxes (panel, with an action; overlay), four
 * dismissible Banners at full width (info with an action, success, warning, error) and a Toast
 * held open with an action and a countdown (spec/components/feedback.md).
 * (qt/gallery/sections/messages.qml) */
#include "gallery.h"

static GtkWidget *
empty_box (const char *size, gunichar glyph, const char *title, const char *hint, const char *action)
{
  GtkWidget *e = omarchy_empty_state_new (title);
  omarchy_empty_state_set_size (OMARCHY_EMPTY_STATE (e), size);
  omarchy_empty_state_set_glyph (OMARCHY_EMPTY_STATE (e), gallery_glyph (glyph));
  omarchy_empty_state_set_hint (OMARCHY_EMPTY_STATE (e), hint);
  if (action)
    omarchy_empty_state_set_action_text (OMARCHY_EMPTY_STATE (e), action);
  gtk_widget_set_valign (e, GTK_ALIGN_CENTER);
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_append (GTK_BOX (box), e);
  gtk_widget_set_vexpand (e, TRUE);
  return gallery_size (box, 350, 150);
}

static GtkWidget *
banner (const char *tone, const char *text, const char *action)
{
  GtkWidget *b = omarchy_banner_new (text);
  omarchy_banner_set_tone (OMARCHY_BANNER (b), tone);
  if (action)
    omarchy_banner_set_action_text (OMARCHY_BANNER (b), action);
  omarchy_banner_set_dismissible (OMARCHY_BANNER (b), TRUE);
  return gallery_fill (b);
}

GtkWidget *
gallery_section_messages (void)
{
  GtkWidget *section = gallery_section ("Messages");

  GtkWidget *row = gallery_row ("huge");
  gallery_add (row, empty_box ("panel", 0xF05AA, "No networks found", "Turn on Wi-Fi to scan", "Scan"));
  gallery_add (row, empty_box ("overlay", 0xF014C, "Clipboard is empty", "Copy something to see it here", NULL));
  gallery_add (section, row);

  gallery_add (section, banner ("info", "Omarchy 3.2 is available", "Update"));
  gallery_add (section, banner ("success", "Backup finished", NULL));
  gallery_add (section, banner ("warning", "Battery is low (12%)", NULL));
  gallery_add (section, banner ("error", "Could not reach the update server", NULL));

  /* A toast as it draws, held open (the app owns when it shows and the timer). */
  GtkWidget *toast = omarchy_toast_new ("Export finished");
  omarchy_toast_set_glyph (OMARCHY_TOAST (toast), gallery_glyph (0xF012C));
  omarchy_toast_set_text (OMARCHY_TOAST (toast), "omarchy-demo.mp4 · 42 MB");
  omarchy_toast_set_action_text (OMARCHY_TOAST (toast), "Open");
  omarchy_toast_set_countdown (OMARCHY_TOAST (toast), 0.6);
  gallery_add (section, toast);
  return section;
}
