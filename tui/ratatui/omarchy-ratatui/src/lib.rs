//! Omarchy's look for Ratatui: the live Omarchy theme, followed without a restart, and the kit's
//! components drawn on the terminal's cell grid the way `spec/terminal.md` maps them.
//!
//! ```no_run
//! use omarchy_ratatui::{App, Draw, Flow, Theme, Themed, ui::*};
//! use omarchy_ratatui::ratatui::Frame;
//! use omarchy_ratatui::crossterm::event::Event;
//!
//! struct Demo { theme: Theme, wifi: bool }
//! impl Themed for Demo {
//!     fn theme(&self) -> &Theme { &self.theme }
//!     fn theme_mut(&mut self) -> &mut Theme { &mut self.theme }
//! }
//! impl App for Demo {
//!     fn draw(&mut self, frame: &mut Frame) {
//!         let t = &self.theme;
//!         frame.render_widget(Toggle::new("Wi-Fi").description("Basecamp").on(self.wifi).width(40).with(t), frame.area());
//!     }
//!     fn event(&mut self, ev: &Event) -> Flow { if omarchy_ratatui::is_quit(ev) { Flow::Quit } else { Flow::Continue } }
//! }
//! fn main() -> std::io::Result<()> {
//!     omarchy_ratatui::run(Demo { theme: Theme::load(), wifi: true }).map(|_| ())
//! }
//! ```
//!
//! - [`run`] is the one call (`spec/following.md`): it sets the terminal up, follows the theme,
//!   fades colors on a switch (truecolor only, none under reduced motion), and calls your
//!   [`App::draw`] and [`App::event`]. [`Themed::theme_changed`] is the changed hook,
//!   [`Theme::revision`] a cache key.
//! - [`ui`] has every component of `spec/components.md`, drawn per `spec/terminal.md`; [`Draw`] is
//!   their trait and [`Draw::with`] makes any of them a Ratatui `Widget`.
//! - [`paint`] is the cell painting they share (alpha compositing, control edges, box corners).

// Painting helpers take a cell's position, size and state: many plain arguments.
#![allow(clippy::too_many_arguments)]

mod depth;
mod fade;
pub mod layout;
pub mod paint;
mod run;
#[cfg(feature = "snap")]
pub mod snap;
mod theme;
pub mod ui;

pub use depth::{Depth, nearest256, over, xterm256};
pub use fade::{cubic_bezier, mix_tokens};
pub use run::{App, Flow, Themed, is_quit, run, run_on};
pub use theme::Theme;

pub use crossterm;
pub use omarchy_theme;
pub use omarchy_theme::{Rgba, Tokens};
pub use ratatui;

use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

/// A kit component: it knows its natural size in cells and draws itself into an area.
pub trait Draw {
    /// The natural size, (columns, rows).
    fn size(&self, t: &Theme) -> (u16, u16);
    /// Draw into `area` (normally the natural size; wider areas stretch fills where that makes
    /// sense).
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer);

    /// This component as a Ratatui widget, drawn with `t`.
    fn with(self, t: &Theme) -> On<'_, Self>
    where
        Self: Sized,
    {
        On(t, self)
    }
}

/// A component bound to a theme: a Ratatui `Widget`.
pub struct On<'t, W>(pub &'t Theme, pub W);

impl<W: Draw> ratatui::widgets::Widget for On<'_, W> {
    fn render(self, area: Rect, buf: &mut Buffer) {
        let (w, h) = self.1.size(self.0);
        let area = Rect { width: area.width.min(w.max(1)), height: area.height.min(h.max(1)), ..area };
        self.1.draw(self.0, area, buf);
    }
}

impl<D: Draw + ?Sized> Draw for Box<D> {
    fn size(&self, t: &Theme) -> (u16, u16) {
        (**self).size(t)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        (**self).draw(t, area, buf)
    }
}

impl<D: Draw + ?Sized> Draw for &D {
    fn size(&self, t: &Theme) -> (u16, u16) {
        (**self).size(t)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        (**self).draw(t, area, buf)
    }
}
