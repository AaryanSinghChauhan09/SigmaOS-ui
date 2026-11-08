//! Omarchy's primitives for Rune, drawn the way the Omarchy shell's QML kit draws them (the Qt
//! binding in this repo is that kit). Same names, states and metrics as `spec/components.md`.
//!
//! Every builder ends in `.build(&theme)` and returns an [`El`]. Handlers attach through the
//! builder (`on_press`, `on_toggle`, `on_change`). `.cursor(true)` forces the hover-cursor look,
//! as the shell's `hasCursor` does for keyboard-driven panels.

mod button;
mod data;
mod field;
mod slider;
mod surface;
mod toggle;

pub use button::{Button, ButtonGroup, IconButton};
pub use data::{Calendar, Column, Date, Pane, Sidebar, SidebarItem, SidebarSection, SplitView, Table, Tree, TreeNode};
pub use field::{Dropdown, NumberField, TextField};
pub use slider::Slider;
pub use surface::{Card, ConfirmDialog, MenuRow, Tooltip, section_header, separator};
pub use toggle::{Toggle, ToggleSwitch};

use omarchy_theme::{BorderSpec, Rgba};
use rune::{El, el, text};

use crate::{Theme, border, color};

/// A handler waiting for the app's state type: builders keep these rather than being generic
/// themselves, so `Button::new("Save")` needs no type annotation.
pub(crate) type Attach = Box<dyn FnOnce(El) -> El>;

/// One visual state of a control: fill and border.
#[derive(Clone)]
pub(crate) struct Look {
    pub fill: Rgba,
    pub border: BorderSpec,
}

impl Look {
    pub fn new(fill: Rgba, border: BorderSpec) -> Look {
        Look { fill, border }
    }
    pub fn clear() -> Look {
        Look { fill: Rgba::TRANSPARENT, border: BorderSpec::none() }
    }
}

/// The widest border any of `looks` paints, per side, so changing state never relayouts (the
/// shell reserves the same way).
pub(crate) fn reserve(looks: &[&BorderSpec]) -> [f32; 4] {
    let mut out = [0.0f32; 4];
    for spec in looks {
        for (i, w) in spec.widths.iter().enumerate() {
            out[i] = out[i].max(*w as f32);
        }
    }
    out
}

/// Control chrome: the `rest` look, swapped for the `hot` look under the pointer or keyboard
/// focus (Omarchy's focus mirrors hover), or always when `forced`. Both looks are layers under
/// `children`, cross-faded by the hover group, so the hot look replaces the rest look the way
/// the shell's state ladder does rather than stacking on it. `base` is the laid-out container
/// without children.
pub(crate) fn chrome(t: &Theme, base: El, children: Vec<El>, rest: &Look, hot: &Look, forced: bool, radius: f32) -> El {
    let layer = |look: &Look| {
        let e = el().pin(Some(0.0), Some(0.0), Some(0.0), Some(0.0)).radius(radius).fill(color(look.fill));
        border(e, &look.border)
    };
    let pressed = color(t.tokens().controls.pressed.fill);
    let base = base.hover_group_focus().radius(radius).press(pressed);
    let base = if forced {
        base.child(layer(hot))
    } else {
        base.child(layer(rest).group_opacity(1.0, 0.0)).child(layer(hot).group_opacity(0.0, 1.0))
    };
    base.children(children)
}

/// A glyph boxed the way Qt's `Text` boxes one: the advance, widened by any ink that overhangs
/// it, in whole pixels as Qt's hinted bearings are. Nerd Font icons in the non-Mono Nerd Fonts
/// overhang their cell to the right, so centering the bare advance (as Rune lays text out) leaves
/// the icon right of center.
pub(crate) fn glyph(t: &Theme, s: &str, size: i64, ink: rune::Color) -> El {
    let style = t.text_style(size);
    let e = text(s.to_string()).font(style).color(ink).single_line();
    let Some([x, _, w, _]) = rune::text::ink(s, style).map(|i| i.bounds) else { return e };
    let advance = rune::text::advance(s, style);
    let left = (-x).max(0.0).ceil();
    let right = if x + w > advance { ((x + w).ceil() - advance).ceil() } else { 0.0 };
    if left == 0.0 && right == 0.0 {
        return e;
    }
    let (_, h) = rune::text::measure(s, &[], style, None);
    el().w(advance + left + right).h(h).child(e.offset(left, 0.0))
}
