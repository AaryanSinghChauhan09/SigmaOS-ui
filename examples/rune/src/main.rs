//! A tiny Omarchy panel in Rune, following the live theme.
//!
//!   cargo run --release                         a window; switch themes while it runs
//!   cargo run --release -- --snap out.png       render once, headless (Rune's tooling)

use omarchy_rune::rune::{AppSpec, Cx, El, col, row};
use omarchy_rune::{Theme, Themed};
use omarchy_rune::ui::{Button, Heading, HeadingLevel, ListRow, Pane, Toggle, Tone};

const WIDTH: f32 = 400.0;

struct App {
    theme: Theme,
    wifi: bool,
}

// Where the theme lives, so omarchy_rune::run can follow the desktop for the app.
impl Themed for App {
    fn theme(&self) -> &Theme {
        &self.theme
    }
    fn theme_mut(&mut self) -> &mut Theme {
        &mut self.theme
    }
}

fn view(s: &App, _: &mut Cx<App>) -> El {
    let t = &s.theme;
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
    // rune::run with the theme followed: colors fade, rounding and fonts switch, on every
    // `omarchy theme set`, font change, text size change and Hyprland reload.
    omarchy_rune::run(AppSpec::new("Network", (WIDTH, 283.0), init, view));
}
