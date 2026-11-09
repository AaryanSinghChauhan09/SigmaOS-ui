//! Color depth (spec/terminal.md, Color): every component paints 24-bit colors into the buffer;
//! [`crate::Theme::finish`] then writes the page background as the terminal's default and, below
//! truecolor, maps every color down: to the nearest xterm-256 index (16–255), or to the ANSI index
//! of the nearest theme role, so the terminal's own (Omarchy-themed) palette supplies it.

use omarchy_theme::{Rgba, Tokens};
use ratatui::style::Color;

/// How many colors the terminal shows.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Depth {
    /// 24-bit SGR (`38;2;r;g;b`). Every Omarchy terminal.
    True,
    /// xterm's 256 colors (`38;5;n`), indices 16–255 only.
    Ansi256,
    /// The 16 ANSI colors (30–37, 90–97), by role.
    Ansi16,
}

impl Depth {
    /// `OMARCHY_UI_COLORS` (`24bit`, `truecolor`, `256`, `16`) when set; else truecolor when
    /// `COLORTERM` is `truecolor` or `24bit`; else 256 when `TERM` mentions `256color`; else 16.
    pub fn detect() -> Depth {
        Self::from_env(|k| std::env::var(k).ok())
    }

    pub fn from_env(var: impl Fn(&str) -> Option<String>) -> Depth {
        match var("OMARCHY_UI_COLORS").as_deref().map(str::trim) {
            Some("24bit" | "truecolor" | "24") => return Depth::True,
            Some("256") => return Depth::Ansi256,
            Some("16" | "8") => return Depth::Ansi16,
            _ => {}
        }
        if matches!(var("COLORTERM").as_deref(), Some("truecolor" | "24bit")) {
            return Depth::True;
        }
        if var("TERM").is_some_and(|t| t.contains("256color")) {
            return Depth::Ansi256;
        }
        Depth::Ansi16
    }
}

/// The xterm-256 color at `i` (16–255): the 6×6×6 cube, then the 24 grays.
pub fn xterm256(i: u8) -> [u8; 3] {
    const LEVELS: [u8; 6] = [0, 95, 135, 175, 215, 255];
    if i >= 232 {
        let v = 8 + 10 * (i - 232);
        return [v, v, v];
    }
    let n = i.saturating_sub(16);
    [LEVELS[(n / 36) as usize], LEVELS[(n / 6 % 6) as usize], LEVELS[(n % 6) as usize]]
}

fn dist(a: [u8; 3], b: [u8; 3]) -> u32 {
    let d = |x: u8, y: u8| (x as i32 - y as i32).pow(2) as u32;
    d(a[0], b[0]) + d(a[1], b[1]) + d(a[2], b[2])
}

/// The nearest xterm-256 index in 16–255 by squared sRGB distance; ties go to the lower index.
pub fn nearest256(c: [u8; 3]) -> u8 {
    let mut best = (u32::MAX, 16u8);
    for i in 16..=255u8 {
        let d = dist(c, xterm256(i));
        if d < best.0 {
            best = (d, i);
        }
    }
    best.1
}

/// One role of the 16-color table: its color in this theme and what it becomes.
#[derive(Clone, Copy)]
pub(crate) struct Role {
    pub rgb: [u8; 3],
    pub color: Color,
    /// Also set the faint attribute (SGR 2): `textFaint`.
    pub faint: bool,
}

pub(crate) fn rgb(c: Rgba) -> [u8; 3] {
    [c.r, c.g, c.b]
}

/// `top` over the opaque `under`, per 8-bit channel: `(t·a + u·(255 − a) + 127) / 255`, which is
/// `round((t·a + u·(255 − a)) / 255)` exactly (the core's `mix` by `a / 255`).
pub fn over(top: Rgba, under: Rgba) -> Rgba {
    let a = top.a as u32;
    let ch = |t: u8, u: u8| ((t as u32 * a + u as u32 * (255 - a) + 127) / 255) as u8;
    Rgba { r: ch(top.r, under.r), g: ch(top.g, under.g), b: ch(top.b, under.b), a: 255 }
}

/// The 16-color tables (spec/terminal.md): ink roles in order, then fill roles. Earlier entries
/// win exact ties.
pub(crate) fn tables(t: &Tokens) -> (Vec<Role>, Vec<Role>) {
    let bg = t.palette.background;
    let o = |c: Rgba| rgb(over(c, bg));
    let r = |c: Rgba, color: Color| Role { rgb: o(c), color, faint: false };
    let ink = vec![
        r(t.palette.foreground, Color::Reset),
        r(t.roles.text_dim, Color::DarkGray),
        Role { rgb: o(t.roles.text_faint), color: Color::DarkGray, faint: true },
        r(t.palette.muted, Color::DarkGray),
        r(t.palette.accent, Color::Blue),
        r(t.palette.urgent, Color::Red),
        r(t.palette.success, Color::Green),
        r(t.palette.warning, Color::Yellow),
        r(t.palette.background, Color::Black),
        r(t.controls.normal.border.color, Color::DarkGray),
        r(t.controls.hover.border.color, Color::DarkGray),
        r(t.roles.track, Color::DarkGray),
    ];
    let fill = vec![
        r(t.palette.background, Color::Reset),
        r(t.controls.normal.fill, Color::Reset),
        r(t.controls.selected.fill, Color::Reset),
        r(t.controls.hover.fill, Color::DarkGray),
        r(t.controls.focus.fill, Color::DarkGray),
        r(t.controls.pressed.fill, Color::DarkGray),
        r(t.palette.accent, Color::Blue),
        r(t.palette.urgent, Color::Red),
        r(t.palette.success, Color::Green),
        r(t.palette.warning, Color::Yellow),
        r(t.palette.foreground, Color::Gray),
    ];
    (ink, fill)
}

pub(crate) fn nearest_role(c: [u8; 3], roles: &[Role]) -> Role {
    let mut best = (u32::MAX, roles[0]);
    for r in roles {
        let d = dist(c, r.rgb);
        if d < best.0 {
            best = (d, *r);
        }
    }
    best.1
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn detects_depth() {
        let env = |pairs: &'static [(&'static str, &'static str)]| move |k: &str| pairs.iter().find(|(a, _)| *a == k).map(|(_, v)| v.to_string());
        assert_eq!(Depth::from_env(env(&[("COLORTERM", "truecolor")])), Depth::True);
        assert_eq!(Depth::from_env(env(&[("TERM", "xterm-256color")])), Depth::Ansi256);
        assert_eq!(Depth::from_env(env(&[("TERM", "linux")])), Depth::Ansi16);
        assert_eq!(Depth::from_env(env(&[("COLORTERM", "truecolor"), ("OMARCHY_UI_COLORS", "16")])), Depth::Ansi16);
    }

    #[test]
    fn composites_like_the_core() {
        let fg = Rgba::rgb(0xa9, 0xb1, 0xd6);
        let bg = Rgba::rgb(0x1a, 0x1b, 0x26);
        for a in 0..=255u8 {
            let mine = over(Rgba { a, ..fg }, bg);
            let core = bg.mix(fg, a as f64 / 255.0);
            assert_eq!((mine.r, mine.g, mine.b), (core.r, core.g, core.b), "alpha {a}");
        }
    }

    #[test]
    fn nearest_256() {
        assert_eq!(nearest256([0, 0, 0]), 16);
        assert_eq!(nearest256([255, 255, 255]), 231);
        assert_eq!(nearest256([128, 128, 128]), 244);
    }
}
