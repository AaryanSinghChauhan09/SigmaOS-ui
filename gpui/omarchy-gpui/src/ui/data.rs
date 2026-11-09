//! Data and layout (spec/components/data.md): Table, Tree, SplitView, Sidebar, Calendar. Same
//! anatomy, metrics and keys as the Qt binding's `Table.qml`, `Tree.qml`, `SplitView.qml`,
//! `Sidebar.qml` and `Calendar.qml`.
//!
//! The app owns what is chosen (selection, sort, expanded nodes, sizes, the month); each
//! component asks for changes through its `on_*` handlers. Like Qt's, each is one focus stop
//! that keeps its own keyboard cursor: the pointer moves it too, so one row is lit at a time.
//! `.cursor(..)` forces the hover look on a row without focus (Qt's `hasCursor` + `cursor*`),
//! and that row is where the keyboard cursor starts.
//!
//! `.id(..)` names a component's state and focus; in debug builds its parts carry debug
//! selectors for tests (`<id>-<row>`, `<id>-col-<n>`, `<id>-<node>-toggle`, `<id>-handle-<n>`,
//! `<id>-YYYY-MM-DD`, `<id>-prev`, `<id>-next`).

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

use gpui::prelude::*;
use gpui::{Div, Hsla, div, px};
use omarchy_theme::Rgba;

use crate::{Paint, color};

/// A component's keyboard cursor, kept across frames: where it is, whether the pointer is over
/// the component (which shows it, as focus does), and the forced cursor the app last passed
/// (a new one moves the cursor there).
pub(crate) struct Walk<K> {
    pub cursor: Option<K>,
    pub hovering: bool,
    pub forced: Option<K>,
}

impl<K: Clone + PartialEq> Walk<K> {
    pub fn new(forced: Option<K>) -> Walk<K> {
        Walk { cursor: forced.clone(), hovering: false, forced }
    }
    /// Take the app's forced cursor when it changed since the last frame.
    pub fn sync(&mut self, forced: &Option<K>) {
        if self.forced != *forced {
            self.forced = forced.clone();
            if forced.is_some() {
                self.cursor = forced.clone();
            }
        }
    }
}

/// A row's fill on the state ladder: the hover fill on the lit cursor row, else the selected
/// fill when selected, else nothing (hover replaces selected, it does not stack).
pub(crate) fn row_fill(t: &Paint, selected: bool, hot: bool) -> Hsla {
    let c = &t.tokens().controls;
    if hot {
        color(c.hover.fill)
    } else if selected {
        color(c.selected.fill)
    } else {
        color(Rgba::TRANSPARENT)
    }
}

/// A hairline in the track color along the bottom of its (relative) parent.
pub(crate) fn bottom_rule(t: &Paint) -> Div {
    let hair = t.tokens().spacing.hairline as f32;
    div().absolute().left_0().right_0().bottom_0().h(px(hair)).bg(t.track())
}

/// Ink for a row's text: the selected-state color when selected.
pub(crate) fn row_ink(t: &Paint, selected: bool) -> Hsla {
    if selected { color(t.tokens().controls.selected.text) } else { t.fg() }
}

/// A key name for the walk: arrows, Home/End, PageUp/PageDown, and Return/Space as "choose".
pub(crate) fn walk_key(ev: &gpui::KeyDownEvent) -> Option<&str> {
    if ev.keystroke.modifiers.modified() {
        return None;
    }
    Some(match ev.keystroke.key.as_str() {
        "up" => "up",
        "down" => "down",
        "left" => "left",
        "right" => "right",
        "home" => "home",
        "end" => "end",
        "pageup" => "pageup",
        "pagedown" => "pagedown",
        "enter" | "kp_enter" | "space" => "choose",
        _ => return None,
    })
}
