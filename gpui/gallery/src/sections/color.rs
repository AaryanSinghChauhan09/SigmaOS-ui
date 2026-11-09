//! The `color` section (spec/gallery.md).

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{ColorPicker, Hsva};

use crate::Gallery;

/// What this section keeps between frames: a fixed color, not the theme's accent, so the
/// picker's content is the same in every theme.
pub struct State {
    pub value: Hsva,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State { value: Hsva::from_hex("#7aa2f7", 0.0).expect("a hex color") }
    }
}

pub fn build(g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    vec![
        ColorPicker::new(g.more.color.value)
            .size(300.0, 260.0)
            .on_change(cx.listener(|g: &mut Gallery, v: &Hsva, _, cx| {
                g.more.color.value = *v;
                cx.notify();
            }))
            .into_any_element(),
    ]
}
