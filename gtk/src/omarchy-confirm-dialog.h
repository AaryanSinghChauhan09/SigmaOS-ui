/* omarchy-confirm-dialog.h: ConfirmDialog (spec/components.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_CONFIRM_DIALOG (omarchy_confirm_dialog_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyConfirmDialog, omarchy_confirm_dialog, OMARCHY, CONFIRM_DIALOG, GtkWidget)
GtkWidget *omarchy_confirm_dialog_new (const char *message);
const char *omarchy_confirm_dialog_get_message (OmarchyConfirmDialog *self);
void omarchy_confirm_dialog_set_message (OmarchyConfirmDialog *self, const char *message);
void omarchy_confirm_dialog_set_cancel_text (OmarchyConfirmDialog *self, const char *text);
void omarchy_confirm_dialog_set_confirm_text (OmarchyConfirmDialog *self, const char *text);
int omarchy_confirm_dialog_get_selected_index (OmarchyConfirmDialog *self);
void omarchy_confirm_dialog_set_selected_index (OmarchyConfirmDialog *self, int index);

G_END_DECLS
