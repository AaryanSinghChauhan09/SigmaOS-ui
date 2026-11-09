//! The `slider` section (spec/gallery.md).

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::Slider;

use crate::Gallery;

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    vec![
        Slider::new("level", g.level)
            .width(360.0)
            .on_change(cx.listener(|g: &mut Gallery, v: &f32, _, cx| {
                g.level = *v;
                cx.notify();
            }))
            .into_any_element(),
        Slider::new("stepped", g.stepped)
            .ticks(5)
            .step(0.25)
            .width(360.0)
            .on_change(cx.listener(|g: &mut Gallery, v: &f32, _, cx| {
                g.stepped = (v * 4.0).round() / 4.0;
                cx.notify();
            }))
            .into_any_element(),
    ]
}
