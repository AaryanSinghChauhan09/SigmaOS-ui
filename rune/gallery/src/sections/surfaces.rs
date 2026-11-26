//! The `surfaces` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{
    Card, ConfirmDialog, Heading, HeadingLevel, IconButton, MenuRow, Pane, SettingRow, Slider,
    Toggle, ToggleSwitch, Tooltip, separator,
};
use rune::{El, Side, col, el, row};

use crate::State;

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let network = |id: &'static str, label: &str| {
        MenuRow::new(label)
            .checked(s.network == id)
            .on_press(move |s: &mut State| s.network = id.into())
    };
    let card = Card::new()
        .title("Network")
        .width(300.0)
        .child(
            Toggle::new("Wi-Fi")
                .description("Basecamp")
                .checked(s.wifi)
                .on_toggle(|s: &mut State| s.wifi ^= true)
                .build(t),
        )
        .child(separator(t))
        .child(
            col()
                .w_full()
                .child(network("basecamp", "Basecamp").build(t))
                .child(
                    network("guest", "Basecamp Guest")
                        .highlighted(true)
                        .build(t),
                )
                .child(network("coffee", "Coffee Shop").build(t)),
        )
        .build(t);
    let side = col()
        .gap(sp.huge as f32)
        .items_start()
        .child(Tooltip::new("Restart Wi-Fi").build(t))
        .child(
            el().size(340.0, 170.0).center().child(
                ConfirmDialog::new("Remove the Coffee Shop network?")
                    .width(308.0)
                    .build(t),
            ),
        );

    // Panes: an app's side panel with a rule against its neighbour, and a bordered settings group.
    let layers = Pane::new()
        .title("Layers")
        .subtitle("3 items")
        .divider(Side::Left)
        .actions([IconButton::new("\u{F067}").build(t)])
        .width(240.0)
        .child(
            col()
                .w_full()
                .child(MenuRow::new("Background").build(t))
                .child(MenuRow::new("Title").checked(true).build(t))
                .child(MenuRow::new("Logo").build(t)),
        )
        .build(t);
    let night = Pane::new()
        .title("Night light")
        .subtitle("Sunset to sunrise")
        .bordered(true)
        .actions([ToggleSwitch::new(s.night_light)
            .key("night-light")
            .on_toggle(|s: &mut State| s.night_light ^= true)
            .build(t)])
        .width(240.0)
        .child(
            SettingRow::new("Strength")
                .control(
                    Slider::new(s.night_strength)
                        .width(100.0)
                        .on_change(|s: &mut State, v| s.night_strength = v)
                        .build(t),
                )
                .build(t),
        )
        .build(t);
    let panes = row()
        .items_start()
        .gap(sp.huge as f32)
        .child(layers)
        .child(night);

    // Headings: the three levels, a trailing action, and a toolbar's centered title.
    let levels = col()
        .w(240.0)
        .gap(sp.huge as f32)
        .child(
            Heading::new("Settings")
                .subtitle("Omarchy")
                .level(HeadingLevel::Display)
                .build(t)
                .w_full(),
        )
        .child(
            Heading::new("Appearance")
                .subtitle("Theme, font and rounding")
                .level(HeadingLevel::Heading)
                .build(t)
                .w_full(),
        );
    let variants = col()
        .w(240.0)
        .gap(sp.huge as f32)
        .child(
            Heading::new("Frame")
                .trailing([IconButton::new("\u{F141}").build(t)])
                .build(t)
                .w_full(),
        )
        .child(
            Heading::new("notes.md")
                .subtitle("~/Documents")
                .centered(true)
                .build(t)
                .w_full(),
        );
    let headings = row()
        .items_start()
        .gap(sp.huge as f32)
        .child(levels)
        .child(variants);

    vec![
        row()
            .items_start()
            .gap(sp.huge as f32)
            .child(card)
            .child(side),
        panes,
        headings,
    ]
}
