//! The `surfaces` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{
    Card, ConfirmDialog, Heading, HeadingLevel, IconButton, MenuRow, Pane, SettingRow, Side, Slider, Toggle, ToggleSwitch, Tooltip, separator,
};

use crate::Gallery;

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = t.tokens().spacing.clone();
    let huge = px(sp.huge as f32);
    let network = |id: &'static str, label: &'static str, g: &Gallery, cx: &mut Context<Gallery>| {
        MenuRow::new(label).checked(g.network == id).on_click(cx.listener(move |g: &mut Gallery, _, _, cx| {
            g.network = id.into();
            cx.notify();
        }))
    };
    let card = Card::new()
        .title("Network")
        .width(300.0)
        .child(Toggle::new("Wi-Fi").id("card-wifi").description("Basecamp").checked(g.wifi).on_toggle(cx.listener(|g: &mut Gallery, v: &bool, _, cx| {
            g.wifi = *v;
            cx.notify();
        })))
        .child(separator(&t))
        .child(
            div()
                .flex()
                .flex_col()
                .w_full()
                .child(network("basecamp", "Basecamp", g, cx))
                .child(network("guest", "Basecamp Guest", g, cx).highlighted(true))
                .child(network("coffee", "Coffee Shop", g, cx)),
        );
    let side = div()
        .flex()
        .flex_col()
        .gap(huge)
        .items_start()
        .child(Tooltip::new("Restart Wi-Fi"))
        .child(div().flex().w(px(340.0)).h(px(170.0)).items_center().justify_center().child(ConfirmDialog::new("Remove the Coffee Shop network?").width(308.0)));

    // Panes: an app's side panel with a rule against its neighbour, and a bordered settings group.
    let layers = Pane::new()
        .title("Layers")
        .subtitle("3 items")
        .divider(Side::Left)
        .action(IconButton::new("\u{F067}"))
        .width(240.0)
        .child(
            div()
                .flex()
                .flex_col()
                .w_full()
                .child(MenuRow::new("Background"))
                .child(MenuRow::new("Title").checked(true))
                .child(MenuRow::new("Logo")),
        );
    let night = Pane::new()
        .title("Night light")
        .subtitle("Sunset to sunrise")
        .bordered(true)
        .action(ToggleSwitch::new("night-light", g.night_light).on_toggle(cx.listener(|g: &mut Gallery, v: &bool, _, cx| {
            g.night_light = *v;
            cx.notify();
        })))
        .width(240.0)
        .child(SettingRow::new("Strength").control(Slider::new("night-strength", g.night_strength).width(100.0).on_change(cx.listener(
            |g: &mut Gallery, v: &f32, _, cx| {
                g.night_strength = *v;
                cx.notify();
            },
        ))));
    let row = || div().flex().flex_row().items_start().gap(huge);
    let col = |w: f32| div().flex().flex_col().w(px(w)).gap(huge);

    // Headings: the three levels, a trailing action, and a toolbar's centered title.
    let levels = col(240.0)
        .child(Heading::new("Settings").subtitle("Omarchy").level(HeadingLevel::Display))
        .child(Heading::new("Appearance").subtitle("Theme, font and rounding").level(HeadingLevel::Heading));
    let variants = col(240.0)
        .child(Heading::new("Frame").trailing(IconButton::new("\u{F141}")))
        .child(Heading::new("notes.md").subtitle("~/Documents").centered(true));

    vec![
        row().child(card).child(side).into_any_element(),
        row().child(layers).child(night).into_any_element(),
        row().child(levels).child(variants).into_any_element(),
    ]
}
