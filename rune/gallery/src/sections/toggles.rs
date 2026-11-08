//! The `toggles` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Toggle, ToggleSwitch};
use rune::{El, row};

use crate::State;

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let gap = t.tokens().spacing.xxl as f32;
    let mut switches = row().items_center().gap(gap);
    for i in 0..4 {
        switches = switches.child(
            ToggleSwitch::new(s.switches[i])
                .key(format!("switch-{i}"))
                .cursor(i >= 2)
                .on_toggle(move |s: &mut State| s.switches[i] ^= true)
                .build(t),
        );
    }
    vec![
        switches,
        Toggle::new("Wi-Fi")
            .description("Connected to Basecamp")
            .checked(s.wifi)
            .width(360.0)
            .on_toggle(|s: &mut State| s.wifi ^= true)
            .build(t),
        Toggle::new("Bluetooth")
            .description("Off")
            .checked(s.bluetooth)
            .cursor(true)
            .width(360.0)
            .on_toggle(|s: &mut State| s.bluetooth ^= true)
            .build(t),
    ]
}
