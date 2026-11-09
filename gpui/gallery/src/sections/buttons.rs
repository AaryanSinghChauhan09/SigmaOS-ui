//! The `buttons` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Button, ButtonGroup, IconButton};

use crate::Gallery;

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let gap = t.tokens().spacing.control_gap as f32;
    vec![
        div()
            .flex()
            .flex_row()
            .items_center()
            .gap(px(gap))
            .child(Button::new("Plain"))
            .child(Button::new("Bordered").bordered(true))
            .child(Button::new("Hover").bordered(true).cursor(true))
            .child(Button::new("Selected").bordered(true).selected(true))
            .child(Button::new("Icon").icon("\u{f013}").bordered(true))
            .child(IconButton::new("\u{f011}").bordered(true))
            .child(IconButton::new("\u{f021}").bordered(true).cursor(true))
            .into_any_element(),
        ButtonGroup::new([("left", "Left"), ("center", "Center"), ("right", "Right")], g.align.clone())
            .on_choose(cx.listener(|g: &mut Gallery, v: &SharedString, _, cx| {
                g.align = v.clone();
                cx.notify();
            }))
            .into_any_element(),
    ]
}
