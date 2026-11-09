//! A tiny "Network" panel: the Omarchy look and live theme-following in a GPUI app, with one
//! call (`omarchy_gpui::init`).

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Button, Heading, HeadingLevel, ListRow, Pane, Toggle, Tone};
use omarchy_gpui::{Paint, gpui_platform};

const WIDTH: f32 = 400.0;

struct Network {
    wifi: bool,
}

impl Render for Network {
    fn render(&mut self, _: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        // What's on screen now: the tokens, with colors mid-fade during a theme switch.
        let t = Paint::of(cx);
        let pad = t.tokens().spacing.panel_padding as f32;
        let inner = WIDTH - 4.0 * pad;

        let panel = Pane::new()
            .bordered(true)
            .width(WIDTH - 2.0 * pad)
            .child(Heading::new("Network").subtitle("Wi-Fi and saved networks").level(HeadingLevel::Heading))
            .child(
                Toggle::new("Wi-Fi")
                    .description(if self.wifi { "On" } else { "Off" })
                    .checked(self.wifi)
                    .width(inner)
                    .on_toggle(cx.listener(|this, on: &bool, _, cx| {
                        this.wifi = *on;
                        cx.notify();
                    })),
            )
            .child(ListRow::new("Basecamp").glyph("\u{F0928}").status("Connected", Tone::Success).current(self.wifi).width(inner))
            .child(div().flex().child(Button::new("Forget network").bordered(true)));

        div().size_full().bg(t.bg()).p(px(pad)).child(panel)
    }
}

fn main() {
    #[cfg(feature = "snap")]
    if omarchy_gpui::snap::from_args((WIDTH, 283.0), |_, _| Network { wifi: true }) {
        return;
    }
    gpui_platform::application().run(|cx| {
        // The one call: the theme, following the desktop, the font and the color fade.
        omarchy_gpui::init(cx);
        let bounds = Bounds::centered(None, size(px(WIDTH), px(283.0)), cx);
        cx.open_window(
            WindowOptions { window_bounds: Some(WindowBounds::Windowed(bounds)), app_id: Some("network".into()), ..Default::default() },
            |window, cx| {
                window.set_window_title("Network");
                cx.new(|_| Network { wifi: true })
            },
        )
        .expect("open the window");
        cx.activate(true);
    });
}
