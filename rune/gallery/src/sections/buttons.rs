//! The `buttons` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Button, ButtonGroup, IconButton};
use rune::{El, row};

use crate::State;

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let gap = t.tokens().spacing.control_gap as f32;
    vec![
        row()
            .items_center()
            .gap(gap)
            .child(Button::new("Plain").build(t))
            .child(Button::new("Bordered").bordered(true).build(t))
            .child(Button::new("Hover").bordered(true).cursor(true).build(t))
            .child(Button::new("Selected").bordered(true).selected(true).build(t))
            .child(Button::new("Icon").icon("\u{f013}").bordered(true).build(t))
            .child(IconButton::new("\u{f011}").label("Power").bordered(true).build(t))
            .child(IconButton::new("\u{f021}").label("Refresh").bordered(true).cursor(true).build(t)),
        ButtonGroup::new([("left", "Left"), ("center", "Center"), ("right", "Right")], s.align.clone())
            .on_choose(|s: &mut State, v: &str| s.align = v.into())
            .build(t),
    ]
}
