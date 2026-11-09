//! Snapshots (spec/terminal.md, Snapshots): a buffer written as the canonical ANSI text every
//! terminal binding's `--snap` writes, and the `--snap out.ans [--size COLSxROWS]` an app gets
//! from [`from_args`].

use ratatui::buffer::Buffer;
use ratatui::layout::Rect;
use ratatui::style::{Color, Modifier};
use unicode_width::UnicodeWidthStr;

use crate::{Draw, Theme};

fn color(c: Color) -> omarchy_ansi::Color {
    match c {
        Color::Rgb(r, g, b) => omarchy_ansi::Color::Rgb(r, g, b),
        Color::Indexed(n) => omarchy_ansi::Color::Indexed(n),
        Color::Reset => omarchy_ansi::Color::Default,
        // Named colors are the 16-color fallback's (not in snapshots, which are truecolor).
        other => {
            let n = match other {
                Color::Black => 0,
                Color::Red => 1,
                Color::Green => 2,
                Color::Yellow => 3,
                Color::Blue => 4,
                Color::Magenta => 5,
                Color::Cyan => 6,
                Color::Gray => 7,
                Color::DarkGray => 8,
                Color::LightRed => 9,
                Color::LightGreen => 10,
                Color::LightYellow => 11,
                Color::LightBlue => 12,
                Color::LightMagenta => 13,
                Color::LightCyan => 14,
                _ => 15,
            };
            omarchy_ansi::Color::Indexed(n)
        }
    }
}

/// The buffer as a grid of the shared snapshot model. Call [`Theme::finish`] on it first.
pub fn grid(buf: &Buffer) -> omarchy_ansi::Grid {
    let area = buf.area;
    let mut g = omarchy_ansi::Grid::new(area.width as usize, area.height as usize);
    for y in 0..area.height {
        let mut x = 0;
        while x < area.width {
            let cell = &buf[(area.x + x, area.y + y)];
            let sym = cell.symbol();
            let w = UnicodeWidthStr::width(sym).clamp(1, 2) as u16;
            let w = if x + w > area.width { 1 } else { w };
            let m = cell.modifier;
            let out = g.get_mut(x as usize, y as usize).unwrap();
            out.text = if UnicodeWidthStr::width(sym) == 0 { " ".into() } else { sym.to_string() };
            out.width = w as u8;
            out.fg = color(cell.fg);
            out.bg = color(cell.bg);
            out.attrs = omarchy_ansi::Attrs {
                bold: m.contains(Modifier::BOLD),
                faint: m.contains(Modifier::DIM),
                italic: m.contains(Modifier::ITALIC),
                underline: m.contains(Modifier::UNDERLINED),
                reverse: m.contains(Modifier::REVERSED),
                crossed: m.contains(Modifier::CROSSED_OUT),
            };
            if w == 2 {
                let cont = g.get_mut(x as usize + 1, y as usize).unwrap();
                cont.text.clear();
                cont.width = 0;
            }
            x += w;
        }
    }
    g
}

/// The buffer as canonical ANSI snapshot text.
pub fn ansi(buf: &Buffer) -> String {
    omarchy_ansi::write_canonical(&grid(buf))
}

/// Draw `view` into a fresh `cols` × `rows` buffer (rows 0: the view's natural height), finish it
/// in truecolor, and return the snapshot text.
pub fn render(t: &mut Theme, view: &dyn Draw, cols: u16, rows: u16) -> String {
    t.set_depth(crate::Depth::True);
    let (_, h) = view.size(t);
    let rows = if rows == 0 { h.max(1) } else { rows };
    let area = Rect::new(0, 0, cols, rows);
    let mut buf = Buffer::empty(area);
    t.clear_hits();
    view.draw(t, area, &mut buf);
    t.finish(&mut buf);
    ansi(&buf)
}

/// `--snap out.ans [--size COLSxROWS]` from the command line: renders `view` with `t` and writes
/// the snapshot, returning true when it did (the app then exits instead of running). Width
/// defaults to 96 columns, rows to the view's natural height.
pub fn from_args(t: &mut Theme, view: &dyn Draw) -> std::io::Result<bool> {
    let args: Vec<String> = std::env::args().collect();
    let Some(i) = args.iter().position(|a| a == "--snap") else { return Ok(false) };
    let out = args.get(i + 1).ok_or_else(|| std::io::Error::other("--snap needs a file"))?;
    let (cols, rows) = size_arg(&args).unwrap_or((96, 0));
    std::fs::write(out, render(t, view, cols, rows))?;
    Ok(true)
}

/// `--size COLSxROWS` from `args`.
pub fn size_arg(args: &[String]) -> Option<(u16, u16)> {
    let i = args.iter().position(|a| a == "--size")?;
    let (c, r) = args.get(i + 1)?.split_once('x')?;
    Some((c.parse().ok()?, r.parse().ok()?))
}
