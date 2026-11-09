//! The live-switch test (spec/following.md), run under `tools/live/run`: an app on the kit's own
//! loop (`omarchy_ratatui::run_on`, a `TestBackend` instead of the terminal) follows the theme, the
//! theme is switched the way `omarchy theme set` does it, and the app must see catppuccin-latte
//! through the public API, the changed hook once with the new tokens, revision 1, a fade that ran
//! and landed exactly (the frame's painted cell too); under `OMARCHY_UI_REDUCE_MOTION=1`, no fade.

use std::time::{Duration, Instant};

use omarchy_ratatui::crossterm::event::Event;
use omarchy_ratatui::omarchy_theme::Rgba;
use omarchy_ratatui::ratatui::backend::TestBackend;
use omarchy_ratatui::ratatui::style::Color;
use omarchy_ratatui::ratatui::{Frame, Terminal};
use omarchy_ratatui::{App, Depth, Flow, Theme, Themed, Tokens, paint};

const LATTE: &str = "#eff1f5ff";

fn fail(msg: String) -> ! {
    eprintln!("live switch: FAIL: {msg}");
    std::process::exit(1);
}

struct Live {
    theme: Theme,
    home: String,
    switch: String,
    started: Instant,
    switched: Option<Instant>,
    start_bg: Rgba,
    hooks: Vec<String>,
    faded: bool,
    between: bool,
    landed: Option<Instant>,
    /// The accent cell the last frame painted.
    painted: Option<Color>,
}

impl Themed for Live {
    fn theme(&self) -> &Theme {
        &self.theme
    }
    fn theme_mut(&mut self) -> &mut Theme {
        &mut self.theme
    }
    fn theme_changed(&mut self, tokens: &Tokens) {
        // After the new tokens are in effect: the public API already reads them.
        if self.theme.tokens().palette.background != tokens.palette.background {
            fail("the hook ran before the new tokens were in effect".into());
        }
        self.hooks.push(tokens.palette.background.hex());
    }
}

impl App for Live {
    fn draw(&mut self, frame: &mut Frame) {
        let t = &self.theme;
        paint::put(t, frame.buffer_mut(), 0, 0, "█", t.accent());
        let shown = t.shown().palette.background;
        self.faded |= t.fading();
        self.between |= t.tokens().palette.background.hex() == LATTE && shown != self.start_bg && shown != t.tokens().palette.background;
    }
    fn event(&mut self, _: &Event) -> Flow {
        Flow::Continue
    }
    fn tick(&mut self) -> Flow {
        if self.switched.is_none() && self.started.elapsed() > Duration::from_millis(300) {
            let ok = std::process::Command::new(&self.switch).arg(&self.home).arg("catppuccin-latte").status().is_ok_and(|s| s.success());
            if !ok {
                fail(format!("{} failed", self.switch));
            }
            self.switched = Some(Instant::now());
        }
        let Some(at) = self.switched else { return Flow::Continue };
        let t = &self.theme;
        if self.landed.is_none() && t.tokens().palette.background.hex() == LATTE && !t.fading() {
            self.landed = Some(Instant::now());
        }
        if let Some(l) = self.landed {
            // Another 600 ms: the hook must still have run once.
            return if l.elapsed() > Duration::from_millis(600) { Flow::Quit } else { Flow::Continue };
        }
        if at.elapsed() > Duration::from_secs(5) {
            fail(format!("background is {} after 5 s, not {LATTE}", t.tokens().palette.background.hex()));
        }
        Flow::Continue
    }
}

fn main() {
    let home = std::env::var("OMARCHY_UI_LIVE_HOME").unwrap_or_else(|_| fail("run me under tools/live/run".into()));
    let switch = std::env::var("OMARCHY_UI_LIVE_SWITCH").unwrap_or_else(|_| fail("no OMARCHY_UI_LIVE_SWITCH".into()));
    let reduced = std::env::var("OMARCHY_UI_REDUCE_MOTION").is_ok_and(|v| v == "1");
    let mut theme = Theme::load();
    theme.set_depth(Depth::True);
    theme.follow();
    let start_bg = theme.tokens().palette.background;
    let mut app = Live { theme, home, switch, started: Instant::now(), switched: None, start_bg, hooks: Vec::new(), faded: false, between: false, landed: None, painted: None };
    let mut term = Terminal::new(TestBackend::new(4, 1)).unwrap();
    let mut next = |timeout: Duration| -> std::io::Result<Option<Event>> {
        std::thread::sleep(timeout);
        Ok(None)
    };
    omarchy_ratatui::run_on(&mut term, &mut app, &mut next).unwrap_or_else(|e| fail(e.to_string()));
    // The last frame drawn: the accent cell holds the new accent exactly.
    app.painted = Some(term.backend().buffer()[(0, 0)].fg);
    let t = &app.theme;
    let accent = t.tokens().palette.accent;
    if app.painted != Some(Color::Rgb(accent.r, accent.g, accent.b)) {
        fail(format!("the frame shows {:?}, not the new accent {}", app.painted, accent.hex()));
    }
    if t.shown().palette.background != t.tokens().palette.background {
        fail("the fade didn't land exactly".into());
    }
    if app.hooks != [LATTE] {
        fail(format!("changed hook fired {:?}, want once with {LATTE}", app.hooks));
    }
    if t.revision() != 1 {
        fail(format!("revision {}, want 1", t.revision()));
    }
    if reduced && (app.faded || app.between) {
        fail("a fade ran under reduced motion".into());
    }
    if !reduced && !(app.faded && app.between) {
        fail(format!("no fade seen (fading {}, a color between {})", app.faded, app.between));
    }
    let ms = app.landed.zip(app.switched).map_or(0, |(l, s)| l.duration_since(s).as_millis());
    println!("live switch: {LATTE} in {ms} ms, hook once, revision 1, {}", if reduced { "no fade (reduced motion)" } else { "faded and landed exactly" });
}
