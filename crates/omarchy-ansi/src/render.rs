//! Rasterizing a snapshot grid with the resolved monospace font.

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::process::Command;

use ab_glyph::{Font, FontVec, PxScale, point};

use crate::{Grid, boxdraw, indexed_rgb};

/// The font files a render draws with.
#[derive(Clone, Debug, Default)]
pub struct Faces {
    pub regular: PathBuf,
    pub bold: Option<PathBuf>,
    pub italic: Option<PathBuf>,
    pub bold_italic: Option<PathBuf>,
}

fn fc_match(pattern: &str) -> Option<(String, PathBuf)> {
    let out = Command::new("fc-match")
        .args(["-f", "%{family}\n%{file}", pattern])
        .output()
        .ok()?;
    if !out.status.success() {
        return None;
    }
    let s = String::from_utf8_lossy(&out.stdout);
    let mut lines = s.lines();
    let families = lines.next()?.to_string();
    let file = PathBuf::from(lines.next()?.trim());
    file.exists().then_some((families, file))
}

impl Faces {
    /// The core's resolved family (`fc-match monospace`, or `OMARCHY_UI_FONT_FAMILY`): its regular
    /// face, and its Bold, Italic and Bold Italic styles from fontconfig when the family has them.
    pub fn resolve(tokens: &omarchy_theme::Tokens) -> Result<Faces, String> {
        let family = tokens.font.resolved_family.clone();
        let regular = tokens
            .font
            .file
            .clone()
            .or_else(|| fc_match(&family).map(|(_, f)| f))
            .ok_or_else(|| format!("no font file for {family}"))?;
        let style = |s: &str| {
            fc_match(&format!("{family}:style={s}"))
                .filter(|(fams, _)| fams.split(',').any(|f| f.trim() == family))
                .map(|(_, f)| f)
                .filter(|f| *f != regular)
        };
        Ok(Faces {
            bold: style("Bold"),
            italic: style("Italic"),
            bold_italic: style("Bold Italic"),
            regular,
        })
    }
}

/// How to render: the font size in px at scale 1 (12 by default), the device scale (2), the
/// default colors and the faces.
#[derive(Clone, Debug)]
pub struct RenderOptions {
    pub font_size_px: f32,
    pub scale: f32,
    pub fg: [u8; 3],
    pub bg: [u8; 3],
    pub faces: Faces,
}

impl RenderOptions {
    /// Defaults from the core's tokens under the current environment.
    pub fn from_tokens(tokens: &omarchy_theme::Tokens) -> Result<RenderOptions, String> {
        let p = &tokens.palette;
        Ok(RenderOptions {
            font_size_px: 12.0,
            scale: 2.0,
            fg: [p.foreground.r, p.foreground.g, p.foreground.b],
            bg: [p.background.r, p.background.g, p.background.b],
            faces: Faces::resolve(tokens)?,
        })
    }
}

/// Opaque RGBA pixels.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Image {
    pub width: u32,
    pub height: u32,
    pub rgba: Vec<u8>,
    /// The cell size in pixels.
    pub cell_width: u32,
    pub cell_height: u32,
}

impl Image {
    fn new(width: u32, height: u32, cell_width: u32, cell_height: u32) -> Image {
        Image {
            width,
            height,
            rgba: vec![255; (width * height * 4) as usize],
            cell_width,
            cell_height,
        }
    }

    /// `out = round(ink × c + under × (1 − c))` per channel, sRGB 8-bit.
    pub(crate) fn blend(&mut self, x: i64, y: i64, ink: [u8; 3], c: f32) {
        if x < 0 || y < 0 || x >= self.width as i64 || y >= self.height as i64 || c <= 0.0 {
            return;
        }
        let c = c.min(1.0);
        let i = ((y as u32 * self.width + x as u32) * 4) as usize;
        for (k, ink) in ink.iter().enumerate() {
            let under = self.rgba[i + k] as f32;
            self.rgba[i + k] = (*ink as f32 * c + under * (1.0 - c)).round() as u8;
        }
    }

    pub(crate) fn fill(&mut self, x0: i64, y0: i64, x1: i64, y1: i64, ink: [u8; 3], c: f32) {
        for y in y0.max(0)..y1.min(self.height as i64) {
            for x in x0.max(0)..x1.min(self.width as i64) {
                self.blend(x, y, ink, c);
            }
        }
    }

    /// Encode as a PNG file.
    pub fn write_png(&self, path: &Path) -> Result<(), String> {
        crate::write_png(path, self.width, self.height, &self.rgba)
    }
}

struct Fonts {
    faces: [Option<FontVec>; 4],
    fallback: HashMap<char, Option<usize>>,
    fallback_fonts: Vec<FontVec>,
    fallback_paths: HashMap<PathBuf, usize>,
}

fn load(path: &Path) -> Result<FontVec, String> {
    let data = std::fs::read(path).map_err(|e| format!("{}: {e}", path.display()))?;
    FontVec::try_from_vec(data).map_err(|e| format!("{}: {e}", path.display()))
}

impl Fonts {
    fn new(f: &Faces) -> Result<Fonts, String> {
        let opt = |p: &Option<PathBuf>| p.as_deref().and_then(|p| load(p).ok());
        Ok(Fonts {
            faces: [
                Some(load(&f.regular)?),
                opt(&f.bold),
                opt(&f.italic),
                opt(&f.bold_italic),
            ],
            fallback: HashMap::new(),
            fallback_fonts: Vec::new(),
            fallback_paths: HashMap::new(),
        })
    }

    fn face(&self, bold: bool, italic: bool) -> &FontVec {
        let i = match (bold, italic) {
            (false, false) => 0,
            (true, false) => 1,
            (false, true) => 2,
            (true, true) => 3,
        };
        self.faces[i]
            .as_ref()
            .or(if bold && italic {
                self.faces[1].as_ref()
            } else {
                None
            })
            .unwrap_or(self.faces[0].as_ref().unwrap())
    }

    /// The face to draw `c` with: the styled face, else the regular one, else a fallback font
    /// fontconfig finds for the codepoint.
    fn for_char(&mut self, c: char, bold: bool, italic: bool) -> Option<&FontVec> {
        if self.face(bold, italic).glyph_id(c).0 != 0 {
            return Some(self.face(bold, italic));
        }
        if self.faces[0].as_ref().unwrap().glyph_id(c).0 != 0 {
            return self.faces[0].as_ref();
        }
        if !self.fallback.contains_key(&c) {
            let found = fc_match(&format!(":charset={:x}", c as u32)).and_then(|(_, path)| {
                if let Some(i) = self.fallback_paths.get(&path) {
                    return Some(*i);
                }
                let font = load(&path).ok()?;
                self.fallback_fonts.push(font);
                let i = self.fallback_fonts.len() - 1;
                self.fallback_paths.insert(path, i);
                Some(i)
            });
            let found = found.filter(|i| self.fallback_fonts[*i].glyph_id(c).0 != 0);
            self.fallback.insert(c, found);
        }
        self.fallback[&c].map(|i| &self.fallback_fonts[i])
    }
}

/// The em size `px` as ab_glyph's scale (which is the ascent-to-descent height).
fn px_scale(font: &FontVec, px: f32) -> PxScale {
    let upem = font.units_per_em().unwrap_or(1000.0);
    PxScale::from(px * font.height_unscaled() / upem)
}

/// Cell metrics at pixel size `p` for the regular face.
struct Metrics {
    cell_w: u32,
    cell_h: u32,
    baseline: i64,
}

fn metrics(font: &FontVec, p: f32) -> Metrics {
    let upem = font.units_per_em().unwrap_or(1000.0);
    let k = p / upem;
    let adv = font.h_advance_unscaled(font.glyph_id('M')) * k;
    let h =
        (font.ascent_unscaled() - font.descent_unscaled() + font.line_gap_unscaled().max(0.0)) * k;
    Metrics {
        cell_w: (adv.round() as u32).max(1),
        cell_h: (h.ceil() as u32).max(1),
        baseline: (font.ascent_unscaled() * k).round() as i64,
    }
}

/// Rasterize `grid`.
///
/// - Pixel size `P = font_size_px × scale`. Cell width `round(advance of "M")`, cell height
///   `ceil(ascent − descent + line gap)` from the regular face's `hhea` metrics at `P`; the
///   baseline is `round(ascent)` from the cell's top. The image is `cols × cell width` by
///   `rows × cell height`, opaque.
/// - Default colors are `fg` / `bg`; ANSI 0–15 the xterm palette; reverse swaps fg and bg first;
///   faint ink is `round((fg + bg) / 2)` per channel against the cell's bg.
/// - Every cell's background is painted first, then the text, left to right and top to bottom,
///   unclipped: a glyph wider than its cell overflows into the next one, as foot draws it.
/// - Glyphs blend in sRGB 8-bit: `round(ink × coverage + under × (1 − coverage))`. Bold, italic
///   and bold italic use those faces when the family has them. A character no face has comes
///   from the font `fc-match :charset=<hex>` names, else it's an outlined box.
/// - Box drawing (U+2500–U+254F, U+256D–U+2570 as quarter-circle arcs, U+2574–U+257F), block
///   elements (U+2580–U+259F; shades at 25/50/75% ink) and braille (U+2800–U+28FF, dots on a
///   2 × 4 grid) are drawn as pixels, not from the font. Light lines are `max(1, round(P / 12))`
///   thick, heavy ones twice that, centered in the cell.
/// - Underline: `max(1, round(P / 14))` thick, its top at `baseline + max(1, round(P × 0.1))`.
///   Crossed: the same thickness, its top at `baseline − round(P × 0.3)`.
pub fn render(grid: &Grid, opts: &RenderOptions) -> Result<Image, String> {
    let mut fonts = Fonts::new(&opts.faces)?;
    let p = opts.font_size_px * opts.scale;
    let m = metrics(fonts.faces[0].as_ref().unwrap(), p);
    let (cw, ch) = (m.cell_w, m.cell_h);
    let mut img = Image::new(grid.cols as u32 * cw, grid.rows as u32 * ch, cw, ch);
    let light = (p / 12.0).round().max(1.0) as i64;
    let deco = (p / 14.0).round().max(1.0) as i64;

    let colors = |cell: &crate::Cell| {
        let rgb = |c: crate::Color, d: [u8; 3]| match c {
            crate::Color::Default => d,
            crate::Color::Rgb(r, g, b) => [r, g, b],
            crate::Color::Indexed(n) => indexed_rgb(n),
        };
        let (mut fg, mut bg) = (rgb(cell.fg, opts.fg), rgb(cell.bg, opts.bg));
        if cell.attrs.reverse {
            std::mem::swap(&mut fg, &mut bg);
        }
        if cell.attrs.faint {
            fg = [0, 1, 2].map(|k| (fg[k] as u16 + bg[k] as u16).div_ceil(2) as u8);
        }
        (fg, bg)
    };

    for row in 0..grid.rows {
        for col in 0..grid.cols {
            let (_, bg) = colors(&grid.cells[row * grid.cols + col]);
            let (x, y) = ((col as u32 * cw) as i64, (row as u32 * ch) as i64);
            img.fill(x, y, x + cw as i64, y + ch as i64, bg, 1.0);
        }
    }

    for row in 0..grid.rows {
        for col in 0..grid.cols {
            let cell = &grid.cells[row * grid.cols + col];
            if cell.width == 0 {
                continue;
            }
            let (ink, _) = colors(cell);
            let x0 = (col as u32 * cw) as i64;
            let y0 = (row as u32 * ch) as i64;
            let wpx = cw as i64 * cell.width as i64;
            let mut chars = cell.text.chars();
            let first = chars.next();
            let single = first.is_some() && chars.next().is_none();
            let drawn = match (single, first) {
                (true, Some(c)) => {
                    boxdraw::draw(&mut img, c, x0, y0, cw as i64, ch as i64, light, ink)
                }
                _ => false,
            };
            if !drawn && cell.text != " " {
                let mut x = x0 as f32;
                for c in cell.text.chars() {
                    match fonts.for_char(c, cell.attrs.bold, cell.attrs.italic) {
                        Some(font) => {
                            let scale = px_scale(font, p);
                            let id = font.glyph_id(c);
                            let glyph = id
                                .with_scale_and_position(scale, point(x, (y0 + m.baseline) as f32));
                            if let Some(out) = font.outline_glyph(glyph) {
                                let b = out.px_bounds();
                                let (bx, by) = (b.min.x as i64, b.min.y as i64);
                                out.draw(|gx, gy, c| {
                                    img.blend(bx + gx as i64, by + gy as i64, ink, c)
                                });
                            }
                            let upem = font.units_per_em().unwrap_or(1000.0);
                            x += font.h_advance_unscaled(id) * p / upem;
                        }
                        None => {
                            // No font has it: an outlined box in the cell.
                            let (l, t, r, btm) =
                                (x as i64 + 1, y0 + 2, x as i64 + wpx - 1, y0 + ch as i64 - 2);
                            img.fill(l, t, r, t + 1, ink, 1.0);
                            img.fill(l, btm - 1, r, btm, ink, 1.0);
                            img.fill(l, t, l + 1, btm, ink, 1.0);
                            img.fill(r - 1, t, r, btm, ink, 1.0);
                            x += wpx as f32;
                        }
                    }
                }
            }
            if cell.attrs.underline {
                let top = y0 + m.baseline + ((p * 0.1).round() as i64).max(1);
                img.fill(
                    x0,
                    top,
                    x0 + wpx,
                    (top + deco).min(y0 + ch as i64),
                    ink,
                    1.0,
                );
            }
            if cell.attrs.crossed {
                let top = y0 + m.baseline - (p * 0.3).round() as i64;
                img.fill(x0, top, x0 + wpx, top + deco, ink, 1.0);
            }
        }
    }
    Ok(img)
}
