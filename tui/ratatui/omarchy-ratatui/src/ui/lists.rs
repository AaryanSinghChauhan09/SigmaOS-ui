//! Rows, the keyboard cursor that walks them, the scroll view they sit in, and the small pieces
//! rows and grids are made of (spec/terminal.md, Lists).

use crossterm::event::{KeyCode, KeyEvent, KeyModifiers};
use omarchy_theme::Rgba;
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::{Tone, look};
use crate::paint::{self, Ink, edges, fill, put, text, text_center, text_fit, text_right, width};
use crate::{Draw, Theme};

// ---------------------------------------------------------------------------------------------
// CursorModel

#[derive(Clone, Debug, PartialEq)]
pub struct CursorSection {
    pub name: String,
    pub count: usize,
}

/// What a key did to a [`CursorModel`].
#[derive(Clone, Debug, PartialEq)]
pub enum CursorKey {
    Moved,
    Activate,
    Close,
    Delete,
    Text(String),
    Ignored,
}

/// The keyboard cursor every panel re-implements: a section and a row within it, and whether it
/// shows. The same keys as the GUI bindings' (spec/components/lists.md); the pointer moves it
/// only when it reaches another cell.
#[derive(Clone, Debug)]
pub struct CursorModel {
    sections: Vec<CursorSection>,
    section: usize,
    index: usize,
    pub active: bool,
    pub columns: usize,
    pub page: usize,
    gate: Option<(u16, u16)>,
    initial_sample: bool,
}

impl CursorModel {
    pub fn new<N: Into<String>>(sections: impl IntoIterator<Item = (N, usize)>) -> CursorModel {
        let sections = sections.into_iter().map(|(n, c)| CursorSection { name: n.into(), count: c }).collect();
        let mut m = CursorModel { sections, section: 0, index: 0, active: false, columns: 1, page: 5, gate: None, initial_sample: false };
        m.settle();
        m
    }
    /// One unnamed section of `count` rows.
    pub fn list(count: usize) -> CursorModel {
        CursorModel::new([("", count)])
    }
    pub fn columns(mut self, n: usize) -> Self {
        self.columns = n.max(1);
        self
    }
    pub fn section(&self) -> &str {
        self.sections.get(self.section).map_or("", |s| s.name.as_str())
    }
    pub fn index(&self) -> usize {
        self.index
    }
    pub fn is_at(&self, section: &str, index: usize) -> bool {
        self.active && self.section() == section && self.index == index
    }
    pub fn cursor_in(&self, section: &str) -> Option<usize> {
        (self.active && self.section() == section).then_some(self.index)
    }
    pub fn set_count(&mut self, section: &str, count: usize) {
        if let Some(s) = self.sections.iter_mut().find(|s| s.name == section) {
            s.count = count;
        }
        self.settle();
    }
    pub fn set(&mut self, section: &str, index: usize) {
        if let Some(i) = self.sections.iter().position(|s| s.name == section) {
            self.section = i;
            self.index = index;
            self.active = true;
            self.settle();
        }
    }
    pub fn reset(&mut self) {
        self.active = false;
        self.gate = None;
        self.initial_sample = false;
    }
    pub fn move_by(&mut self, dx: i32, dy: i32) -> bool {
        if !self.wake() {
            return true;
        }
        let before = (self.section, self.index);
        if dx != 0 && self.columns > 1 {
            let col = self.index % self.columns;
            let to = col as i64 + dx as i64;
            if to >= 0 && (to as usize) < self.columns {
                let next = self.index as i64 + dx as i64;
                if next >= 0 && (next as usize) < self.count() {
                    self.index = next as usize;
                }
            }
        }
        if dy != 0 {
            let step = self.columns as i64;
            for _ in 0..dy.unsigned_abs() {
                let next = self.index as i64 + step * dy.signum() as i64;
                if next >= 0 && (next as usize) < self.count() {
                    self.index = next as usize;
                } else if dy > 0 {
                    if let Some(s) = self.neighbour(1) {
                        self.section = s;
                        self.index = (self.index % self.columns).min(self.count().saturating_sub(1));
                    } else if self.columns > 1 && self.index / self.columns < (self.count().saturating_sub(1)) / self.columns {
                        self.index = self.count() - 1;
                    }
                } else if let Some(s) = self.neighbour(-1) {
                    self.section = s;
                    let count = self.count();
                    let last_line = (count.saturating_sub(1) / self.columns) * self.columns;
                    self.index = (last_line + self.index % self.columns).min(count.saturating_sub(1));
                }
            }
        }
        (self.section, self.index) != before
    }
    pub fn move_section(&mut self, delta: i32) -> bool {
        if !self.wake() {
            return true;
        }
        match self.neighbour(delta.signum()) {
            Some(s) => {
                self.section = s;
                self.index = 0;
                true
            }
            None => false,
        }
    }
    pub fn page_by(&mut self, delta: i32) -> bool {
        if !self.wake() {
            return true;
        }
        let before = self.index;
        let last = self.count().saturating_sub(1) as i64;
        let step = (self.page.max(1) * self.columns) as i64;
        self.index = (self.index as i64 + step * delta.signum() as i64).clamp(0, last) as usize;
        self.index != before
    }
    pub fn home(&mut self) -> bool {
        self.active = true;
        let before = (self.section, self.index);
        if let Some(s) = self.sections.iter().position(|s| s.count > 0) {
            self.section = s;
            self.index = 0;
        }
        (self.section, self.index) != before
    }
    pub fn end(&mut self) -> bool {
        self.active = true;
        let before = (self.section, self.index);
        if let Some(s) = self.sections.iter().rposition(|s| s.count > 0) {
            self.section = s;
            self.index = self.sections[s].count - 1;
        }
        (self.section, self.index) != before
    }
    /// Down/j, Up/k, Right/l, Left/h, Page Up/Down, Home/End, Tab/Shift+Tab (sections), Return and
    /// Space (activate), Escape (close), Delete and x (delete), any other printable key (text).
    pub fn apply_key(&mut self, key: &KeyEvent) -> CursorKey {
        let plain = !key.modifiers.intersects(KeyModifiers::CONTROL | KeyModifiers::ALT | KeyModifiers::SUPER);
        match key.code {
            KeyCode::Down => self.moved(|c| c.move_by(0, 1)),
            KeyCode::Up => self.moved(|c| c.move_by(0, -1)),
            KeyCode::Right => self.moved(|c| c.move_by(1, 0)),
            KeyCode::Left => self.moved(|c| c.move_by(-1, 0)),
            KeyCode::PageDown => self.moved(|c| c.page_by(1)),
            KeyCode::PageUp => self.moved(|c| c.page_by(-1)),
            KeyCode::Home => self.moved(|c| c.home()),
            KeyCode::End => self.moved(|c| c.end()),
            KeyCode::Enter => CursorKey::Activate,
            KeyCode::Esc => CursorKey::Close,
            KeyCode::Delete => CursorKey::Delete,
            KeyCode::Tab => self.moved(|c| c.move_section(1)),
            KeyCode::BackTab => self.moved(|c| c.move_section(-1)),
            KeyCode::Char(ch) if plain => match ch {
                'j' => self.moved(|c| c.move_by(0, 1)),
                'k' => self.moved(|c| c.move_by(0, -1)),
                'l' => self.moved(|c| c.move_by(1, 0)),
                'h' => self.moved(|c| c.move_by(-1, 0)),
                ' ' => CursorKey::Activate,
                'x' | 'X' => CursorKey::Delete,
                _ => printable(key).map_or(CursorKey::Ignored, CursorKey::Text),
            },
            _ => CursorKey::Ignored,
        }
    }
    fn moved(&mut self, f: impl FnOnce(&mut Self) -> bool) -> CursorKey {
        f(self);
        CursorKey::Moved
    }
    /// A pointer sample over row `index` of `section` at a cell: moves the cursor only when the
    /// pointer reached another cell since the last accepted sample (a keyboard move resets the
    /// gate; the next sample only primes it).
    pub fn pointer_moved(&mut self, col: u16, row: u16, section: &str, index: usize) -> bool {
        let moved = match self.gate {
            Some(g) => g != (col, row),
            None => self.initial_sample,
        };
        if self.gate.is_none() || moved {
            self.gate = Some((col, row));
        }
        self.initial_sample = false;
        if moved {
            self.set(section, index);
        }
        moved
    }
    pub fn reset_pointer(&mut self) {
        self.gate = None;
        self.initial_sample = false;
    }
    pub fn allow_initial_sample(&mut self) {
        self.gate = None;
        self.initial_sample = true;
    }
    fn count(&self) -> usize {
        self.sections.get(self.section).map_or(0, |s| s.count)
    }
    fn wake(&mut self) -> bool {
        self.reset_pointer();
        if self.active {
            return true;
        }
        self.active = true;
        self.settle();
        false
    }
    fn neighbour(&self, dir: i32) -> Option<usize> {
        let mut i = self.section as i64 + dir as i64;
        while i >= 0 && (i as usize) < self.sections.len() {
            if self.sections[i as usize].count > 0 {
                return Some(i as usize);
            }
            i += dir as i64;
        }
        None
    }
    fn settle(&mut self) {
        if self.count() == 0
            && let Some(s) = self.neighbour(1).or_else(|| self.neighbour(-1))
        {
            self.section = s;
            self.index = 0;
        }
        self.index = self.index.min(self.count().saturating_sub(1));
    }
}

/// The text a key types (type-ahead filters), or None for a non-printable key or one with Ctrl,
/// Alt or Super.
pub fn printable(key: &KeyEvent) -> Option<String> {
    if key.modifiers.intersects(KeyModifiers::CONTROL | KeyModifiers::ALT | KeyModifiers::SUPER) {
        return None;
    }
    match key.code {
        KeyCode::Char(c) if !c.is_control() && !c.is_whitespace() => Some(c.to_string()),
        _ => None,
    }
}

// ---------------------------------------------------------------------------------------------
// ListRow

/// One row of a keyboard-and-mouse list, one line:
/// `[edge][pad][glyph ]Title  subtitle ··· value [trailing] [end][pad][edge]`.
/// The cursor row takes the hover fill and edges, the current row the selected fill.
pub struct ListRow {
    title: String,
    glyph: String,
    glyph_color: Option<Rgba>,
    leading: Option<Box<dyn Draw>>,
    subtitle: String,
    status: Option<(String, Tone)>,
    value: String,
    trailing: Option<Box<dyn Draw>>,
    trailing_glyph: Option<(String, Tone)>,
    action: Option<(String, bool)>,
    action_cursor: bool,
    expansion: Option<Box<dyn Draw>>,
    cursor: bool,
    current: bool,
    disabled: bool,
    width: u16,
    key: Option<String>,
}

impl ListRow {
    pub fn new(title: impl Into<String>) -> Self {
        ListRow {
            title: title.into(),
            glyph: String::new(),
            glyph_color: None,
            leading: None,
            subtitle: String::new(),
            status: None,
            value: String::new(),
            trailing: None,
            trailing_glyph: None,
            action: None,
            action_cursor: false,
            expansion: None,
            cursor: false,
            current: false,
            disabled: false,
            width: 53,
            key: None,
        }
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn glyph_color(mut self, c: Rgba) -> Self {
        self.glyph_color = Some(c);
        self
    }
    /// Any leading item instead of a glyph (a small Avatar: 2 columns).
    pub fn leading(mut self, d: impl Draw + 'static) -> Self {
        self.leading = Some(Box::new(d));
        self
    }
    pub fn subtitle(mut self, s: impl Into<String>) -> Self {
        self.subtitle = s.into();
        self
    }
    pub fn status(mut self, s: impl Into<String>, tone: Tone) -> Self {
        self.status = Some((s.into(), tone));
        self
    }
    pub fn value(mut self, v: impl Into<String>) -> Self {
        self.value = v.into();
        self
    }
    pub fn trailing(mut self, d: impl Draw + 'static) -> Self {
        self.trailing = Some(Box::new(d));
        self
    }
    pub fn trailing_glyph(mut self, g: impl Into<String>, tone: Tone) -> Self {
        self.trailing_glyph = Some((g.into(), tone));
        self
    }
    /// The action revealed in the end slot under the cursor (urgent: destructive).
    pub fn action(mut self, g: impl Into<String>, urgent: bool) -> Self {
        self.action = Some((g.into(), urgent));
        self
    }
    pub fn action_cursor(mut self, on: bool) -> Self {
        self.action_cursor = on;
        self
    }
    /// Opens the row: any item under the main line, inside the row's fill.
    pub fn expansion(mut self, d: impl Draw + 'static) -> Self {
        self.expansion = Some(Box::new(d));
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    pub fn current(mut self, on: bool) -> Self {
        self.current = on;
        self
    }
    pub fn disabled(mut self, on: bool) -> Self {
        self.disabled = on;
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    /// The row keyed `<k>`, its action `<k>-action`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for ListRow {
    fn size(&self, t: &Theme) -> (u16, u16) {
        (self.width, 1 + self.expansion.as_ref().map_or(0, |e| e.size(t).1))
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        let hot = self.cursor && !self.disabled;
        let (fill_c, border) = if hot {
            (c.hover.fill, super::border_of(&c.hover.border))
        } else if self.current && !self.disabled {
            (c.selected.fill, super::border_of(&c.selected.border))
        } else {
            (Rgba::TRANSPARENT, None)
        };
        fill(t, buf, area, fill_c);
        for dy in 0..area.height {
            edges(t, buf, area.x, area.y + dy, area.width, border);
        }
        let y = area.y;
        let faint = self.disabled.then(|| t.faint());
        let mut x = area.x + 2;
        if let Some(l) = &self.leading {
            let (lw, _) = l.size(t);
            l.draw(t, Rect::new(x, y, lw, 1), buf);
            x += lw + 1;
        } else if !self.glyph.is_empty() {
            put(t, buf, x, y, &self.glyph, faint.or(self.glyph_color).unwrap_or(t.fg()));
            x += 3;
        }
        // The end: [pad][edge] at the right, the end slot before them, then trailing, value.
        let mut right = area.right().saturating_sub(2);
        let show_action = self.action.is_some() && !self.disabled && (self.cursor || self.action_cursor);
        if self.trailing_glyph.is_some() || self.action.is_some() {
            right = right.saturating_sub(1);
            let ex = right;
            if show_action {
                let (g, urgent) = self.action.as_ref().unwrap();
                let ink = if *urgent { t.urgent() } else { t.dim() };
                if self.action_cursor {
                    fill(t, buf, Rect::new(ex, y, 1, 1), ink.with_alpha(c.hover.fill.alpha_f() as f64));
                }
                put(t, buf, ex, y, g, ink);
                if let Some(k) = &self.key {
                    t.hit(format!("{k}-action"), Rect::new(ex, y, 1, 1));
                }
            } else if let Some((g, tone)) = &self.trailing_glyph {
                put(t, buf, ex, y, g, faint.unwrap_or(tone.color(t, t.dim())));
            }
            right = right.saturating_sub(1);
        }
        if let Some(tr) = &self.trailing {
            let (tw, _) = tr.size(t);
            right = right.saturating_sub(tw);
            tr.draw(t, Rect::new(right, y, tw, 1), buf);
            right = right.saturating_sub(1);
        }
        if !self.value.is_empty() {
            let vw = width(&self.value);
            text_right(t, buf, right, y, &self.value, faint.unwrap_or(t.dim()));
            right = right.saturating_sub(vw + 1);
        }
        let avail = right.saturating_sub(x);
        let tw = text_fit(t, buf, x, y, &self.title, avail, Ink::new(faint.unwrap_or(t.fg())).bold(true));
        let second = if let Some((s, tone)) = &self.status {
            Some((s.as_str(), faint.unwrap_or(tone.color(t, t.dim()))))
        } else if !self.subtitle.is_empty() {
            Some((self.subtitle.as_str(), faint.unwrap_or(t.dim())))
        } else {
            None
        };
        if let Some((s, ink)) = second
            && avail > tw + 2
        {
            text_fit(t, buf, x + tw + 2, y, s, avail - tw - 2, ink);
        }
        if let Some(e) = &self.expansion {
            let (_, eh) = e.size(t);
            let inner = Rect::new(area.x + 2, y + 1, area.width.saturating_sub(4), eh);
            e.draw(t, inner, buf);
        }
        if let Some(k) = &self.key
            && !self.disabled
        {
            t.hit(k.clone(), Rect { height: 1, ..area });
        }
    }
}

// ---------------------------------------------------------------------------------------------
// ScrollArea

/// A vertical scroll view capped at `max_rows`, the bar over its last column while the content
/// overflows: a `│` groove in `track`, a `┃` thumb in the normal border color.
pub struct ScrollArea {
    children: Vec<Box<dyn Draw>>,
    max_rows: u16,
    offset: u16,
    gap: u16,
    width: u16,
}

impl ScrollArea {
    pub fn new(width: u16) -> Self {
        ScrollArea { children: Vec::new(), max_rows: u16::MAX, offset: 0, gap: 0, width }
    }
    pub fn max_rows(mut self, n: u16) -> Self {
        self.max_rows = n;
        self
    }
    /// The first content row shown.
    pub fn offset(mut self, rows: u16) -> Self {
        self.offset = rows;
        self
    }
    pub fn gap(mut self, g: u16) -> Self {
        self.gap = g;
        self
    }
    pub fn child(mut self, d: impl Draw + 'static) -> Self {
        self.children.push(Box::new(d));
        self
    }
    pub fn boxed(mut self, d: Box<dyn Draw>) -> Self {
        self.children.push(d);
        self
    }
    fn content_h(&self, t: &Theme) -> u16 {
        let n = self.children.len() as u16;
        self.children.iter().map(|c| c.size(t).1).sum::<u16>() + n.saturating_sub(1) * self.gap
    }
    /// The offset that shows rows `[top, top + h)` of the content in a `max` -row viewport, moving
    /// as little as possible from `offset`.
    pub fn ensure_visible(offset: u16, top: u16, h: u16, max: u16) -> u16 {
        if top < offset {
            top
        } else if top + h > offset + max {
            (top + h).saturating_sub(max)
        } else {
            offset
        }
    }
}

impl Draw for ScrollArea {
    fn size(&self, t: &Theme) -> (u16, u16) {
        (self.width, self.content_h(t).min(self.max_rows))
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let total = self.content_h(t);
        let view = area.height;
        let offset = self.offset.min(total.saturating_sub(view));
        // Draw the whole content into a scratch buffer, then copy the window.
        let scratch_area = Rect::new(area.x, 0, area.width, total.max(1));
        let mut scratch = Buffer::empty(scratch_area);
        // What's under the area shows through the scratch (fills composite over it).
        for y in 0..total.min(view + offset) {
            if y >= offset {
                for x in area.left()..area.right() {
                    if let (Some(src), Some(dst)) = (buf.cell((x, area.y + y - offset)).cloned(), scratch.cell_mut((x, y))) {
                        *dst = src;
                    }
                }
            }
        }
        let mut y = 0;
        let hits = t.hits_len();
        for c in &self.children {
            let (cw, ch) = c.size(t);
            c.draw(t, Rect::new(area.x, y, cw.min(area.width), ch).intersection(scratch_area), &mut scratch);
            y += ch + self.gap;
        }
        // Keys drawn in the scratch buffer land where they show, or nowhere when scrolled away.
        let shown = Rect::new(area.x, offset, area.width, view);
        t.remap_hits(hits, |r| {
            let r = r.intersection(shown);
            (r.height > 0).then(|| Rect { y: r.y - offset + area.y, ..r })
        });
        for dy in 0..view.min(total) {
            for x in area.left()..area.right() {
                if let (Some(src), Some(dst)) = (scratch.cell((x, offset + dy)).cloned(), buf.cell_mut((x, area.y + dy))) {
                    *dst = src;
                }
            }
        }
        if total > view && view > 0 {
            let bx = area.right() - 1;
            let thumb = ((view as u32 * view as u32 + total as u32 / 2) / total as u32).max(1) as u16;
            let room = view - thumb;
            let max_off = total - view;
            let ty = ((offset as u32 * room as u32 + max_off as u32 / 2) / max_off as u32) as u16;
            for dy in 0..view {
                let on = dy >= ty && dy < ty + thumb;
                let (sym, ink) = if on { ("┃", t.shown().controls.normal.border.color) } else { ("│", t.track()) };
                put(t, buf, bx, area.y + dy, sym, ink);
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// KeyValue, Hero

/// Dim labels beside their values, `columns` pairs to a line: label column, one space, value
/// column, three columns between pairs.
pub struct KeyValue {
    pairs: Vec<(String, String, Tone)>,
    columns: usize,
}

impl Default for KeyValue {
    fn default() -> Self {
        Self::new()
    }
}

impl KeyValue {
    pub fn new() -> Self {
        KeyValue { pairs: Vec::new(), columns: 1 }
    }
    pub fn pair(mut self, label: impl Into<String>, value: impl Into<String>) -> Self {
        self.pairs.push((label.into(), value.into(), Tone::Neutral));
        self
    }
    pub fn pair_toned(mut self, label: impl Into<String>, value: impl Into<String>, tone: Tone) -> Self {
        self.pairs.push((label.into(), value.into(), tone));
        self
    }
    pub fn columns(mut self, n: usize) -> Self {
        self.columns = n.max(1);
        self
    }
    fn widths(&self) -> Vec<(u16, u16)> {
        (0..self.columns)
            .map(|c| {
                let col = self.pairs.iter().skip(c).step_by(self.columns);
                let (mut l, mut v) = (0, 0);
                for (a, b, _) in col {
                    l = l.max(width(a));
                    v = v.max(width(b));
                }
                (l, v)
            })
            .collect()
    }
}

impl Draw for KeyValue {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let w: u16 = self.widths().iter().map(|(l, v)| l + 1 + v).sum::<u16>() + 3 * (self.columns as u16 - 1);
        (w, self.pairs.len().div_ceil(self.columns) as u16)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let ws = self.widths();
        for (i, (l, v, tone)) in self.pairs.iter().enumerate() {
            let (line, col) = (i / self.columns, i % self.columns);
            let x = area.x + ws[..col].iter().map(|(a, b)| a + 1 + b + 3).sum::<u16>();
            let y = area.y + line as u16;
            text(t, buf, x, y, l, ws[col].0, t.dim());
            text(t, buf, x + ws[col].0 + 1, y, v, ws[col].1, tone.color(t, t.fg()));
        }
    }
}

/// A panel's head: the glyph, then the bold title over an uppercased bold status line
/// (`textDim`, urgent on error), the trailing control at the end of the first row.
pub struct Hero {
    glyph: String,
    title: String,
    status: String,
    error: bool,
    glyph_opacity: f64,
    trailing: Option<Box<dyn Draw>>,
    width: u16,
}

impl Hero {
    pub fn new(glyph: impl Into<String>, title: impl Into<String>) -> Self {
        Hero { glyph: glyph.into(), title: title.into(), status: String::new(), error: false, glyph_opacity: 1.0, trailing: None, width: 53 }
    }
    pub fn status(mut self, s: impl Into<String>) -> Self {
        self.status = s.into();
        self
    }
    pub fn error(mut self, on: bool) -> Self {
        self.error = on;
        self
    }
    pub fn glyph_opacity(mut self, o: f64) -> Self {
        self.glyph_opacity = o;
        self
    }
    pub fn trailing(mut self, d: impl Draw + 'static) -> Self {
        self.trailing = Some(Box::new(d));
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
}

impl Draw for Hero {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, 2)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        put(t, buf, area.x, area.y, &self.glyph, t.fg().with_alpha(self.glyph_opacity));
        let tw = self.trailing.as_ref().map_or(0, |d| d.size(t).0 + 2);
        let words = area.width.saturating_sub(3 + tw);
        text_fit(t, buf, area.x + 3, area.y, &self.title, words, Ink::new(t.fg()).bold(true));
        let ink = if self.error { t.urgent() } else { t.dim() };
        text_fit(t, buf, area.x + 3, area.y + 1, &self.status.to_uppercase(), words, Ink::new(ink).bold(true));
        if let Some(d) = &self.trailing {
            let (w, h) = d.size(t);
            d.draw(t, Rect::new(area.right().saturating_sub(w), area.y, w, h), buf);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Badge, StatusDot

/// A count or short label: ` 3 ` bold. Neutral: the normal fill; toned: the tone at the selected
/// fill's alpha, text in the tone; filled: the tone solid, text in the background.
pub struct Badge {
    label: String,
    tone: Tone,
    filled: bool,
}

impl Badge {
    pub fn new(label: impl Into<String>) -> Self {
        Badge { label: label.into(), tone: Tone::Neutral, filled: false }
    }
    pub fn tone(mut self, t: Tone) -> Self {
        self.tone = t;
        self
    }
    pub fn filled(mut self, on: bool) -> Self {
        self.filled = on;
        self
    }
}

impl Draw for Badge {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (width(&self.label) + 2, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        let tone = self.tone.color(t, t.fg());
        let (bg, ink) = if self.filled {
            (tone, t.bg())
        } else if self.tone == Tone::Neutral {
            (c.normal.fill, t.fg())
        } else {
            (tone.with_alpha(c.selected.fill.alpha_f() as f64), tone)
        };
        fill(t, buf, area, bg);
        text(t, buf, area.x + 1, area.y, &self.label, area.width.saturating_sub(2), Ink::new(ink).bold(true));
    }
}

/// A dot `●` in its tone.
pub struct StatusDot(pub Tone);

impl Draw for StatusDot {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (1, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        put(t, buf, area.x, area.y, "●", self.0.color(t, t.fg()));
    }
}

// ---------------------------------------------------------------------------------------------
// Avatar

/// An avatar's size in cells: small 2×1 (a ListRow's leading slot), medium 4×2, large 6×3.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum AvatarSize {
    Small,
    #[default]
    Medium,
    Large,
}

impl AvatarSize {
    pub fn cells(self) -> (u16, u16) {
        match self {
            AvatarSize::Small => (2, 1),
            AvatarSize::Medium => (4, 2),
            AvatarSize::Large => (6, 3),
        }
    }
}

/// A decoded picture (8-bit straight RGBA), drawn in half-block pixels.
#[derive(Clone, Debug)]
pub struct AvatarImage {
    pub width: u32,
    pub height: u32,
    pub rgba: Vec<u8>,
}

impl AvatarImage {
    /// Decode a PNG (8-bit RGB or RGBA); None when it can't be read.
    pub fn load(path: impl AsRef<std::path::Path>) -> Option<AvatarImage> {
        AvatarImage::decode(&std::fs::read(path).ok()?)
    }

    /// Decode PNG bytes (8-bit RGB or RGBA, gray, gray-alpha).
    pub fn decode(bytes: &[u8]) -> Option<AvatarImage> {
        let mut dec = png::Decoder::new(std::io::Cursor::new(bytes));
        dec.set_transformations(png::Transformations::EXPAND | png::Transformations::STRIP_16);
        let mut reader = dec.read_info().ok()?;
        let mut data = vec![0; reader.output_buffer_size()?];
        let info = reader.next_frame(&mut data).ok()?;
        let (w, h) = (info.width, info.height);
        let rgba = match info.color_type {
            png::ColorType::Rgba => data[..(w * h * 4) as usize].to_vec(),
            png::ColorType::Rgb => data[..(w * h * 3) as usize].chunks(3).flat_map(|p| [p[0], p[1], p[2], 255]).collect(),
            png::ColorType::GrayscaleAlpha => data[..(w * h * 2) as usize].chunks(2).flat_map(|p| [p[0], p[0], p[0], p[1]]).collect(),
            png::ColorType::Grayscale => data[..(w * h) as usize].iter().flat_map(|&g| [g, g, g, 255]).collect(),
            _ => return None,
        };
        Some(AvatarImage { width: w, height: h, rgba })
    }

    /// The `w` × `h` box-filtered pixel (i, j): the source box `[floor(i·W/w), floor((i+1)·W/w))`
    /// (and the same down), alpha-weighted color `round(Σc·a / Σa)`, alpha `round(Σa / n)`.
    pub fn sample(&self, w: u32, h: u32, i: u32, j: u32) -> Rgba {
        let (x0, x1) = (i * self.width / w, ((i + 1) * self.width / w).max(i * self.width / w + 1));
        let (y0, y1) = (j * self.height / h, ((j + 1) * self.height / h).max(j * self.height / h + 1));
        let (mut r, mut g, mut b, mut a, mut n) = (0u64, 0u64, 0u64, 0u64, 0u64);
        for y in y0..y1.min(self.height) {
            for x in x0..x1.min(self.width) {
                let p = ((y * self.width + x) * 4) as usize;
                let pa = self.rgba[p + 3] as u64;
                r += self.rgba[p] as u64 * pa;
                g += self.rgba[p + 1] as u64 * pa;
                b += self.rgba[p + 2] as u64 * pa;
                a += pa;
                n += 1;
            }
        }
        if a == 0 || n == 0 {
            return Rgba::TRANSPARENT;
        }
        let ch = |s: u64| ((s * 2 + a) / (2 * a)) as u8;
        Rgba { r: ch(r), g: ch(g), b: ch(b), a: ((a * 2 + n) / (2 * n)) as u8 }
    }
}

/// A person's or an app's picture with fallbacks: the image (in half-block pixels, two per
/// cell), a glyph, the initial. Icons by freedesktop name don't exist in a terminal: an Avatar
/// with only `.icon(name)` draws its glyph or its initial (spec/terminal.md, Icons).
pub struct Avatar {
    name: String,
    image: Option<AvatarImage>,
    glyph: String,
    size: AvatarSize,
    round: bool,
    status: Option<Tone>,
}

impl Avatar {
    pub fn new(name: impl Into<String>) -> Self {
        Avatar { name: name.into(), image: None, glyph: String::new(), size: AvatarSize::Medium, round: false, status: None }
    }
    pub fn image(mut self, img: Option<AvatarImage>) -> Self {
        self.image = img;
        self
    }
    /// A freedesktop icon name: accepted for API parity, not drawn (no icons in a terminal).
    pub fn icon(self, _name: &str) -> Self {
        self
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn size(mut self, s: AvatarSize) -> Self {
        self.size = s;
        self
    }
    pub fn round(mut self, on: bool) -> Self {
        self.round = on;
        self
    }
    pub fn status(mut self, tone: Tone) -> Self {
        self.status = Some(tone);
        self
    }
    /// The first letter of `name`, upper-cased.
    pub fn initial(name: &str) -> String {
        name.chars().find(|c| c.is_alphanumeric()).map(|c| c.to_uppercase().collect()).unwrap_or_default()
    }
}

impl Draw for Avatar {
    fn size(&self, _: &Theme) -> (u16, u16) {
        self.size.cells()
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let (w, h) = self.size.cells();
        let area = Rect { width: w.min(area.width), height: h.min(area.height), ..area };
        if let Some(img) = &self.image {
            let (pw, ph) = (w as u32, h as u32 * 2);
            for j in 0..h {
                for i in 0..w {
                    let px = |pj: u32| -> Rgba {
                        if self.round {
                            // Outside the inscribed circle (pixel centers, in pixel units): clear.
                            let (cx, cy) = (i as f64 + 0.5 - pw as f64 / 2.0, pj as f64 + 0.5 - ph as f64 / 2.0);
                            let r = pw.min(ph) as f64 / 2.0;
                            if cx * cx + cy * cy > r * r {
                                return Rgba::TRANSPARENT;
                            }
                        }
                        img.sample(pw, ph, i as u32, pj)
                    };
                    let (top, bottom) = (px(j as u32 * 2), px(j as u32 * 2 + 1));
                    let (x, y) = (area.x + i, area.y + j);
                    fill(t, buf, Rect::new(x, y, 1, 1), bottom);
                    put(t, buf, x, y, "▀", crate::over(top, t.bg()));
                }
            }
        } else {
            fill(t, buf, area, t.shown().controls.selected.fill);
            let (mark, bold) = if !self.glyph.is_empty() { (self.glyph.clone(), false) } else { (Avatar::initial(&self.name), true) };
            let (cx, cy) = (area.x + (w - 1) / 2, area.y + (h - 1) / 2);
            put(t, buf, cx, cy, &mark, Ink::new(t.fg()).bold(bold));
        }
        if let Some(tone) = self.status {
            put(t, buf, area.right() - 1, area.bottom() - 1, "●", tone.color(t, t.fg()));
        }
    }
}

// ---------------------------------------------------------------------------------------------
// TileGrid

/// One tile: a glyph or an Avatar over a label of up to two lines.
pub struct Tile {
    label: String,
    glyph: String,
    avatar: Option<Avatar>,
}

impl Tile {
    pub fn new(label: impl Into<String>) -> Self {
        Tile { label: label.into(), glyph: String::new(), avatar: None }
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn avatar(mut self, a: Avatar) -> Self {
        self.avatar = Some(a.size(AvatarSize::Medium));
        self
    }
}

/// Tiles 11 columns × 4 rows, no gap: an icon box of two rows (the glyph on its second row, or a
/// medium Avatar filling it), then two label rows, centered, wrapped and elided. The cursor tile
/// takes the hover fill and edges, the current one the selected fill.
pub struct TileGrid {
    tiles: Vec<Tile>,
    columns: usize,
    cursor: Option<usize>,
    current: Option<usize>,
    key: Option<String>,
}

impl TileGrid {
    pub fn new(tiles: impl IntoIterator<Item = Tile>) -> Self {
        TileGrid { tiles: tiles.into_iter().collect(), columns: 4, cursor: None, current: None, key: None }
    }
    pub fn columns(mut self, n: usize) -> Self {
        self.columns = n.max(1);
        self
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    pub fn current(mut self, i: Option<usize>) -> Self {
        self.current = i;
        self
    }
    /// Tiles keyed `<k>-<i>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    pub const TILE: (u16, u16) = (11, 4);
}

impl Draw for TileGrid {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let cols = self.columns.min(self.tiles.len().max(1)) as u16;
        (cols * Self::TILE.0, self.tiles.len().div_ceil(self.columns) as u16 * Self::TILE.1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let (tw, th) = Self::TILE;
        for (i, tile) in self.tiles.iter().enumerate() {
            let (c, r) = ((i % self.columns) as u16, (i / self.columns) as u16);
            let cell = Rect::new(area.x + c * tw, area.y + r * th, tw, th).intersection(area);
            if cell.height < th {
                continue;
            }
            let l = look(t, self.cursor == Some(i), self.current == Some(i), false, false);
            fill(t, buf, cell, l.fill);
            for dy in 0..th {
                edges(t, buf, cell.x, cell.y + dy, tw, l.border);
            }
            if let Some(a) = &tile.avatar {
                let (aw, ah) = a.size(t);
                a.draw(t, Rect::new(cell.x + (tw - aw) / 2, cell.y, aw, ah), buf);
            } else {
                text_center(t, buf, cell.x + 1, cell.y + 1, tw - 2, &tile.glyph, t.fg());
            }
            let lines = super::wrap(&tile.label, tw - 2);
            for (n, line) in lines.iter().take(2).enumerate() {
                let line = if n == 1 && lines.len() > 2 { paint::fit(&format!("{line} {}", lines[2..].join(" ")), tw - 2) } else { line.clone() };
                text_center(t, buf, cell.x + 1, cell.y + 2 + n as u16, tw - 2, &line, Ink::new(t.fg()).bold(l.bold));
            }
            if let Some(k) = &self.key {
                t.hit(format!("{k}-{i}"), cell);
            }
        }
    }
}
