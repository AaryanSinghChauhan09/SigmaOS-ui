/* omarchy-toggle.h: ToggleSwitch and Toggle (spec/components.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_TOGGLE_SWITCH (omarchy_toggle_switch_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyToggleSwitch, omarchy_toggle_switch, OMARCHY, TOGGLE_SWITCH, GtkWidget)
GtkWidget *omarchy_toggle_switch_new (void);
gboolean omarchy_toggle_switch_get_checked (OmarchyToggleSwitch *self);
void omarchy_toggle_switch_set_checked (OmarchyToggleSwitch *self, gboolean checked);
gboolean omarchy_toggle_switch_get_cursor (OmarchyToggleSwitch *self);
void omarchy_toggle_switch_set_cursor (OmarchyToggleSwitch *self, gboolean cursor);
gboolean omarchy_toggle_switch_get_interactive (OmarchyToggleSwitch *self);
void omarchy_toggle_switch_set_interactive (OmarchyToggleSwitch *self, gboolean interactive);
gboolean omarchy_toggle_switch_get_busy (OmarchyToggleSwitch *self);
void omarchy_toggle_switch_set_busy (OmarchyToggleSwitch *self, gboolean busy);

#define OMARCHY_TYPE_TOGGLE (omarchy_toggle_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyToggle, omarchy_toggle, OMARCHY, TOGGLE, GtkWidget)
GtkWidget *omarchy_toggle_new (const char *label, const char *description);
const char *omarchy_toggle_get_label (OmarchyToggle *self);
void omarchy_toggle_set_label (OmarchyToggle *self, const char *label);
const char *omarchy_toggle_get_description (OmarchyToggle *self);
void omarchy_toggle_set_description (OmarchyToggle *self, const char *description);
gboolean omarchy_toggle_get_checked (OmarchyToggle *self);
void omarchy_toggle_set_checked (OmarchyToggle *self, gboolean checked);
gboolean omarchy_toggle_get_cursor (OmarchyToggle *self);
void omarchy_toggle_set_cursor (OmarchyToggle *self, gboolean cursor);

G_END_DECLS
