//! Every component of `spec/components.md`, under the same names, drawn on the cell grid as
//! `spec/terminal.md` maps them. Each is a builder implementing [`crate::Draw`]: `.with(&theme)`
//! makes it a Ratatui widget. Components are views of the app's state (the app owns selection,
//! cursors and values); the key handlers that move that state live beside them
//! ([`CursorModel`], [`Menu::handle_key`], ...).

use omarchy_theme::Rgba;

use crate::Theme;

mod controls;
mod data;
mod editing;
mod feedback;
mod forms;
mod lists;
mod navigation;
mod search;
mod surfaces;

pub use controls::*;
pub use data::*;
pub use editing::*;
pub use feedback::*;
pub use forms::*;
pub use lists::*;
pub use navigation::*;
pub use search::*;
pub use surfaces::*;

/// A component's color by meaning: neutral (its own ink), accent, success, warning, urgent, muted.
/// The feedback components' names are aliases: `INFO` is accent, `ERROR` urgent.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Tone {
    #[default]
    Neutral,
    Accent,
    Success,
    Warning,
    Urgent,
    Muted,
}

impl Tone {
    pub const INFO: Tone = Tone::Accent;
    pub const ERROR: Tone = Tone::Urgent;

    /// The tone's color; `neutral` for [`Tone::Neutral`].
    pub fn color(self, t: &Theme, neutral: Rgba) -> Rgba {
        match self {
            Tone::Neutral => neutral,
            Tone::Accent => t.accent(),
            Tone::Success => t.success(),
            Tone::Warning => t.warning(),
            Tone::Urgent => t.urgent(),
            Tone::Muted => t.muted(),
        }
    }
}

/// How a control looks in its state (spec/terminal.md, States): the fill over its padded inside,
/// the color of its edges (None: no edges shown), its label ink and weight.
#[derive(Clone, Copy, Debug)]
pub struct Look {
    pub fill: Rgba,
    pub border: Option<Rgba>,
    pub text: Rgba,
    pub bold: bool,
}

/// The border color of a state, when it has a width.
pub(crate) fn border_of(b: &omarchy_theme::BorderSpec) -> Option<Rgba> {
    (!b.is_none()).then_some(b.color)
}

/// pressed > focus > hover-cursor > selected > normal, for a control that shows `rest_border`
/// (the normal border, or none for a plain one) and `rest_fill` at rest.
pub fn look(t: &Theme, hot: bool, selected: bool, rest_fill: bool, rest_border: bool) -> Look {
    let c = &t.shown().controls;
    if hot {
        return Look { fill: c.hover.fill, border: border_of(&c.hover.border), text: if selected { c.selected.text } else { t.fg() }, bold: selected };
    }
    if selected {
        let border = border_of(&c.selected.border).or(if rest_border { border_of(&c.normal.border) } else { None });
        return Look { fill: c.selected.fill, border, text: c.selected.text, bold: true };
    }
    Look {
        fill: if rest_fill { c.normal.fill } else { Rgba::TRANSPARENT },
        border: if rest_border { border_of(&c.normal.border) } else { None },
        text: t.fg(),
        bold: false,
    }
}


/// Spinner frames, chevrons and marks the components share.
pub mod glyphs {
    /// Menus' check, Checkbox's mark.
    pub const CHECK: &str = "✓";
    /// A chosen radio.
    pub const DOT: &str = "●";
    /// Dropdown and MultiSelect chevron (U+F0140).
    pub const CHEVRON_DOWN: &str = "\u{f0140}";
    /// Tree, a closed branch (U+F0142), and the Calendar's next (U+F0142).
    pub const CHEVRON_RIGHT: &str = "\u{f0142}";
    /// The Calendar's previous (U+F0141).
    pub const CHEVRON_LEFT: &str = "\u{f0141}";
    /// Table, ascending (U+F0143).
    pub const CHEVRON_UP: &str = "\u{f0143}";
    /// A submenu row's chevron (U+F054).
    pub const SUBMENU: &str = "\u{f054}";
    /// Dismiss (U+F0156).
    pub const CLOSE: &str = "\u{f0156}";
    /// The pulse glyph (U+F0996).
    pub const LOADING: &str = "\u{f0996}";
    /// A turning spinner's frames, one per 90 ms (900 ms a turn).
    pub const SPINNER: [&str; 10] = ["⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"];
}
