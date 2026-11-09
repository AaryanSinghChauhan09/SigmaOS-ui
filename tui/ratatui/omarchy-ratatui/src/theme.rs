//! The live Omarchy theme for a Ratatui app (`spec/following.md`, `spec/terminal.md`).
//!
//! [`Theme`] holds the tokens in effect (the target of any fade), what is on screen (the same
//! tokens with colors part-way through a theme fade), the revision and the watch. Components draw
//! from [`Theme::shown`]; an app's own drawing that should fade with them reads colors there too.

use std::cell::{Cell as StdCell, RefCell};
use std::sync::Arc;
use std::sync::mpsc;
use std::time::{Duration, Instant};

use omarchy_theme::{Rgba, Tokens};
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;
use ratatui::style::{Color, Modifier};

use crate::depth::{self, Depth};
use crate::fade::{cubic_bezier, mix_tokens};

/// The theme in effect. Keep one in the app's state (`Theme::load()`) and implement
/// [`crate::Themed`] to hand it to [`crate::run`].
pub struct Theme {
    tokens: Arc<Tokens>,
    shown: Arc<Tokens>,
    revision: u64,
    depth: Depth,
    fade: Option<Fade>,
    rx: Option<mpsc::Receiver<Tokens>>,
    _watch: Option<omarchy_theme::Watch>,
    hits: RefCell<Vec<(String, Rect)>>,
    started: Instant,
    clock: StdCell<Option<u64>>,
}

struct Fade {
    from: Arc<Tokens>,
    start: Instant,
    duration: Duration,
}

impl Theme {
    /// The theme as it is now (or the `OMARCHY_UI_THEME_DIR` fixture), not following anything yet;
    /// [`crate::run`] (or [`Theme::follow`]) starts following. Color depth from the environment.
    pub fn load() -> Theme {
        Theme::from_tokens(omarchy_theme::load())
    }

    /// A theme pinned to `tokens` (tests, previews). Truecolor until [`Theme::set_depth`].
    pub fn from_tokens(tokens: Tokens) -> Theme {
        let tokens = Arc::new(tokens);
        Theme {
            shown: tokens.clone(),
            tokens,
            revision: 0,
            depth: Depth::detect(),
            fade: None,
            rx: None,
            _watch: None,
            hits: RefCell::new(Vec::new()),
            started: Instant::now(),
            clock: StdCell::new(None),
        }
    }

    /// Follow the desktop: theme switches, `omarchy font set`, `omarchy display text size`,
    /// Hyprland reloads, the language and the icon theme. The core's watch delivers on its own
    /// thread; changes wait in a channel until [`Theme::take_change`] runs on the app's loop.
    /// Calling it again does nothing.
    pub fn follow(&mut self) {
        if self._watch.is_some() {
            return;
        }
        let (tx, rx) = mpsc::channel();
        let watch = omarchy_theme::watch((*self.tokens).clone(), move |fresh| {
            let _ = tx.send(fresh);
        });
        self.rx = Some(rx);
        self._watch = Some(watch);
    }

    /// Apply the next delivered change, if one is waiting: the new tokens take effect at once,
    /// colors start fading, the revision counts it. Returns the new tokens (for the changed hook).
    /// [`crate::run`] calls this; an app with its own loop calls it each iteration.
    pub fn take_change(&mut self) -> Option<Arc<Tokens>> {
        let fresh = self.rx.as_ref()?.try_recv().ok()?;
        self.set_tokens(fresh).then(|| self.tokens.clone())
    }

    /// Deliver a change by hand (pin or preview a theme): tokens equal to the current ones change
    /// nothing and return false.
    pub fn set_tokens(&mut self, fresh: Tokens) -> bool {
        if *self.tokens == fresh {
            return false;
        }
        let to = Arc::new(fresh);
        let from = self.shown.clone();
        self.tokens = to.clone();
        self.revision += 1;
        if to.reduce_motion || to.motion.theme == 0 || self.depth != Depth::True {
            self.shown = to;
            self.fade = None;
        } else {
            // Layout, sizes and the font switch now; the colors move from what is on screen.
            self.shown = Arc::new(mix_tokens(&from, &to, 0.0));
            self.fade = Some(Fade { from, start: Instant::now(), duration: Duration::from_millis(to.motion.theme as u64) });
        }
        true
    }

    /// Advance the fade to now. True while it is still running (draw again soon).
    pub fn tick(&mut self) -> bool {
        let Some(fade) = &self.fade else { return false };
        let x = fade.start.elapsed().as_secs_f64() / fade.duration.as_secs_f64();
        if x >= 1.0 {
            // Land exactly on the tokens.
            self.shown = self.tokens.clone();
            self.fade = None;
            return false;
        }
        let p = cubic_bezier(self.tokens.motion.ease, x);
        self.shown = Arc::new(mix_tokens(&fade.from, &self.tokens, p));
        true
    }

    /// True while a theme fade runs.
    pub fn fading(&self) -> bool {
        self.fade.is_some()
    }

    /// The tokens in effect: after a change, the new ones (a fade's target, never a frame of it).
    pub fn tokens(&self) -> &Arc<Tokens> {
        &self.tokens
    }

    /// What is on screen: [`Theme::tokens`] with colors where the fade has them now.
    pub fn shown(&self) -> &Tokens {
        &self.shown
    }

    /// 0 for the tokens the app started with, plus one per change: a cache key.
    pub fn revision(&self) -> u64 {
        self.revision
    }

    pub fn depth(&self) -> Depth {
        self.depth
    }

    /// Draw for this many colors (snapshots pin truecolor).
    pub fn set_depth(&mut self, depth: Depth) {
        self.depth = depth;
    }

    /// The kit's string `key` in the display language (`omarchy_theme::strings`), else `fallback`.
    pub fn string(&self, key: &str, fallback: &str) -> String {
        self.tokens.string(key, fallback)
    }

    // Colors, from what is on screen.

    pub fn fg(&self) -> Rgba {
        self.shown.palette.foreground
    }
    pub fn bg(&self) -> Rgba {
        self.shown.palette.background
    }
    pub fn accent(&self) -> Rgba {
        self.shown.palette.accent
    }
    pub fn urgent(&self) -> Rgba {
        self.shown.palette.urgent
    }
    pub fn success(&self) -> Rgba {
        self.shown.palette.success
    }
    pub fn warning(&self) -> Rgba {
        self.shown.palette.warning
    }
    pub fn muted(&self) -> Rgba {
        self.shown.palette.muted
    }
    /// Secondary text (`roles.textDim`).
    pub fn dim(&self) -> Rgba {
        self.shown.roles.text_dim
    }
    /// Disabled and placeholder text (`roles.textFaint`).
    pub fn faint(&self) -> Rgba {
        self.shown.roles.text_faint
    }
    /// Tracks, rules and separators (`roles.track`).
    pub fn track(&self) -> Rgba {
        self.shown.roles.track
    }
    /// "Hot accent": the accent with a quarter of the foreground mixed in.
    pub fn hot_accent(&self) -> Rgba {
        self.accent().mix(self.fg(), 0.25)
    }
    /// Hyprland's rounding; > 0 draws rounded box corners.
    pub fn radius(&self) -> i64 {
        self.shown.radius
    }
    pub fn rounded(&self) -> bool {
        self.shown.radius > 0
    }
    pub fn reduce_motion(&self) -> bool {
        self.shown.reduce_motion
    }

    // The animation clock (spinners, the indeterminate bar).

    /// Milliseconds since the theme was made, for component animations; pinned by
    /// [`Theme::set_clock`] in tests. Under reduced motion components draw their still frame.
    pub fn clock(&self) -> u64 {
        self.clock.get().unwrap_or_else(|| self.started.elapsed().as_millis() as u64)
    }

    pub fn set_clock(&self, ms: Option<u64>) {
        self.clock.set(ms);
    }

    // Hit testing: components with a `.key(..)` record where they drew.

    /// Record that `key` was drawn at `area` this frame (components do this for their keys).
    pub fn hit(&self, key: impl Into<String>, area: Rect) {
        if area.width > 0 && area.height > 0 {
            self.hits.borrow_mut().push((key.into(), area));
        }
    }

    /// The topmost key drawn at a cell this frame.
    pub fn hit_at(&self, col: u16, row: u16) -> Option<String> {
        let p = ratatui::layout::Position { x: col, y: row };
        self.hits.borrow().iter().rev().find(|(_, r)| r.contains(p)).map(|(k, _)| k.clone())
    }

    /// Where `key` was drawn this frame.
    pub fn find(&self, key: &str) -> Option<Rect> {
        self.hits.borrow().iter().rev().find(|(k, _)| k == key).map(|(_, r)| *r)
    }

    /// How many keys were recorded this frame (a scroll view notes it before drawing offscreen).
    pub fn hits_len(&self) -> usize {
        self.hits.borrow().len()
    }

    /// Move (or drop) the keys recorded since `from` (a scroll view drew them offscreen).
    pub fn remap_hits(&self, from: usize, f: impl Fn(Rect) -> Option<Rect>) {
        let mut hits = self.hits.borrow_mut();
        let tail: Vec<_> = hits.drain(from..).filter_map(|(k, r)| f(r).map(|r| (k, r))).collect();
        hits.extend(tail);
    }

    /// Forget last frame's keys; [`crate::run`] calls it before each draw.
    pub fn clear_hits(&self) {
        self.hits.borrow_mut().clear();
    }

    /// The last step of every frame: cells whose background is the theme's background are written
    /// as the terminal's default (so terminal opacity shows), and below truecolor every color maps
    /// down (spec/terminal.md, Color). [`crate::run`] and the snapshot writer call it.
    pub fn finish(&self, buf: &mut Buffer) {
        let bg = depth::rgb(self.bg());
        let fg = depth::rgb(self.fg());
        let tables = (self.depth == Depth::Ansi16).then(|| depth::tables(&self.shown));
        for cell in buf.content.iter_mut() {
            let cell_bg = match cell.bg {
                Color::Rgb(r, g, b) => Some([r, g, b]),
                _ => None,
            };
            if cell_bg == Some(bg) {
                cell.bg = Color::Reset;
            }
            match self.depth {
                Depth::True => {}
                Depth::Ansi256 => {
                    if let Color::Rgb(r, g, b) = cell.bg {
                        cell.bg = Color::Indexed(depth::nearest256([r, g, b]));
                    }
                    if let Color::Rgb(r, g, b) = cell.fg {
                        cell.fg = if [r, g, b] == fg { Color::Reset } else { Color::Indexed(depth::nearest256([r, g, b])) };
                    }
                }
                Depth::Ansi16 => {
                    let (ink, fill) = tables.as_ref().unwrap();
                    if let Color::Rgb(r, g, b) = cell.bg {
                        cell.bg = depth::nearest_role([r, g, b], fill).color;
                    }
                    if let Color::Rgb(r, g, b) = cell.fg {
                        let role = depth::nearest_role([r, g, b], ink);
                        cell.fg = role.color;
                        if role.faint {
                            cell.modifier |= Modifier::DIM;
                        }
                    }
                    // Ink the same as its fill would vanish: use the default foreground.
                    if cell.fg == cell.bg && cell.fg != Color::Reset {
                        cell.fg = Color::Reset;
                    }
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use ratatui::buffer::Buffer;

    fn theme() -> Theme {
        let mut t = Theme::from_tokens(omarchy_theme::resolve(&omarchy_theme::Sources::default()));
        t.set_depth(Depth::True);
        t
    }

    /// The page background is never painted; other fills are, mapped down below truecolor.
    #[test]
    fn finish_leaves_the_background_to_the_terminal() {
        let mut t = theme();
        let mut buf = Buffer::empty(Rect::new(0, 0, 3, 1));
        crate::paint::fill(&t, &mut buf, Rect::new(0, 0, 1, 1), t.bg());
        crate::paint::fill(&t, &mut buf, Rect::new(1, 0, 1, 1), t.shown().controls.hover.fill);
        crate::paint::put(&t, &mut buf, 2, 0, "x", t.dim());
        let mut tc = buf.clone();
        t.finish(&mut tc);
        assert_eq!(tc[(0, 0)].bg, Color::Reset);
        assert!(matches!(tc[(1, 0)].bg, Color::Rgb(..)));
        t.set_depth(Depth::Ansi16);
        let mut c16 = buf.clone();
        t.finish(&mut c16);
        assert_eq!(c16[(1, 0)].bg, Color::DarkGray, "the cursor's fill is bright black");
        assert_eq!(c16[(2, 0)].fg, Color::DarkGray, "textDim is bright black");
        t.set_depth(Depth::Ansi256);
        let mut c256 = buf;
        t.finish(&mut c256);
        assert!(matches!(c256[(1, 0)].bg, Color::Indexed(16..=255)));
    }

    /// A change counts once, fades in truecolor and lands exactly; none at 16 colors.
    #[test]
    fn changes_fade_and_land() {
        let mut t = theme();
        let mut next = (**t.tokens()).clone();
        next.palette.background = Rgba::rgb(0xef, 0xf1, 0xf5);
        next.reduce_motion = false;
        assert!(t.set_tokens(next.clone()));
        assert!(!t.set_tokens(next.clone()), "the same tokens are no change");
        assert_eq!(t.revision(), 1);
        assert!(t.fading());
        assert_ne!(t.shown().palette.background, next.palette.background);
        std::thread::sleep(Duration::from_millis(next.motion.theme as u64 + 20));
        assert!(!t.tick());
        assert_eq!(t.shown().palette.background, next.palette.background);
        let mut t16 = theme();
        t16.set_depth(Depth::Ansi16);
        t16.set_tokens(next);
        assert!(!t16.fading());
    }
}
