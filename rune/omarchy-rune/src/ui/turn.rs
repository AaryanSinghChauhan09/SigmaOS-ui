//! A Nerd Font glyph as a Rune `Icon`, so it can turn on the render clock.
//!
//! Rune turns text with `El::rotate` (a sprung angle, drawn through a turned layer), but its
//! repeating `LoopSpec::rotation` turns only an element's `icon`. A spinner needs the repeating
//! kind, so the glyph's outline is read from the font fontconfig picks for it (the file Rune's
//! text draws it from) and handed to Rune as an SVG path, placed in its square exactly where the
//! text would put its ink.

use std::collections::HashMap;
use std::path::PathBuf;
use std::process::Command as Process;
use std::sync::Mutex;

use rune::text::Icon;
use swash::FontRef;
use swash::scale::ScaleContext;
use swash::zeno::{Command, PathData};

/// Turned glyphs made so far, by glyph, font, size and placement; each lives for the process
/// (a handful: one per spinner look).
static MADE: Mutex<Option<HashMap<(String, String, u32, [i32; 2]), Option<&'static Icon>>>> =
    Mutex::new(None);

/// `glyph` in `family` at `size` points, its outline's ink centred on `ink_centre` inside a
/// `side`-point square. `None` when no font has it (or it has no outline), and the caller
/// draws text instead.
pub(crate) fn glyph_icon(
    glyph: &str,
    family: &str,
    size: f32,
    side: f32,
    ink_centre: [f32; 2],
) -> Option<&'static Icon> {
    let mut chars = glyph.chars();
    let (ch, None) = (chars.next()?, chars.next()) else {
        return None;
    };
    // Placement to 1/64 pt, as Rune snaps element rects.
    let at = ink_centre.map(|v| (v * 64.0).round() as i32);
    let key = (
        glyph.to_string(),
        family.to_string(),
        (size * 64.0).round() as u32,
        at,
    );
    let mut made = MADE.lock().unwrap_or_else(|e| e.into_inner());
    let made = made.get_or_insert_with(HashMap::new);
    if let Some(&icon) = made.get(&key) {
        return icon;
    }
    let icon = font_for(family, ch).and_then(|(file, index)| {
        let data = std::fs::read(file).ok()?;
        let body = path(&data, index, ch, size, ink_centre)?;
        Some(&*Box::leak(Box::new(Icon {
            name: "omarchy-turning-glyph",
            view: side,
            body: Box::leak(body.into_boxed_str()),
        })))
    });
    made.insert(key, icon);
    icon
}

/// The outline as an SVG `<path>` in a y-down square, its bounds' centre at `centre`.
fn path(data: &[u8], index: u32, ch: char, size: f32, centre: [f32; 2]) -> Option<String> {
    let font = FontRef::from_index(data, index as usize)?;
    let id = font.charmap().map(ch);
    if id == 0 {
        return None;
    }
    let mut cx = ScaleContext::new();
    let mut scaler = cx.builder(font).size(size).hint(false).build();
    let outline = scaler.scale_outline(id)?;
    let b = outline.bounds();
    if b.max.x <= b.min.x || b.max.y <= b.min.y {
        return None;
    }
    // Font units are y-up from the baseline; the square is y-down from its top-left.
    let (dx, dy) = (
        centre[0] - (b.min.x + b.max.x) / 2.0,
        centre[1] + (b.min.y + b.max.y) / 2.0,
    );
    let p = |pt: swash::zeno::Point| format!("{:.3} {:.3}", pt.x + dx, dy - pt.y);
    let mut d = String::new();
    for c in outline.path().commands() {
        match c {
            Command::MoveTo(a) => d += &format!("M{}", p(a)),
            Command::LineTo(a) => d += &format!("L{}", p(a)),
            Command::QuadTo(a, b) => d += &format!("Q{} {}", p(a), p(b)),
            Command::CurveTo(a, b, c) => d += &format!("C{} {} {}", p(a), p(b), p(c)),
            Command::Close => d += "Z",
        }
    }
    Some(format!(r#"<path d="{d}" fill="currentColor"/>"#))
}

/// The font file (and face index) fontconfig draws `ch` from in `family`.
fn font_for(family: &str, ch: char) -> Option<(PathBuf, u32)> {
    let pattern = format!("{family}:charset={:x}", ch as u32);
    let out = Process::new("fc-match")
        .args(["-f", "%{file}\n%{index}", &pattern])
        .output()
        .ok()?;
    let out = String::from_utf8_lossy(&out.stdout).into_owned();
    let mut lines = out.lines();
    let file = PathBuf::from(lines.next()?.trim());
    let index = lines
        .next()
        .and_then(|i| i.trim().parse().ok())
        .unwrap_or(0);
    file.exists().then_some((file, index))
}
