//! Data and layout primitives: Table, Tree, SplitView, Sidebar and Calendar. Same anatomy and
//! metrics as the Qt binding's `Table.qml`, `Tree.qml`, `SplitView.qml`, `Sidebar.qml` and
//! `Calendar.qml` (spec/components/data.md).

mod calendar;
mod sidebar;
mod split;
mod table;
mod tree;

pub use calendar::{Calendar, Date};
pub use sidebar::{Sidebar, SidebarItem, SidebarSection};
pub use split::{SplitPane, SplitView};
pub use table::{Column, Table};
pub use tree::{Tree, TreeNode};

use omarchy_theme::{BorderSpec, Rgba};
use rune::{El, el, row};

use super::{Look, chrome};
use crate::Theme;

/// The width Qt's `Text` gives a glyph: its advance widened by ink overhanging either side, in
/// whole pixels (as `ui::glyph` boxes it).
pub(crate) fn glyph_width(t: &Theme, s: &str, size: i64) -> f32 {
    let style = t.text_style(size);
    let advance = rune::text::advance(s, style);
    let Some([x, _, w, _]) = rune::text::ink(s, style).map(|i| i.bounds) else {
        return advance;
    };
    let left = (-x).max(0.0).ceil();
    let right = if x + w > advance {
        ((x + w).ceil() - advance).ceil()
    } else {
        0.0
    };
    advance + left + right
}

/// A list row's chrome: transparent or the selected fill at rest, the hover fill under the
/// pointer, keyboard focus or `cursor` (the hover look replaces the selected one, as the state
/// ladder says).
pub(crate) fn row_chrome(
    t: &Theme,
    base: El,
    children: Vec<El>,
    selected: bool,
    cursor: bool,
) -> El {
    let c = &t.tokens().controls;
    let rest = Look::new(
        if selected {
            c.selected.fill
        } else {
            Rgba::TRANSPARENT
        },
        BorderSpec::none(),
    );
    let hot = Look::new(c.hover.fill, BorderSpec::none());
    chrome(t, base, children, &rest, &hot, cursor, t.radius())
}

/// A hairline in the track color along the bottom of its parent.
pub(crate) fn bottom_rule(t: &Theme) -> El {
    let hair = t.tokens().spacing.hairline as f32;
    el().pin(Some(0.0), None, Some(0.0), Some(0.0))
        .h(hair)
        .fill(t.track())
}

/// A slot `width` wide holding `glyph` centered on whole pixels, as Qt's
/// `x: Math.round((slot - width) / 2)` places one.
pub(crate) fn centered_glyph(t: &Theme, s: &str, size: i64, ink: rune::Color, width: f32) -> El {
    let left = ((width - glyph_width(t, s, size)) / 2.0).round().max(0.0);
    row()
        .w(width)
        .h_full()
        .items_center()
        .pl(left)
        .child(super::glyph(t, s, size, ink))
}
