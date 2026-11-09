//! Navigation (spec/components/navigation.md): Menu, ContextMenu, Keycap, KeyCombo, ShortcutSheet, Toolbar, Tabs, IconRail.

use gpui::prelude::*;
use gpui::{AnyElement, SharedString};

use crate::Paint;

/// A tooltip's shortcut after its text (a KeyCombo once navigation has one).
pub(crate) fn tooltip_shortcut(t: &Paint, combo: SharedString) -> AnyElement {
    super::label(t, combo, t.tokens().font.caption, false, t.dim()).into_any_element()
}
