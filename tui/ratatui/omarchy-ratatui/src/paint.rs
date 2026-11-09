//! Cell painting in the kit's terms (spec/terminal.md): colors with alpha composite over the
//! cell they land on (its painted background, else the theme background), box corners follow the
//! radius, controls frame themselves with eighth-block edges. Every component draws through these;
//! an app's own drawing can too.

use omarchy_theme::Rgba;
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;
use ratatui::style::{Color, Modifier};
use unicode_width::{UnicodeWidthChar, UnicodeWidthStr};

use crate::Theme;
use crate::depth::over;

/// A control's left edge: U+258F LEFT ONE EIGHTH BLOCK, a thin line on the control's outer edge.
pub const EDGE_LEFT: &str = "▏";
/// A control's right edge: U+2595 RIGHT ONE EIGHTH BLOCK.
pub const EDGE_RIGHT: &str = "▕";
/// The ellipsis that ends elided text.
pub const ELLIPSIS: &str = "…";

/// Ink: a color (alpha composites over the cell's background) and attributes.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Ink {
    pub color: Rgba,
    pub bold: bool,
    pub italic: bool,
    pub underline: bool,
}

impl Ink {
    pub fn new(color: Rgba) -> Ink {
        Ink { color, bold: false, italic: false, underline: false }
    }
    pub fn bold(mut self, b: bool) -> Ink {
        self.bold = b;
        self
    }
    pub fn italic(mut self, b: bool) -> Ink {
        self.italic = b;
        self
    }
    pub fn underline(mut self, b: bool) -> Ink {
        self.underline = b;
        self
    }
    fn modifier(&self) -> Modifier {
        let mut m = Modifier::empty();
        if self.bold {
            m |= Modifier::BOLD;
        }
        if self.italic {
            m |= Modifier::ITALIC;
        }
        if self.underline {
            m |= Modifier::UNDERLINED;
        }
        m
    }
}

impl From<Rgba> for Ink {
    fn from(c: Rgba) -> Ink {
        Ink::new(c)
    }
}

fn rgb(c: Rgba) -> Color {
    Color::Rgb(c.r, c.g, c.b)
}

/// The background a cell shows now: what was painted there, else the theme background.
pub fn under(t: &Theme, buf: &Buffer, x: u16, y: u16) -> Rgba {
    match buf.cell((x, y)).map(|c| c.bg) {
        Some(Color::Rgb(r, g, b)) => Rgba::rgb(r, g, b),
        _ => t.bg(),
    }
}

/// Display width in cells (unicode-width; Nerd Font glyphs are 1).
pub fn width(s: &str) -> u16 {
    UnicodeWidthStr::width(s).min(u16::MAX as usize) as u16
}

/// `s` cut to `w` cells, ending in "…" when it had to be cut.
pub fn fit(s: &str, w: u16) -> String {
    if width(s) <= w {
        return s.to_string();
    }
    if w == 0 {
        return String::new();
    }
    let mut out = String::new();
    let mut used = 0u16;
    for ch in s.chars() {
        let cw = UnicodeWidthChar::width(ch).unwrap_or(0) as u16;
        if used + cw > w - 1 {
            break;
        }
        out.push(ch);
        used += cw;
    }
    out.push_str(ELLIPSIS);
    out
}

/// Composite `c` over the background of every cell in `area` (the characters stay).
pub fn fill(t: &Theme, buf: &mut Buffer, area: Rect, c: Rgba) {
    if c.a == 0 {
        return;
    }
    let area = area.intersection(buf.area);
    for y in area.top()..area.bottom() {
        for x in area.left()..area.right() {
            let u = under(t, buf, x, y);
            if let Some(cell) = buf.cell_mut((x, y)) {
                cell.bg = rgb(over(c, u));
            }
        }
    }
}

/// Blank the cells of `area`: spaces on the theme background (a surface drawn over something).
pub fn clear(buf: &mut Buffer, area: Rect) {
    let area = area.intersection(buf.area);
    for y in area.top()..area.bottom() {
        for x in area.left()..area.right() {
            if let Some(cell) = buf.cell_mut((x, y)) {
                cell.reset();
            }
        }
    }
}

/// Put one symbol (a character and any marks on it) at a cell in `ink`.
pub fn put(t: &Theme, buf: &mut Buffer, x: u16, y: u16, sym: &str, ink: impl Into<Ink>) {
    let ink = ink.into();
    let u = under(t, buf, x, y);
    if let Some(cell) = buf.cell_mut((x, y)) {
        cell.set_symbol(sym);
        cell.fg = rgb(over(ink.color, u));
        cell.modifier = ink.modifier();
    }
}

/// Write `s` from (x, y), at most `max` cells, in `ink`; returns the cells written. Wide
/// characters take two cells; marks join the character before them.
pub fn text(t: &Theme, buf: &mut Buffer, x: u16, y: u16, s: &str, max: u16, ink: impl Into<Ink>) -> u16 {
    let ink = ink.into();
    let mut col = 0u16;
    let mut last: Option<(u16, String)> = None;
    let flush = |buf: &mut Buffer, last: &mut Option<(u16, String)>| {
        if let Some((cx, sym)) = last.take() {
            put(t, buf, cx, y, &sym, ink);
        }
    };
    for ch in s.chars() {
        if ch.is_control() {
            continue;
        }
        let cw = UnicodeWidthChar::width(ch).unwrap_or(0) as u16;
        if cw == 0 {
            if let Some((_, sym)) = last.as_mut() {
                sym.push(ch);
            }
            continue;
        }
        if col + cw > max {
            break;
        }
        flush(buf, &mut last);
        last = Some((x + col, ch.to_string()));
        if cw == 2 {
            // The column a wide character covers: blank, same background.
            if let Some(cell) = buf.cell_mut((x + col + 1, y)) {
                cell.set_symbol(" ");
            }
        }
        col += cw;
    }
    flush(buf, &mut last);
    col
}

/// [`text`] elided with "…" to `max` cells.
pub fn text_fit(t: &Theme, buf: &mut Buffer, x: u16, y: u16, s: &str, max: u16, ink: impl Into<Ink>) -> u16 {
    text(t, buf, x, y, &fit(s, max), max, ink)
}

/// [`text`] right-aligned to end at `right` (exclusive).
pub fn text_right(t: &Theme, buf: &mut Buffer, right: u16, y: u16, s: &str, ink: impl Into<Ink>) -> u16 {
    let w = width(s);
    text(t, buf, right.saturating_sub(w), y, s, w, ink)
}

/// [`text`] centered in `w` cells from `x` (an odd remainder goes right).
pub fn text_center(t: &Theme, buf: &mut Buffer, x: u16, y: u16, w: u16, s: &str, ink: impl Into<Ink>) -> u16 {
    let s = fit(s, w);
    let sw = width(&s);
    text(t, buf, x + (w.saturating_sub(sw)) / 2, y, &s, sw, ink)
}

/// A control's edges: [`EDGE_LEFT`] at `x` and [`EDGE_RIGHT`] at `x + w − 1` in `color`, when the
/// state shows a border. With `None` the cells are left as they are (reserved, so a state change
/// never relayouts).
pub fn edges(t: &Theme, buf: &mut Buffer, x: u16, y: u16, w: u16, color: Option<Rgba>) {
    if let Some(c) = color
        && w >= 2
    {
        put(t, buf, x, y, EDGE_LEFT, c);
        put(t, buf, x + w - 1, y, EDGE_RIGHT, c);
    }
}

/// A horizontal rule of `sym` (`─` by default) across `w` cells.
pub fn hline(t: &Theme, buf: &mut Buffer, x: u16, y: u16, w: u16, sym: &str, color: Rgba) {
    for i in 0..w {
        put(t, buf, x + i, y, sym, color);
    }
}

/// A vertical rule of `sym` (`│`) down `h` cells.
pub fn vline(t: &Theme, buf: &mut Buffer, x: u16, y: u16, h: u16, sym: &str, color: Rgba) {
    for i in 0..h {
        put(t, buf, x, y + i, sym, color);
    }
}

/// The corners for the theme's radius: square at 0, rounded above.
pub fn corners(t: &Theme) -> [&'static str; 4] {
    if t.rounded() { ["╭", "╮", "╰", "╯"] } else { ["┌", "┐", "└", "┘"] }
}

/// A box border around `area` in `color` (light lines whatever the border width: spec/terminal.md),
/// corners by the radius, an optional title in the top edge (`┌ Title ───┐`, bold, from the
/// second column). Returns the area inside the border.
pub fn frame(t: &Theme, buf: &mut Buffer, area: Rect, color: Rgba, title: Option<(&str, Ink)>) -> Rect {
    if area.width < 2 || area.height < 2 {
        return area;
    }
    let [tl, tr, bl, br] = corners(t);
    let (x, y, r, b) = (area.x, area.y, area.right() - 1, area.bottom() - 1);
    put(t, buf, x, y, tl, color);
    put(t, buf, r, y, tr, color);
    put(t, buf, x, b, bl, color);
    put(t, buf, r, b, br, color);
    hline(t, buf, x + 1, y, area.width - 2, "─", color);
    hline(t, buf, x + 1, b, area.width - 2, "─", color);
    vline(t, buf, x, y + 1, area.height - 2, "│", color);
    vline(t, buf, r, y + 1, area.height - 2, "│", color);
    if let Some((title, ink)) = title
        && area.width > 4
    {
        let s = format!(" {} ", fit(title, area.width - 4));
        text(t, buf, x + 1, y, &s, area.width - 2, ink);
    }
    Rect { x: area.x + 1, y: area.y + 1, width: area.width - 2, height: area.height - 2 }
}

/// Composite `c` (a scrim) over everything in `area`: backgrounds and ink alike, so what is under
/// a modal fades toward it.
pub fn scrim(t: &Theme, buf: &mut Buffer, area: Rect, c: Rgba) {
    if c.a == 0 {
        return;
    }
    let area = area.intersection(buf.area);
    for y in area.top()..area.bottom() {
        for x in area.left()..area.right() {
            let u = under(t, buf, x, y);
            if let Some(cell) = buf.cell_mut((x, y)) {
                cell.bg = rgb(over(c, u));
                if let Color::Rgb(r, g, b) = cell.fg {
                    cell.fg = rgb(over(c, Rgba::rgb(r, g, b)));
                }
            }
        }
    }
}

/// The inner rect of `area` inset by `dx` columns and `dy` rows on every side.
pub fn inset(area: Rect, dx: u16, dy: u16) -> Rect {
    Rect {
        x: area.x + dx.min(area.width / 2),
        y: area.y + dy.min(area.height / 2),
        width: area.width.saturating_sub(2 * dx),
        height: area.height.saturating_sub(2 * dy),
    }
}
