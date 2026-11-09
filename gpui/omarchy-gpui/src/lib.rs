//! Omarchy's look for GPUI: the live Omarchy theme, followed without a restart, and the kit's
//! components drawn the way the Qt kit draws them.
//!
//! ```ignore
//! use omarchy_gpui::{gpui::*, ui::*};
//!
//! gpui_platform::application().run(|cx| {
//!     omarchy_gpui::init(cx); // the theme, following the desktop, the font, the fade
//!     cx.open_window(WindowOptions::default(), |_, cx| cx.new(|_| App { wifi: true })).unwrap();
//! });
//! ```
//!
//! - [`init`] is the one call (`spec/following.md`): it installs [`Theme`], follows the desktop
//!   through `omarchy_theme::watch` onto the app thread, fades colors on a switch, and refreshes
//!   windows.
//! - [`on_change`] (or `cx.observe_global::<Theme>()`) is the changed hook; [`Theme::revision`]
//!   a cache key.
//! - [`Paint`] is what's on screen (colors mid-fade); [`ui`] the components.

mod fade;
mod font;
mod theme;
pub mod ui;
#[cfg(feature = "gpui-component")]
pub mod component;
#[cfg(feature = "snap")]
pub mod snap;

pub use fade::{cubic_bezier, mix_tokens};
pub use theme::{Paint, Theme, color, install, on_change, set_tokens};

pub use gpui;
/// The platform crate GPUI apps start from (`gpui_platform::application()`), the one this kit is
/// built against.
pub use gpui_platform;
pub use omarchy_theme;
pub use omarchy_theme::Tokens;

/// The one call: install the live Omarchy theme as a GPUI global and follow the desktop (theme
/// switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads, the language and
/// icon theme). With the `gpui-component` feature, gpui-component's theme follows too. Calling it
/// again does nothing.
pub fn init(cx: &mut gpui::App) {
    if cx.has_global::<Theme>() {
        return;
    }
    install(cx, omarchy_theme::load());
    theme::follow(cx);
}
