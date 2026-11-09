//! Omarchy UI for gtk4-rs: Omarchy's look on GTK 4 and libadwaita, the live theme, and the
//! component kit. A binding of the C library libomarchy-gtk-1, so the widgets are the same ones C,
//! Python, GJS and Vala apps get.
//!
//! ```no_run
//! use gtk::prelude::*;
//!
//! let app = gtk::Application::builder().application_id("org.example.App").build();
//! app.connect_activate(|app| {
//!     let theme = omarchy_gtk::init(); // the one call: restyle GTK, follow the desktop
//!     let card = omarchy_gtk::Card::new("Network");
//!     card.append(&omarchy_gtk::Toggle::new("Wi-Fi", "On"));
//!     theme.connect_changed(|_, tokens| println!("now {}", tokens.palette_background()));
//!     gtk::ApplicationWindow::builder().application(app).child(&card).build().present();
//! });
//! app.run();
//! ```
//!
//! Every kit widget is a type here (`Button`, `Toggle`, `ListRow`, ...); set what has no typed
//! method through its GObject properties (`widget.set_property("bordered", true)`), with the names
//! of the C API's gtk-doc.

mod ffi_manual;
#[allow(clippy::all)]
pub mod ffi;
mod widgets;

use std::ffi::{CStr, CString};

use glib::prelude::*;
use glib::translate::*;
pub use widgets::*;

pub use gtk;

/// `omarchy_gtk_init()`: Omarchy's look on stock GTK and libadwaita widgets, libadwaita's color
/// scheme and accent from the theme, the kit's types registered, and the desktop followed live
/// (colors fade on a switch). Call after GTK is initialized (in `activate`), before showing a
/// window. Returns the theme.
pub fn init() -> Theme {
    unsafe { from_glib_none(ffi_manual::omarchy_gtk_init() as *mut ffi::OmarchyTheme) }
}

/// The library's version.
pub fn version() -> &'static str {
    unsafe { CStr::from_ptr(ffi_manual::omarchy_get_version()).to_str().unwrap_or("") }
}

/// The tokens a change delivers: the canonical JSON's shape (spec/tokens.md) as a `a{sv}`.
pub struct Tokens(pub glib::Variant);

impl Tokens {
    /// A value by its path in the canonical JSON (`palette.background`).
    pub fn lookup(&self, path: &str) -> Option<glib::Variant> {
        let mut v = self.0.clone();
        for part in path.split('.') {
            let dict = glib::VariantDict::new(Some(&v));
            v = dict.lookup_value(part, None)?;
        }
        Some(v)
    }
    /// A string at a path (`mode`, `palette.background` as `#rrggbbaa`).
    pub fn string(&self, path: &str) -> Option<String> {
        self.lookup(path).and_then(|v| v.str().map(str::to_owned))
    }
    pub fn palette_background(&self) -> String {
        self.string("palette.background").unwrap_or_default()
    }
}

impl Theme {
    /// The theme, made and following the desktop on first use.
    pub fn default() -> Theme {
        unsafe { from_glib_none(ffi_manual::omarchy_theme_get_default() as *mut ffi::OmarchyTheme) }
    }
    fn raw(&self) -> *mut glib::gobject_ffi::GObject {
        self.as_ptr() as *mut _
    }
    /// 0 for the tokens loaded first, plus one per change: a cache key.
    pub fn revision(&self) -> u64 {
        self.property::<u64>("revision")
    }
    pub fn fading(&self) -> bool {
        self.property::<bool>("fading")
    }
    /// The tokens as canonical JSON on one line.
    pub fn tokens_json(&self) -> String {
        unsafe { CStr::from_ptr(ffi_manual::omarchy_theme_get_tokens_json(self.raw())).to_string_lossy().into_owned() }
    }
    /// A color by its canonical JSON path (`palette.accent`, `controls.hover.fill`): the target.
    pub fn color(&self, path: &str) -> Option<gtk::gdk::RGBA> {
        let p = CString::new(path).ok()?;
        unsafe { from_glib_full(ffi_manual::omarchy_theme_get_color(self.raw(), p.as_ptr())) }
    }
    /// The color on screen: mid-fade, a frame between two themes.
    pub fn paint_color(&self, path: &str) -> Option<gtk::gdk::RGBA> {
        let p = CString::new(path).ok()?;
        unsafe { from_glib_full(ffi_manual::omarchy_theme_get_paint_color(self.raw(), p.as_ptr())) }
    }
    /// A number by path (`radius`, `spacing.panelPadding`, `font.body`).
    pub fn number(&self, path: &str) -> f64 {
        let p = CString::new(path).unwrap_or_default();
        unsafe { ffi_manual::omarchy_theme_get_number(self.raw(), p.as_ptr()) }
    }
    /// A spacing token in pixels (`controlGap`, `panelPadding`).
    pub fn spacing(&self, token: &str) -> i32 {
        self.number(&format!("spacing.{token}")) as i32
    }
    /// A type-scale token in pixels (`body`, `title`).
    pub fn font_size(&self, token: &str) -> i32 {
        self.number(&format!("font.{token}")) as i32
    }
    /// `Style.space(px)`: a size on the theme's spacing scale.
    pub fn space(&self, px: f64) -> i32 {
        unsafe { ffi_manual::omarchy_theme_space(self.raw(), px) }
    }
    /// The file an icon name resolves to in the theme's icon theme at `size` device pixels.
    pub fn lookup_icon(&self, name: &str, size: i32) -> Option<std::path::PathBuf> {
        let n = CString::new(name).ok()?;
        unsafe {
            let p = ffi_manual::omarchy_theme_lookup_icon(self.raw(), n.as_ptr(), size);
            if p.is_null() {
                return None;
            }
            let s = CStr::from_ptr(p).to_string_lossy().into_owned();
            glib::ffi::g_free(p as *mut _);
            Some(s.into())
        }
    }
    /// The kit's string for `key` in the display language, or `fallback`.
    pub fn translate(&self, key: &str, fallback: &str) -> String {
        let (k, f) = (CString::new(key).unwrap_or_default(), CString::new(fallback).unwrap_or_default());
        unsafe { CStr::from_ptr(ffi_manual::omarchy_theme_translate(self.raw(), k.as_ptr(), f.as_ptr())).to_string_lossy().into_owned() }
    }
    /// The changed hook (spec/following.md): once per real change, on the main thread, after the
    /// new tokens are in effect, with them; never for the initial load.
    pub fn connect_changed<F: Fn(&Theme, Tokens) + 'static>(&self, f: F) -> glib::SignalHandlerId {
        self.connect_local("changed", false, move |args| {
            let theme = args[0].get::<Theme>().ok()?;
            let tokens = args[1].get::<glib::Variant>().ok()?;
            f(&theme, Tokens(tokens));
            None
        })
    }
    /// Every frame the colors on screen move (the fade), for custom drawing that follows it.
    pub fn connect_repaint<F: Fn(&Theme) + 'static>(&self, f: F) -> glib::SignalHandlerId {
        self.connect_local("repaint", false, move |args| {
            f(&args[0].get::<Theme>().ok()?);
            None
        })
    }
}

macro_rules! new_with {
    ($ty:ident, $($prop:literal : $arg:ident),*) => {
        impl $ty {
            pub fn new($($arg: &str),*) -> $ty {
                glib::Object::builder()$(.property($prop, $arg))*.build()
            }
        }
    };
}

new_with!(Button, "label": label);
new_with!(IconButton, "icon": glyph);
new_with!(Label, "text": text);
new_with!(Card, "title": title);
new_with!(Toggle, "label": label, "description": description);
new_with!(MenuRow, "text": text);
new_with!(Tooltip, "text": text);
new_with!(SectionHeader, "text": text);
new_with!(Heading, "text": text);
new_with!(Dropdown, "label": label);
new_with!(ConfirmDialog, "message": message);

impl Card {
    pub fn append(&self, child: &impl IsA<gtk::Widget>) {
        unsafe extern "C" {
            fn omarchy_card_append(card: *mut ffi::OmarchyCard, child: *mut gtk::ffi::GtkWidget);
        }
        unsafe { omarchy_card_append(self.to_glib_none().0, child.as_ref().to_glib_none().0) }
    }
}

impl Pane {
    pub fn new(title: &str) -> Pane {
        let pane: Pane = glib::Object::new();
        pane.set_title(title);
        pane
    }
    pub fn set_title(&self, title: &str) {
        unsafe extern "C" {
            fn omarchy_pane_set_title(pane: *mut ffi::OmarchyPane, title: *const std::os::raw::c_char);
        }
        let t = CString::new(title).unwrap_or_default();
        unsafe { omarchy_pane_set_title(self.to_glib_none().0, t.as_ptr()) }
    }
    pub fn append(&self, child: &impl IsA<gtk::Widget>) {
        unsafe extern "C" {
            fn omarchy_pane_append(pane: *mut ffi::OmarchyPane, child: *mut gtk::ffi::GtkWidget);
        }
        unsafe { omarchy_pane_append(self.to_glib_none().0, child.as_ref().to_glib_none().0) }
    }
}

impl Heading {
    pub fn add_trailing(&self, child: &impl IsA<gtk::Widget>) {
        unsafe extern "C" {
            fn omarchy_heading_add_trailing(h: *mut ffi::OmarchyHeading, child: *mut gtk::ffi::GtkWidget);
        }
        unsafe { omarchy_heading_add_trailing(self.to_glib_none().0, child.as_ref().to_glib_none().0) }
    }
}

impl Button {
    pub fn connect_clicked<F: Fn(&Button) + 'static>(&self, f: F) -> glib::SignalHandlerId {
        self.connect_local("clicked", false, move |args| {
            f(&args[0].get::<Button>().ok()?);
            None
        })
    }
}

impl Toggle {
    pub fn is_checked(&self) -> bool {
        self.property("checked")
    }
    pub fn set_checked(&self, on: bool) {
        self.set_property("checked", on)
    }
    /// After a click or Space/Return flipped it; read [`Toggle::is_checked`].
    pub fn connect_toggled<F: Fn(&Toggle) + 'static>(&self, f: F) -> glib::SignalHandlerId {
        self.connect_local("toggled", false, move |args| {
            f(&args[0].get::<Toggle>().ok()?);
            None
        })
    }
}
