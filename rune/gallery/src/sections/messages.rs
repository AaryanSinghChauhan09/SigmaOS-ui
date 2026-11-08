//! The `messages` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::feedback::Tone;
use omarchy_rune::ui::{Banner, EmptyState, Toast};
use rune::{El, el, row};

use crate::State;

pub fn build(_s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let gap = t.tokens().spacing.huge as f32;
    let boxed = |e: El| el().size(350.0, 150.0).center().child(e);
    let banner = |tone: Tone, text: &str| Banner::new(text).tone(tone).dismissible(true);
    vec![
        row()
            .gap(gap)
            .child(boxed(
                EmptyState::new("No networks found").glyph("\u{f05aa}").hint("Turn on Wi-Fi to scan").action("Scan").build(t),
            ))
            .child(boxed(
                EmptyState::new("Clipboard is empty").glyph("\u{f014c}").hint("Copy something to see it here").overlay(true).build(t),
            )),
        banner(Tone::Info, "Omarchy 3.2 is available").action("Update").build(t),
        banner(Tone::Success, "Backup finished").build(t),
        banner(Tone::Warning, "Battery is low (12%)").build(t),
        banner(Tone::Error, "Could not reach the update server").build(t),
        // A toast as it draws, held open (the app owns when it shows and the timer).
        Toast::new("Export finished").glyph("\u{f012c}").text("omarchy-demo.mp4 · 42 MB").action("Open").countdown(0.6).build(t),
    ]
}
