//! The Ratatui gallery: every Omarchy UI component in the live theme, section for section with the
//! other galleries (spec/gallery.md), at the cell sizes spec/terminal.md lists; the reference of
//! the terminal family in `bin/review`.
//!
//!   omarchy-ratatui-gallery                                   the gallery in this terminal
//!   omarchy-ratatui-gallery --section lists                   ...starting on a section
//!   omarchy-ratatui-gallery --list-sections                   the sections, in order
//!   omarchy-ratatui-gallery --snap out.ans --section buttons [--size 96x0]
//!                                                             one section as an ANSI snapshot
//!
//! In the window: `[` and `]` (or Ctrl+←/→) change section, Tab and Shift+Tab walk the section's
//! controls, the focused one takes the keys, the mouse clicks and hovers, the wheel and Page
//! Up/Down scroll, `q` quits.

mod interact;
mod sections;
mod state;

use omarchy_ratatui::crossterm::event::{Event, KeyCode, KeyEventKind, KeyModifiers, MouseButton, MouseEventKind};
use omarchy_ratatui::layout::Col;
use omarchy_ratatui::paint::{self, Ink};
use omarchy_ratatui::ratatui::Frame;
use omarchy_ratatui::ratatui::buffer::Buffer;
use omarchy_ratatui::ratatui::layout::Rect;
use omarchy_ratatui::ui::{DropdownList, KeyCombo};
use omarchy_ratatui::{App, Draw, Flow, Theme, Themed};

use sections::{COLS, SECTIONS, page};
use state::{State, THEMES};

struct Gallery {
    theme: Theme,
    state: State,
    section: usize,
    scroll: u16,
    /// The body's height last frame (for Page Up/Down and clamping the scroll).
    body: (u16, u16),
}

impl Themed for Gallery {
    fn theme(&self) -> &Theme {
        &self.theme
    }
    fn theme_mut(&mut self) -> &mut Theme {
        &mut self.theme
    }
}

/// Draw section `name` into a buffer of its own (natural height plus room for an open dropdown),
/// with hits recorded in that buffer's coordinates.
fn draw_page(s: &State, t: &Theme, name: &str) -> Buffer {
    let p: Col = page(s, t, name);
    let (_, h) = p.size(t);
    let area = Rect::new(0, 0, COLS, h + 8);
    let mut buf = Buffer::empty(area);
    p.draw(t, Rect { height: h, ..area }, &mut buf);
    if name == "inputs"
        && s.dropdown_open
        && let Some(r) = t.find("theme")
    {
        let list = DropdownList::new(THEMES, Some(s.theme_choice)).highlighted(Some(s.dropdown_row)).width(r.width).key("theme-list");
        let (w, lh) = list.size(t);
        list.draw(t, Rect::new(r.x, r.y + 1, w, lh), &mut buf);
    }
    buf
}

impl Gallery {
    fn name(&self) -> &'static str {
        SECTIONS[self.section].0
    }
    fn go(&mut self, d: isize) {
        self.section = (self.section as isize + d).rem_euclid(SECTIONS.len() as isize) as usize;
        self.scroll = 0;
        self.state.focus = None;
        self.state.dropdown_open = false;
    }
    fn cycle_focus(&mut self, back: bool) {
        let fs = interact::focusables(self.name());
        if fs.is_empty() {
            return;
        }
        let i = self.state.focus.and_then(|f| fs.iter().position(|x| *x == f));
        let n = fs.len();
        let next = match (i, back) {
            (None, false) => 0,
            (None, true) => n - 1,
            (Some(i), false) => (i + 1) % n,
            (Some(i), true) => (i + n - 1) % n,
        };
        self.state.focus = Some(fs[next]);
    }
}

impl App for Gallery {
    fn draw(&mut self, frame: &mut Frame) {
        let t = &self.theme;
        let area = frame.area();
        let buf = frame.buffer_mut();
        // Header: the gallery, the section and where it is in the list.
        paint::text(t, buf, 1, 0, "Omarchy UI · Ratatui", area.width, Ink::new(t.fg()).bold(true));
        let (_, title) = SECTIONS[self.section];
        let right = format!("{title}  {}/{}", self.section + 1, SECTIONS.len());
        paint::text_right(t, buf, area.width.saturating_sub(1), 0, &right, t.dim());
        paint::hline(t, buf, 0, 1, area.width, "─", t.track());
        // Footer: the keys.
        let fy = area.height.saturating_sub(1);
        let mut x = 1;
        for (combo, what) in [("Tab", "focus"), ("[", ""), ("]", "section"), ("q", "quit")] {
            let kc = KeyCombo::new(combo);
            let w = kc.size(t).0;
            kc.draw(t, Rect::new(x, fy, w, 1).intersection(area), buf);
            x += w + 1;
            if !what.is_empty() {
                x += paint::text(t, buf, x, fy, what, area.width.saturating_sub(x), t.dim()) + 2;
            }
        }
        // The section, scrolled, between them.
        let body = Rect::new(0, 2, area.width.min(COLS), area.height.saturating_sub(3));
        let hits = t.hits_len();
        let page = draw_page(&self.state, t, self.name());
        let natural = page.area.height.saturating_sub(8);
        self.body = (body.height, natural);
        self.scroll = self.scroll.min(natural.saturating_sub(body.height));
        let scroll = self.scroll;
        for dy in 0..body.height {
            for dx in 0..body.width {
                if let (Some(src), Some(dst)) = (page.cell((dx, scroll + dy)), buf.cell_mut((body.x + dx, body.y + dy))) {
                    *dst = src.clone();
                }
            }
        }
        let shown = Rect::new(0, scroll, body.width, body.height);
        t.remap_hits(hits, |r| {
            let r = r.intersection(shown);
            (r.height > 0).then(|| Rect { y: r.y - scroll + body.y, ..r })
        });
    }

    fn event(&mut self, ev: &Event) -> Flow {
        if omarchy_ratatui::is_quit(ev) && !matches!(self.state.focus, Some("search" | "name" | "password" | "searchlist")) {
            return Flow::Quit;
        }
        match ev {
            Event::Key(k) if k.kind != KeyEventKind::Release => {
                if k.modifiers.contains(KeyModifiers::CONTROL) && k.code == KeyCode::Char('c') {
                    return Flow::Quit;
                }
                if interact::key(&mut self.state, k) {
                    return Flow::Continue;
                }
                match k.code {
                    KeyCode::Char(']') => self.go(1),
                    KeyCode::Char('[') => self.go(-1),
                    KeyCode::Right if k.modifiers.contains(KeyModifiers::CONTROL) => self.go(1),
                    KeyCode::Left if k.modifiers.contains(KeyModifiers::CONTROL) => self.go(-1),
                    KeyCode::Tab => self.cycle_focus(false),
                    KeyCode::BackTab => self.cycle_focus(true),
                    KeyCode::Esc => self.state.focus = None,
                    KeyCode::PageDown => self.scroll = self.scroll.saturating_add(self.body.0.saturating_sub(1)),
                    KeyCode::PageUp => self.scroll = self.scroll.saturating_sub(self.body.0.saturating_sub(1)),
                    KeyCode::Down if self.state.focus.is_none() => self.scroll = self.scroll.saturating_add(1),
                    KeyCode::Up if self.state.focus.is_none() => self.scroll = self.scroll.saturating_sub(1),
                    _ => {}
                }
            }
            Event::Mouse(m) => match m.kind {
                MouseEventKind::ScrollDown => self.scroll = self.scroll.saturating_add(3),
                MouseEventKind::ScrollUp => self.scroll = self.scroll.saturating_sub(3),
                MouseEventKind::Down(MouseButton::Left) => {
                    if let Some(key) = self.theme.hit_at(m.column, m.row) {
                        let rect = self.theme.find(&key).unwrap_or_default();
                        let name = self.name();
                        if let Some(f) = interact::focus_of(name, &key) {
                            self.state.focus = Some(f);
                        }
                        interact::click(&mut self.state, &self.theme, name, &key, m.column, rect);
                    }
                }
                MouseEventKind::Moved => {
                    if let Some(key) = self.theme.hit_at(m.column, m.row) {
                        interact::hover(&mut self.state, &key, m.column, m.row);
                    }
                }
                _ => {}
            },
            _ => {}
        }
        Flow::Continue
    }

    fn animating(&self) -> bool {
        // Spinners and the indeterminate bar turn on the progress page.
        self.name() == "progress" && !self.theme.reduce_motion()
    }
}

fn arg(args: &[String], name: &str) -> Option<String> {
    args.iter().position(|a| a == name).and_then(|i| args.get(i + 1)).cloned()
}

fn main() -> std::io::Result<()> {
    let args: Vec<String> = std::env::args().collect();
    if args.iter().any(|a| a == "--list-sections") {
        for (n, _) in SECTIONS {
            println!("{n}");
        }
        return Ok(());
    }
    let section = arg(&args, "--section");
    if let Some(i) = section.as_deref().filter(|s| !SECTIONS.iter().any(|(n, _)| n == s)) {
        eprintln!("no section {i} (--list-sections)");
        std::process::exit(2);
    }
    if let Some(out) = arg(&args, "--snap") {
        let name = section.unwrap_or_else(|| "buttons".into());
        let (cols, rows) = omarchy_ratatui::snap::size_arg(&args).unwrap_or((COLS, 0));
        let mut theme = Theme::load();
        let state = State::init();
        let view = page(&state, &theme, &name);
        std::fs::write(out, omarchy_ratatui::snap::render(&mut theme, &view, cols, rows))?;
        return Ok(());
    }
    let start = section.and_then(|s| SECTIONS.iter().position(|(n, _)| *n == s)).unwrap_or(0);
    omarchy_ratatui::run(Gallery { theme: Theme::load(), state: State::init(), section: start, scroll: 0, body: (0, 0) })?;
    Ok(())
}

#[cfg(test)]
mod tests;
