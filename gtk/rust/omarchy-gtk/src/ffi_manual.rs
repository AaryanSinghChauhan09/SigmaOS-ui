// The C functions the safe API below calls (the widget types are in ffi.rs, generated).
use std::os::raw::{c_char, c_double, c_int};

unsafe extern "C" {
    pub fn omarchy_gtk_init() -> *mut glib::gobject_ffi::GObject;
    pub fn omarchy_get_version() -> *const c_char;
    pub fn omarchy_theme_get_default() -> *mut glib::gobject_ffi::GObject;
    pub fn omarchy_theme_get_tokens_json(theme: *mut glib::gobject_ffi::GObject) -> *const c_char;
    pub fn omarchy_theme_get_color(theme: *mut glib::gobject_ffi::GObject, path: *const c_char) -> *mut gtk::gdk::ffi::GdkRGBA;
    pub fn omarchy_theme_get_paint_color(theme: *mut glib::gobject_ffi::GObject, path: *const c_char) -> *mut gtk::gdk::ffi::GdkRGBA;
    pub fn omarchy_theme_get_number(theme: *mut glib::gobject_ffi::GObject, path: *const c_char) -> c_double;
    pub fn omarchy_theme_space(theme: *mut glib::gobject_ffi::GObject, px: c_double) -> c_int;
    pub fn omarchy_theme_lookup_icon(theme: *mut glib::gobject_ffi::GObject, name: *const c_char, size: c_int) -> *mut c_char;
    pub fn omarchy_theme_translate(theme: *mut glib::gobject_ffi::GObject, key: *const c_char, fallback: *const c_char) -> *const c_char;
}
