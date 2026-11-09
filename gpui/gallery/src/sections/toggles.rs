//! The `toggles` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Toggle, ToggleSwitch};

use crate::Gallery;

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let gap = t.tokens().spacing.xxl as f32;
    let mut switches = div().flex().flex_row().items_center().gap(px(gap));
    for i in 0..4 {
        switches = switches.child(
            ToggleSwitch::new(SharedString::from(format!("switch-{i}")), g.switches[i])
                .cursor(i >= 2)
                .on_toggle(cx.listener(move |g: &mut Gallery, v: &bool, _, cx| {
                    g.switches[i] = *v;
                    cx.notify();
                })),
        );
    }
    vec![
        switches.into_any_element(),
        Toggle::new("Wi-Fi")
            .description("Connected to Basecamp")
            .checked(g.wifi)
            .width(360.0)
            .on_toggle(cx.listener(|g: &mut Gallery, v: &bool, _, cx| {
                g.wifi = *v;
                cx.notify();
            }))
            .into_any_element(),
        Toggle::new("Bluetooth")
            .description("Off")
            .checked(g.bluetooth)
            .cursor(true)
            .width(360.0)
            .on_toggle(cx.listener(|g: &mut Gallery, v: &bool, _, cx| {
                g.bluetooth = *v;
                cx.notify();
            }))
            .into_any_element(),
    ]
}
