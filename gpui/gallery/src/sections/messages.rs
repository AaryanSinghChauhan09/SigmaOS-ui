//! The `messages` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Banner, EmptyState, FeedbackTone, Toast};

use crate::Gallery;

/// Which banners have been dismissed.
pub struct State {
    pub dismissed: [bool; 4],
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State { dismissed: [false; 4] }
    }
}

pub fn build(g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let gap = px(t.tokens().spacing.huge as f32);
    let boxed = |e: AnyElement| div().flex().flex_shrink_0().w(px(350.0)).h(px(150.0)).items_center().justify_center().child(e);
    let mut out = vec![
        div()
            .flex()
            .flex_row()
            .gap(gap)
            .child(boxed(
                EmptyState::new("No networks found").glyph("\u{f05aa}").hint("Turn on Wi-Fi to scan").action("Scan").into_any_element(),
            ))
            .child(boxed(
                EmptyState::new("Clipboard is empty")
                    .glyph("\u{f014c}")
                    .hint("Copy something to see it here")
                    .overlay(true)
                    .into_any_element(),
            ))
            .into_any_element(),
    ];
    let banners = [
        (FeedbackTone::Info, "Omarchy 3.2 is available", Some("Update")),
        (FeedbackTone::Success, "Backup finished", None),
        (FeedbackTone::Warning, "Battery is low (12%)", None),
        (FeedbackTone::Error, "Could not reach the update server", None),
    ];
    for (i, (tone, text, action)) in banners.into_iter().enumerate() {
        if g.more.messages.dismissed[i] {
            continue;
        }
        let mut b = Banner::new(text).tone(tone).dismissible(true).on_dismiss(cx.listener(move |g: &mut Gallery, _, _, cx| {
            g.more.messages.dismissed[i] = true;
            cx.notify();
        }));
        if let Some(a) = action {
            b = b.action(a);
        }
        out.push(b.into_any_element());
    }
    // A toast as it draws, held open (the app owns when it shows and the timer).
    out.push(Toast::new("Export finished").glyph("\u{f012c}").text("omarchy-demo.mp4 · 42 MB").action("Open").countdown(0.6).into_any_element());
    out
}
