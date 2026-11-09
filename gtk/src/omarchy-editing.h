/* omarchy-editing.h: Timeline, Scrubber, TrimHandle, CropFrame, DragHandle, DropIndicator and
 * ColorPicker (spec/components/editing.md). */
#pragma once

#if !defined(OMARCHY_GTK_INSIDE) && !defined(OMARCHY_GTK_COMPILATION)
#error "Only <omarchy-gtk.h> can be included directly."
#endif

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OMARCHY_TYPE_TIMELINE (omarchy_timeline_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTimeline, omarchy_timeline, OMARCHY, TIMELINE, GtkWidget)
GtkWidget *omarchy_timeline_new (double duration);
double omarchy_timeline_get_duration (OmarchyTimeline *self);
void omarchy_timeline_set_duration (OmarchyTimeline *self, double duration);
double omarchy_timeline_get_position (OmarchyTimeline *self);
void omarchy_timeline_set_position (OmarchyTimeline *self, double position);
int omarchy_timeline_add_track (OmarchyTimeline *self);
int omarchy_timeline_append_clip (OmarchyTimeline *self, int track, double start, double end, const char *label);
void omarchy_timeline_set_clip_range (OmarchyTimeline *self, int track, int clip, double start, double end);
gboolean omarchy_timeline_get_clip (OmarchyTimeline *self, int track, int clip, double *start, double *end,
                                    const char **label);
int omarchy_timeline_get_n_tracks (OmarchyTimeline *self);
int omarchy_timeline_get_n_clips (OmarchyTimeline *self, int track);
void omarchy_timeline_remove_all (OmarchyTimeline *self);
int omarchy_timeline_get_selected_track (OmarchyTimeline *self);
int omarchy_timeline_get_selected_clip (OmarchyTimeline *self);
void omarchy_timeline_set_selected (OmarchyTimeline *self, int track, int clip);
double omarchy_timeline_get_zoom (OmarchyTimeline *self);
double omarchy_timeline_get_view_start (OmarchyTimeline *self);
void omarchy_timeline_set_view (OmarchyTimeline *self, double zoom, double view_start);
double omarchy_timeline_get_max_zoom (OmarchyTimeline *self);
void omarchy_timeline_set_max_zoom (OmarchyTimeline *self, double max_zoom);
double omarchy_timeline_get_tick_step (OmarchyTimeline *self);
gboolean omarchy_timeline_get_cursor (OmarchyTimeline *self);
void omarchy_timeline_set_cursor (OmarchyTimeline *self, gboolean cursor);

#define OMARCHY_TYPE_SCRUBBER (omarchy_scrubber_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyScrubber, omarchy_scrubber, OMARCHY, SCRUBBER, GtkWidget)
GtkWidget *omarchy_scrubber_new (double position, double duration);
double omarchy_scrubber_get_position (OmarchyScrubber *self);
void omarchy_scrubber_set_position (OmarchyScrubber *self, double position);
double omarchy_scrubber_get_duration (OmarchyScrubber *self);
void omarchy_scrubber_set_duration (OmarchyScrubber *self, double duration);
gboolean omarchy_scrubber_get_cursor (OmarchyScrubber *self);
void omarchy_scrubber_set_cursor (OmarchyScrubber *self, gboolean cursor);
gboolean omarchy_scrubber_get_dragging (OmarchyScrubber *self);
void omarchy_scrubber_set_surface (OmarchyScrubber *self, const char *surface);

#define OMARCHY_TYPE_TRIM_HANDLE (omarchy_trim_handle_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyTrimHandle, omarchy_trim_handle, OMARCHY, TRIM_HANDLE, GtkWidget)
GtkWidget *omarchy_trim_handle_new (const char *edge);
GtkWidget *omarchy_trim_handle_new_crop (const char *edge);
const char *omarchy_trim_handle_get_edge (OmarchyTrimHandle *self);
void omarchy_trim_handle_set_edge (OmarchyTrimHandle *self, const char *edge);
gboolean omarchy_trim_handle_get_crop (OmarchyTrimHandle *self);
void omarchy_trim_handle_set_crop (OmarchyTrimHandle *self, gboolean crop);
gboolean omarchy_trim_handle_get_cursor (OmarchyTrimHandle *self);
void omarchy_trim_handle_set_cursor (OmarchyTrimHandle *self, gboolean cursor);
gboolean omarchy_trim_handle_get_dragging (OmarchyTrimHandle *self);
void omarchy_trim_handle_set_color (OmarchyTrimHandle *self, const GdkRGBA *color);
void omarchy_trim_handle_set_grip_color (OmarchyTrimHandle *self, const GdkRGBA *color);

#define OMARCHY_TYPE_CROP_FRAME (omarchy_crop_frame_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyCropFrame, omarchy_crop_frame, OMARCHY, CROP_FRAME, GtkWidget)
GtkWidget *omarchy_crop_frame_new (void);
const char *omarchy_crop_frame_get_cursor_edge (OmarchyCropFrame *self);
void omarchy_crop_frame_set_cursor_edge (OmarchyCropFrame *self, const char *edge);
OmarchyTrimHandle *omarchy_crop_frame_get_handle (OmarchyCropFrame *self, const char *edge);

#define OMARCHY_TYPE_DRAG_HANDLE (omarchy_drag_handle_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyDragHandle, omarchy_drag_handle, OMARCHY, DRAG_HANDLE, GtkWidget)
GtkWidget *omarchy_drag_handle_new (void);
gboolean omarchy_drag_handle_get_cursor (OmarchyDragHandle *self);
void omarchy_drag_handle_set_cursor (OmarchyDragHandle *self, gboolean cursor);
gboolean omarchy_drag_handle_get_dragging (OmarchyDragHandle *self);

#define OMARCHY_TYPE_DROP_INDICATOR (omarchy_drop_indicator_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyDropIndicator, omarchy_drop_indicator, OMARCHY, DROP_INDICATOR, GtkWidget)
GtkWidget *omarchy_drop_indicator_new (void);
int omarchy_drop_indicator_get_thickness (void);

#define OMARCHY_TYPE_COLOR_PICKER (omarchy_color_picker_get_type ())
G_DECLARE_FINAL_TYPE (OmarchyColorPicker, omarchy_color_picker, OMARCHY, COLOR_PICKER, GtkWidget)
GtkWidget *omarchy_color_picker_new (void);
double omarchy_color_picker_get_hue (OmarchyColorPicker *self);
void omarchy_color_picker_set_hue (OmarchyColorPicker *self, double hue);
double omarchy_color_picker_get_saturation (OmarchyColorPicker *self);
void omarchy_color_picker_set_saturation (OmarchyColorPicker *self, double saturation);
double omarchy_color_picker_get_value (OmarchyColorPicker *self);
void omarchy_color_picker_set_value (OmarchyColorPicker *self, double value);
double omarchy_color_picker_get_alpha (OmarchyColorPicker *self);
void omarchy_color_picker_set_alpha (OmarchyColorPicker *self, double alpha);
gboolean omarchy_color_picker_get_show_alpha (OmarchyColorPicker *self);
void omarchy_color_picker_set_show_alpha (OmarchyColorPicker *self, gboolean show_alpha);
gboolean omarchy_color_picker_get_cursor (OmarchyColorPicker *self);
void omarchy_color_picker_set_cursor (OmarchyColorPicker *self, gboolean cursor);
void omarchy_color_picker_set_surface (OmarchyColorPicker *self, const char *surface);
const char *const *omarchy_color_picker_get_palette_names (OmarchyColorPicker *self);
void omarchy_color_picker_set_palette_names (OmarchyColorPicker *self, const char *const *names);
void omarchy_color_picker_get_color (OmarchyColorPicker *self, GdkRGBA *color);
const char *omarchy_color_picker_get_hex (OmarchyColorPicker *self);
gboolean omarchy_color_picker_set_color (OmarchyColorPicker *self, const char *hex);
void omarchy_color_picker_set_rgba (OmarchyColorPicker *self, const GdkRGBA *color);
int omarchy_color_picker_get_n_swatches (OmarchyColorPicker *self);

G_END_DECLS
