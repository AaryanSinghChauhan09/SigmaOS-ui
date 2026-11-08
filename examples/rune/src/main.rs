//! A tiny Omarchy panel in Rune, following the live theme.
//!
//!   cargo run --release                         a window; switch themes while it runs
//!   cargo run --release -- --snap out.png       render once, headless (Rune's tooling)

use omarchy_rune::Theme;
use omarchy_rune::rune::{self, AppSpec, Cx, El, col, row};
use omarchy_rune::ui::{Button, Heading, HeadingLevel, ListRow, Pane, Toggle, Tone};

const WIDTH: f32 = 400.0;

struct App {
    theme: Theme,
    wifi: bool,
}

fn view(s: &App, cx: &mut Cx<App>) -> El {
    let t = &s.theme;
    // Every frame: colors, rounding and font follow the theme without a restart.
    t.apply(cx);
    let pad = t.tokens().spacing.panel_padding as f32;
    let inner = WIDTH - 4.0 * pad;

    let status = if s.wifi { ("Connected", Tone::Success) } else { ("Saved", Tone::Neutral) };
    let panel = Pane::new()
        .bordered(true)
        .width(WIDTH - 2.0 * pad)
        .child(Heading::new("Network").subtitle("Wi-Fi and saved networks").level(HeadingLevel::Heading).build(t))
        .child(
            Toggle::new("Wi-Fi")
                .description(if s.wifi { "On" } else { "Off" })
                .checked(s.wifi)
                .width(inner)
                .on_toggle(|s: &mut App| s.wifi ^= true)
                .build(t),
        )
        .child(ListRow::new("Basecamp").glyph("\u{F0928}").status(status.0, status.1).value("5 GHz").current(s.wifi).width(inner).build(t))
        .child(row().child(Button::new("Forget network").bordered(true).build(t)))
        .build(t);

    col().w_full().h_full().fill(t.bg()).padding(pad).child(panel)
}

fn init() -> App {
    App { theme: Theme::load(), wifi: true }
}

fn main() {
    rune::run(AppSpec {
        families: Theme::load().families(),
        setup: Some(|s: &mut App, wake| s.theme.follow(wake)),
        poll: Some(|s: &mut App, _, _| s.theme.poll()),
        ..AppSpec::new("Network", (WIDTH, 283.0), init, view)
    });
}
