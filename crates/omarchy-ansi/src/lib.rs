//! The terminal bindings' gallery snapshots (`spec/terminal.md`): parse them, write them in the
//! canonical form, diff them cell by cell, and rasterize them to PNG with one renderer, so review
//! differences between Ratatui, Bubble Tea and Textual are real cell differences.
//!
//! # Snapshot format
//!
//! - UTF-8, exactly ROWS lines, each ending in `\n`. Every line is exactly COLS columns wide by
//!   display width (unicode-width 0.2: wide characters take 2 columns, everything else 1,
//!   Nerd Font private-use glyphs included; zero-width characters attach to the previous cell).
//! - The only escapes are SGR (`ESC [ params m`). Every line starts in the default state.
//! - SGR parameters: 0 (and empty) reset; 1 bold; 2 faint; 3 italic; 4 underline; 7 reverse;
//!   9 crossed; 22 bold and faint off; 23; 24; 27; 29; 30–37 and 90–97 ANSI foreground; 39
//!   default foreground; 40–47 and 100–107 ANSI background; 49 default background;
//!   `38;2;r;g;b` / `48;2;r;g;b` truecolor; `38;5;n` / `48;5;n` the xterm 256 colors.
//! - The default colors (39, 49) are the theme's `palette.foreground` and `palette.background`.
//!
//! # Canonical form ([`write_canonical`])
//!
//! Per line, per cell (a wide character's second column writes nothing): the style is (fg, bg,
//! bold, faint, italic, underline, reverse, crossed). A space with none of underline, reverse or
//! crossed is normalized to the default fg with bold, faint and italic cleared. When the style
//! differs from the previous cell's (a line starts from the default), write `ESC[0`, then `;1`,
//! `;2`, `;3`, `;4`, `;7`, `;9` for the attributes that are set, `;38;2;r;g;b` unless fg is the
//! default, `;48;2;r;g;b` unless bg is the default, then `m`. Then the cell's text. At the end of
//! a line whose last style isn't the default, `ESC[0m`. Then `\n`.
//!
//! # Rendering ([`render`])
//!
//! See [`render`] for the rasterization rules.

mod boxdraw;
mod render;

pub use render::{Faces, Image, RenderOptions, render};

use unicode_width::UnicodeWidthChar;

/// A cell color.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub enum Color {
    /// The terminal's default: the theme's foreground or background.
    #[default]
    Default,
    Rgb(u8, u8, u8),
    /// An xterm 256-color index (0–15 the ANSI colors).
    Indexed(u8),
}

/// Text attributes.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct Attrs {
    pub bold: bool,
    pub faint: bool,
    pub italic: bool,
    pub underline: bool,
    pub reverse: bool,
    pub crossed: bool,
}

/// One column of the grid. A wide character's second column is a continuation cell: width 0,
/// empty text, the same style.
#[derive(Clone, Debug, PartialEq, Eq, Default)]
pub struct Cell {
    pub text: String,
    pub width: u8,
    pub fg: Color,
    pub bg: Color,
    pub attrs: Attrs,
}

impl Cell {
    pub fn blank() -> Cell {
        Cell {
            text: " ".into(),
            width: 1,
            ..Default::default()
        }
    }
}

/// A parsed snapshot: `rows` lines of `cols` cells, row-major.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Grid {
    pub cols: usize,
    pub rows: usize,
    pub cells: Vec<Cell>,
}

impl Grid {
    pub fn new(cols: usize, rows: usize) -> Grid {
        Grid {
            cols,
            rows,
            cells: vec![Cell::blank(); cols * rows],
        }
    }
    pub fn get(&self, col: usize, row: usize) -> Option<&Cell> {
        (col < self.cols && row < self.rows).then(|| &self.cells[row * self.cols + col])
    }
    pub fn get_mut(&mut self, col: usize, row: usize) -> Option<&mut Cell> {
        (col < self.cols && row < self.rows).then(|| &mut self.cells[row * self.cols + col])
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Default)]
struct Style {
    fg: Color,
    bg: Color,
    attrs: Attrs,
}

fn apply_sgr(style: &mut Style, params: &str) -> Result<(), String> {
    let nums: Vec<u32> = if params.is_empty() {
        vec![0]
    } else {
        params
            .split(';')
            .map(|p| {
                if p.is_empty() {
                    Ok(0)
                } else {
                    p.parse::<u32>()
                        .map_err(|_| format!("bad SGR parameter {p:?}"))
                }
            })
            .collect::<Result<_, _>>()?
    };
    let mut i = 0;
    while i < nums.len() {
        let n = nums[i];
        match n {
            0 => *style = Style::default(),
            1 => style.attrs.bold = true,
            2 => style.attrs.faint = true,
            3 => style.attrs.italic = true,
            4 => style.attrs.underline = true,
            7 => style.attrs.reverse = true,
            9 => style.attrs.crossed = true,
            22 => {
                style.attrs.bold = false;
                style.attrs.faint = false;
            }
            23 => style.attrs.italic = false,
            24 => style.attrs.underline = false,
            27 => style.attrs.reverse = false,
            29 => style.attrs.crossed = false,
            30..=37 => style.fg = Color::Indexed((n - 30) as u8),
            90..=97 => style.fg = Color::Indexed((n - 90 + 8) as u8),
            39 => style.fg = Color::Default,
            40..=47 => style.bg = Color::Indexed((n - 40) as u8),
            100..=107 => style.bg = Color::Indexed((n - 100 + 8) as u8),
            49 => style.bg = Color::Default,
            38 | 48 => {
                let c = match nums.get(i + 1) {
                    Some(2) => {
                        let ch = |k: usize| {
                            nums.get(i + k)
                                .copied()
                                .filter(|v| *v <= 255)
                                .ok_or("bad truecolor SGR")
                        };
                        let c = Color::Rgb(ch(2)? as u8, ch(3)? as u8, ch(4)? as u8);
                        i += 4;
                        c
                    }
                    Some(5) => {
                        let v = nums
                            .get(i + 2)
                            .copied()
                            .filter(|v| *v <= 255)
                            .ok_or("bad 256-color SGR")?;
                        i += 2;
                        Color::Indexed(v as u8)
                    }
                    _ => return Err(format!("SGR {n} needs ;2;r;g;b or ;5;n")),
                };
                if n == 38 {
                    style.fg = c;
                } else {
                    style.bg = c;
                }
            }
            other => return Err(format!("unsupported SGR parameter {other}")),
        }
        i += 1;
    }
    Ok(())
}

/// Parse a snapshot strictly: errors name the line (1-based) and column.
pub fn parse(text: &str) -> Result<Grid, String> {
    if text.is_empty() {
        return Err("empty snapshot".into());
    }
    if !text.ends_with('\n') {
        return Err("the last line doesn't end in \\n".into());
    }
    let mut lines: Vec<Vec<Cell>> = Vec::new();
    for (ln, line) in text[..text.len() - 1].split('\n').enumerate() {
        let lineno = ln + 1;
        let mut style = Style::default();
        let mut cells: Vec<Cell> = Vec::new();
        let mut chars = line.chars().peekable();
        while let Some(c) = chars.next() {
            if c == '\x1b' {
                if chars.next() != Some('[') {
                    return Err(format!(
                        "line {lineno}, column {}: an escape other than SGR",
                        cells.len() + 1
                    ));
                }
                let mut params = String::new();
                loop {
                    match chars.next() {
                        Some('m') => break,
                        Some(p) if p.is_ascii_digit() || p == ';' => params.push(p),
                        Some(p) => {
                            return Err(format!(
                                "line {lineno}, column {}: CSI ending in {p:?} isn't SGR",
                                cells.len() + 1
                            ));
                        }
                        None => return Err(format!("line {lineno}: unterminated escape")),
                    }
                }
                apply_sgr(&mut style, &params)
                    .map_err(|e| format!("line {lineno}, column {}: {e}", cells.len() + 1))?;
                continue;
            }
            if c == '\r' || (c.is_control() && c != '\t') || c == '\t' {
                return Err(format!(
                    "line {lineno}, column {}: control character U+{:04X}",
                    cells.len() + 1,
                    c as u32
                ));
            }
            let w = c.width().unwrap_or(0);
            if w == 0 {
                // A combining mark or zero-width character joins the previous cell.
                let Some(prev) = cells.iter_mut().rev().find(|c| c.width > 0) else {
                    return Err(format!(
                        "line {lineno}: a zero-width character starts the line"
                    ));
                };
                prev.text.push(c);
                continue;
            }
            cells.push(Cell {
                text: c.to_string(),
                width: w as u8,
                fg: style.fg,
                bg: style.bg,
                attrs: style.attrs,
            });
            if w == 2 {
                cells.push(Cell {
                    text: String::new(),
                    width: 0,
                    fg: style.fg,
                    bg: style.bg,
                    attrs: style.attrs,
                });
            }
        }
        lines.push(cells);
    }
    let cols = lines[0].len();
    for (i, l) in lines.iter().enumerate() {
        if l.len() != cols {
            return Err(format!(
                "line {} is {} columns wide, line 1 is {cols}",
                i + 1,
                l.len()
            ));
        }
    }
    if cols == 0 {
        return Err("lines are empty".into());
    }
    let rows = lines.len();
    Ok(Grid {
        cols,
        rows,
        cells: lines.into_iter().flatten().collect(),
    })
}

fn normalized(c: &Cell) -> Style {
    let mut s = Style {
        fg: c.fg,
        bg: c.bg,
        attrs: c.attrs,
    };
    if c.text == " " && !c.attrs.underline && !c.attrs.reverse && !c.attrs.crossed {
        s.fg = Color::Default;
        s.attrs.bold = false;
        s.attrs.faint = false;
        s.attrs.italic = false;
    }
    s
}

fn push_color(out: &mut String, base: u8, c: Color) {
    match c {
        Color::Default => {}
        Color::Rgb(r, g, b) => out.push_str(&format!(";{base};2;{r};{g};{b}")),
        Color::Indexed(n) => out.push_str(&format!(";{base};5;{n}")),
    }
}

/// The canonical form of a grid (see the crate docs): byte-identical for equal grids.
pub fn write_canonical(grid: &Grid) -> String {
    let mut out = String::new();
    for row in 0..grid.rows {
        let mut prev = Style::default();
        for col in 0..grid.cols {
            let cell = &grid.cells[row * grid.cols + col];
            if cell.width == 0 {
                continue;
            }
            let s = normalized(cell);
            if s != prev {
                out.push_str("\x1b[0");
                let a = s.attrs;
                for (on, code) in [
                    (a.bold, ";1"),
                    (a.faint, ";2"),
                    (a.italic, ";3"),
                    (a.underline, ";4"),
                    (a.reverse, ";7"),
                    (a.crossed, ";9"),
                ] {
                    if on {
                        out.push_str(code);
                    }
                }
                push_color(&mut out, 38, s.fg);
                push_color(&mut out, 48, s.bg);
                out.push('m');
                prev = s;
            }
            out.push_str(&cell.text);
        }
        if prev != Style::default() {
            out.push_str("\x1b[0m");
        }
        out.push('\n');
    }
    out
}

/// The xterm palette for indices 0–15, then the 6×6×6 cube and the gray ramp.
pub fn indexed_rgb(n: u8) -> [u8; 3] {
    const ANSI: [[u8; 3]; 16] = [
        [0x00, 0x00, 0x00],
        [0xcd, 0x00, 0x00],
        [0x00, 0xcd, 0x00],
        [0xcd, 0xcd, 0x00],
        [0x00, 0x00, 0xee],
        [0xcd, 0x00, 0xcd],
        [0x00, 0xcd, 0xcd],
        [0xe5, 0xe5, 0xe5],
        [0x7f, 0x7f, 0x7f],
        [0xff, 0x00, 0x00],
        [0x00, 0xff, 0x00],
        [0xff, 0xff, 0x00],
        [0x5c, 0x5c, 0xff],
        [0xff, 0x00, 0xff],
        [0x00, 0xff, 0xff],
        [0xff, 0xff, 0xff],
    ];
    match n {
        0..=15 => ANSI[n as usize],
        16..=231 => {
            let i = n - 16;
            let level = |v: u8| if v == 0 { 0 } else { 55 + 40 * v };
            [level(i / 36), level((i / 6) % 6), level(i % 6)]
        }
        _ => {
            let v = 8 + 10 * (n - 232);
            [v, v, v]
        }
    }
}

fn rgb_of(c: Color, default: [u8; 3]) -> [u8; 3] {
    match c {
        Color::Default => default,
        Color::Rgb(r, g, b) => [r, g, b],
        Color::Indexed(n) => indexed_rgb(n),
    }
}

/// What a cell looks like, for comparing: default colors resolved to the palette, reverse
/// applied, and a space without underline or crossed carrying no ink (fg, bold, faint and italic
/// fixed), since nothing of them shows.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Effective {
    pub text: String,
    pub fg: [u8; 3],
    pub bg: [u8; 3],
    pub bold: bool,
    pub faint: bool,
    pub italic: bool,
    pub underline: bool,
    pub crossed: bool,
}

/// Resolve a cell against the theme's default foreground and background.
pub fn resolve(cell: &Cell, palette_fg: [u8; 3], palette_bg: [u8; 3]) -> Effective {
    let mut fg = rgb_of(cell.fg, palette_fg);
    let mut bg = rgb_of(cell.bg, palette_bg);
    if cell.attrs.reverse {
        std::mem::swap(&mut fg, &mut bg);
    }
    let a = cell.attrs;
    let blank = (cell.text == " " || cell.text.is_empty()) && !a.underline && !a.crossed;
    Effective {
        text: cell.text.clone(),
        fg: if blank { [0, 0, 0] } else { fg },
        bg,
        bold: a.bold && !blank,
        faint: a.faint && !blank,
        italic: a.italic && !blank,
        underline: a.underline,
        crossed: a.crossed,
    }
}

/// How two snapshots differ, cell by cell.
#[derive(Clone, Debug, PartialEq)]
pub struct Diff {
    /// The union size.
    pub cols: usize,
    pub rows: usize,
    pub size_matches: bool,
    pub differing: usize,
    pub total: usize,
    /// `round(100 × differing / total, 2)`.
    pub percent: f64,
    /// Whether each cell of the union differs, row-major.
    pub mask: Vec<bool>,
}

impl Diff {
    pub fn to_json(&self) -> String {
        serde_json::json!({
            "cols": self.cols, "rows": self.rows, "sizeMatches": self.size_matches,
            "differing": self.differing, "total": self.total, "percent": self.percent,
        })
        .to_string()
    }
}

/// Compare `a` and `b` over the union of their areas; a cell present on one side only differs.
pub fn diff(a: &Grid, b: &Grid, palette_fg: [u8; 3], palette_bg: [u8; 3]) -> Diff {
    let cols = a.cols.max(b.cols);
    let rows = a.rows.max(b.rows);
    let mut mask = vec![false; cols * rows];
    let mut differing = 0;
    for row in 0..rows {
        for col in 0..cols {
            let same = match (a.get(col, row), b.get(col, row)) {
                (Some(x), Some(y)) => {
                    resolve(x, palette_fg, palette_bg) == resolve(y, palette_fg, palette_bg)
                }
                _ => false,
            };
            if !same {
                mask[row * cols + col] = true;
                differing += 1;
            }
        }
    }
    let total = cols * rows;
    let percent = if total == 0 {
        0.0
    } else {
        (10000.0 * differing as f64 / total as f64).round() / 100.0
    };
    Diff {
        cols,
        rows,
        size_matches: a.cols == b.cols && a.rows == b.rows,
        differing,
        total,
        percent,
        mask,
    }
}

/// Write RGBA pixels as a PNG.
pub fn write_png(
    path: &std::path::Path,
    width: u32,
    height: u32,
    rgba: &[u8],
) -> Result<(), String> {
    let file = std::fs::File::create(path).map_err(|e| format!("{}: {e}", path.display()))?;
    let mut enc = png::Encoder::new(std::io::BufWriter::new(file), width, height);
    enc.set_color(png::ColorType::Rgba);
    enc.set_depth(png::BitDepth::Eight);
    let mut w = enc.write_header().map_err(|e| e.to_string())?;
    w.write_image_data(rgba).map_err(|e| e.to_string())?;
    w.finish().map_err(|e| e.to_string())
}

#[cfg(test)]
mod tests;
