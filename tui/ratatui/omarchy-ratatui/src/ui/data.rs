//! Tables, trees, split panes, sidebars and the month grid (spec/terminal.md, Data). Rows share one
//! ladder: the hover fill on the cursor row, else the selected fill (bold, selected text) on the
//! selected one, else nothing.

use crossterm::event::{KeyCode, KeyEvent};
use omarchy_theme::Rgba;
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::glyphs;
use crate::paint::{Ink, edges, fill, hline, put, text, text_center, text_fit, vline, width};
use crate::{Draw, Theme};

/// The row fill and label ink for a row that may be the cursor and may be selected.
fn row_look(t: &Theme, cursor: bool, selected: bool) -> (Rgba, Ink) {
    let c = &t.shown().controls;
    let fill_c = if cursor {
        c.hover.fill
    } else if selected {
        c.selected.fill
    } else {
        Rgba::TRANSPARENT
    };
    let ink = if selected { Ink::new(c.selected.text).bold(true) } else { Ink::new(t.fg()) };
    (fill_c, ink)
}

// ---------------------------------------------------------------------------------------------
// Table

/// A column: a title, a fixed width in cells (else it shares the slack), right-aligned or not.
#[derive(Clone, Debug)]
pub struct Column {
    title: String,
    width: Option<u16>,
    right: bool,
}

impl Column {
    pub fn new(title: impl Into<String>) -> Self {
        Column { title: title.into(), width: None, right: false }
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = Some(w);
        self
    }
    pub fn right(mut self) -> Self {
        self.right = true;
        self
    }
}

/// A header of bold `textDim` titles (the sorted one with `󰅃`/`󰅀`), a `track` rule under it, then
/// one row per item, cells padded a column each side and elided with "…". (Rules between body
/// rows would double the table's height in cells: the terminal has none.)
pub struct Table {
    columns: Vec<Column>,
    rows: Vec<Vec<String>>,
    sort: Option<(usize, bool)>,
    selected: Option<usize>,
    cursor: Option<usize>,
    width: u16,
    key: Option<String>,
}

impl Table {
    pub fn new(columns: impl IntoIterator<Item = Column>) -> Self {
        Table { columns: columns.into_iter().collect(), rows: Vec::new(), sort: None, selected: None, cursor: None, width: 60, key: None }
    }
    pub fn rows(mut self, rows: impl IntoIterator<Item = impl IntoIterator<Item = impl Into<String>>>) -> Self {
        self.rows = rows.into_iter().map(|r| r.into_iter().map(Into::into).collect()).collect();
        self
    }
    pub fn sort(mut self, column: usize, ascending: bool) -> Self {
        self.sort = Some((column, ascending));
        self
    }
    pub fn selected(mut self, r: Option<usize>) -> Self {
        self.selected = r;
        self
    }
    pub fn cursor(mut self, r: Option<usize>) -> Self {
        self.cursor = r;
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    /// Rows keyed `<k>-<row>`, headers `<k>-col-<column>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    /// Column widths: fixed ones as given, the rest share `floor((width − fixed) / flex)`.
    pub fn widths(&self, total: u16) -> Vec<u16> {
        let fixed: u16 = self.columns.iter().filter_map(|c| c.width).sum();
        let flex = self.columns.iter().filter(|c| c.width.is_none()).count() as u16;
        let share = total.saturating_sub(fixed).checked_div(flex).unwrap_or(0);
        self.columns.iter().map(|c| c.width.unwrap_or(share)).collect()
    }
    /// Up/Down/k/j, Home/End, Page Up/Down move `cursor` among `count` rows; true when handled.
    pub fn handle_key(cursor: &mut usize, count: usize, key: &KeyEvent) -> bool {
        let last = count.saturating_sub(1);
        match key.code {
            KeyCode::Down | KeyCode::Char('j') => *cursor = (*cursor + 1).min(last),
            KeyCode::Up | KeyCode::Char('k') => *cursor = cursor.saturating_sub(1),
            KeyCode::Home => *cursor = 0,
            KeyCode::End => *cursor = last,
            KeyCode::PageDown => *cursor = (*cursor + 5).min(last),
            KeyCode::PageUp => *cursor = cursor.saturating_sub(5),
            _ => return false,
        }
        true
    }
}

impl Draw for Table {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, 2 + self.rows.len() as u16)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let ws = self.widths(area.width);
        let mut x = area.x;
        for (i, (c, w)) in self.columns.iter().zip(&ws).enumerate() {
            let inner = w.saturating_sub(2);
            let ink = Ink::new(t.dim()).bold(true);
            let chevron = self.sort.filter(|(col, _)| *col == i).map(|(_, asc)| if asc { glyphs::CHEVRON_UP } else { glyphs::CHEVRON_DOWN });
            let tw = width(&c.title).min(inner.saturating_sub(if chevron.is_some() { 2 } else { 0 }));
            if c.right {
                let start = x + 1 + inner - tw;
                text_fit(t, buf, start, area.y, &c.title, tw, ink);
                if let Some(ch) = chevron {
                    put(t, buf, start.saturating_sub(2), area.y, ch, t.dim());
                }
            } else {
                text_fit(t, buf, x + 1, area.y, &c.title, tw, ink);
                if let Some(ch) = chevron {
                    put(t, buf, x + 1 + tw + 1, area.y, ch, t.dim());
                }
            }
            if let Some(k) = &self.key {
                t.hit(format!("{k}-col-{i}"), Rect::new(x, area.y, *w, 1));
            }
            x += w;
        }
        hline(t, buf, area.x, area.y + 1, area.width, "─", t.track());
        for (r, row) in self.rows.iter().enumerate() {
            let y = area.y + 2 + r as u16;
            if y >= area.bottom() {
                break;
            }
            let (f, ink) = row_look(t, self.cursor == Some(r), self.selected == Some(r));
            fill(t, buf, Rect::new(area.x, y, area.width, 1), f);
            let mut x = area.x;
            for (i, (c, w)) in self.columns.iter().zip(&ws).enumerate() {
                let s = row.get(i).map(String::as_str).unwrap_or("");
                let inner = w.saturating_sub(2);
                if c.right {
                    let s = crate::paint::fit(s, inner);
                    text(t, buf, x + 1 + inner - width(&s), y, &s, inner, ink);
                } else {
                    text_fit(t, buf, x + 1, y, s, inner, ink);
                }
                x += w;
            }
            if let Some(k) = &self.key {
                t.hit(format!("{k}-{r}"), Rect::new(area.x, y, area.width, 1));
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Tree

/// A node: id, label, optional glyph, children.
#[derive(Clone, Debug)]
pub struct TreeNode {
    pub id: String,
    pub label: String,
    pub glyph: String,
    pub children: Vec<TreeNode>,
}

impl TreeNode {
    pub fn new(id: impl Into<String>, label: impl Into<String>) -> Self {
        TreeNode { id: id.into(), label: label.into(), glyph: String::new(), children: Vec::new() }
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn children(mut self, c: impl IntoIterator<Item = TreeNode>) -> Self {
        self.children = c.into_iter().collect();
        self
    }
}

/// FileTree-style rows: an inset of `1 + 2 × depth`, the chevron (`󰅂` closed, `󰅀` open,
/// `textDim`) and a space, the glyph (`textDim`; selected text when selected) and a space, the
/// label.
pub struct Tree {
    nodes: Vec<TreeNode>,
    expanded: Vec<String>,
    selected: Option<String>,
    cursor: Option<String>,
    width: u16,
    key: Option<String>,
}

impl Tree {
    pub fn new(nodes: impl IntoIterator<Item = TreeNode>) -> Self {
        Tree { nodes: nodes.into_iter().collect(), expanded: Vec::new(), selected: None, cursor: None, width: 28, key: None }
    }
    pub fn expanded(mut self, ids: impl IntoIterator<Item = impl Into<String>>) -> Self {
        self.expanded = ids.into_iter().map(Into::into).collect();
        self
    }
    pub fn selected(mut self, id: Option<&str>) -> Self {
        self.selected = id.map(Into::into);
        self
    }
    pub fn cursor(mut self, id: Option<&str>) -> Self {
        self.cursor = id.map(Into::into);
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    /// Rows keyed `<k>-<id>`, chevrons `<k>-<id>-toggle`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    /// The visible rows, depth first: (depth, node).
    pub fn visible<'a>(nodes: &'a [TreeNode], expanded: &[String]) -> Vec<(u16, &'a TreeNode)> {
        fn walk<'a>(ns: &'a [TreeNode], depth: u16, expanded: &[String], out: &mut Vec<(u16, &'a TreeNode)>) {
            for n in ns {
                out.push((depth, n));
                if !n.children.is_empty() && expanded.contains(&n.id) {
                    walk(&n.children, depth + 1, expanded, out);
                }
            }
        }
        let mut out = Vec::new();
        walk(nodes, 0, expanded, &mut out);
        out
    }
}

impl Draw for Tree {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, Tree::visible(&self.nodes, &self.expanded).len() as u16)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        for (i, (depth, n)) in Tree::visible(&self.nodes, &self.expanded).into_iter().enumerate() {
            let y = area.y + i as u16;
            if y >= area.bottom() {
                break;
            }
            let selected = self.selected.as_deref() == Some(n.id.as_str());
            let (f, ink) = row_look(t, self.cursor.as_deref() == Some(n.id.as_str()), selected);
            fill(t, buf, Rect::new(area.x, y, area.width, 1), f);
            let mut x = area.x + 1 + 2 * depth;
            if !n.children.is_empty() {
                let open = self.expanded.contains(&n.id);
                put(t, buf, x, y, if open { glyphs::CHEVRON_DOWN } else { glyphs::CHEVRON_RIGHT }, t.dim());
                if let Some(k) = &self.key {
                    t.hit(format!("{k}-{}", n.id), Rect::new(area.x, y, area.width, 1));
                    t.hit(format!("{k}-{}-toggle", n.id), Rect::new(x, y, 1, 1));
                }
            } else if let Some(k) = &self.key {
                t.hit(format!("{k}-{}", n.id), Rect::new(area.x, y, area.width, 1));
            }
            x += 2;
            if !n.glyph.is_empty() {
                put(t, buf, x, y, &n.glyph, if selected { c.selected.text } else { t.dim() });
                x += 2;
            }
            text_fit(t, buf, x, y, &n.label, area.right().saturating_sub(x + 1), ink);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// SplitView

/// A pane of a SplitView: content, and a size in cells (None: it fills).
pub struct SplitPane {
    content: Box<dyn Draw>,
    size: Option<u16>,
    min: u16,
    visible: bool,
}

impl SplitPane {
    pub fn new(content: impl Draw + 'static) -> Self {
        SplitPane { content: Box::new(content), size: None, min: 0, visible: true }
    }
    pub fn size(mut self, s: u16) -> Self {
        self.size = Some(s);
        self
    }
    pub fn min(mut self, m: u16) -> Self {
        self.min = m;
        self
    }
    pub fn visible(mut self, on: bool) -> Self {
        self.visible = on;
        self
    }
}

/// Panes side by side with a one-column handle between each pair: `│` in `track`, in the hover
/// fill under the pointer, while dragged or with the cursor.
pub struct SplitView {
    panes: Vec<SplitPane>,
    cursor: Option<usize>,
    width: u16,
    height: u16,
}

impl SplitView {
    pub fn new(width: u16, height: u16) -> Self {
        SplitView { panes: Vec::new(), cursor: None, width, height }
    }
    pub fn pane(mut self, p: SplitPane) -> Self {
        self.panes.push(p);
        self
    }
    /// The handle (0-based) drawn hot.
    pub fn cursor(mut self, handle: Option<usize>) -> Self {
        self.cursor = handle;
        self
    }
}

impl Draw for SplitView {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, self.height)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let panes: Vec<&SplitPane> = self.panes.iter().filter(|p| p.visible).collect();
        let handles = panes.len().saturating_sub(1) as u16;
        let fixed: u16 = panes.iter().filter_map(|p| p.size.map(|s| s.max(p.min))).sum();
        let flex = panes.iter().filter(|p| p.size.is_none()).count() as u16;
        let share = area.width.saturating_sub(fixed + handles).checked_div(flex).unwrap_or(0);
        let mut x = area.x;
        for (i, p) in panes.iter().enumerate() {
            let w = p.size.map(|s| s.max(p.min)).unwrap_or(share);
            p.content.draw(t, Rect::new(x, area.y, w, area.height).intersection(area), buf);
            x += w;
            if i + 1 < panes.len() {
                let hr = Rect::new(x, area.y, 1, area.height);
                if self.cursor == Some(i) {
                    fill(t, buf, hr, t.shown().controls.hover.fill);
                }
                vline(t, buf, x, area.y, area.height, "│", t.track());
                t.hit(format!("split-handle-{i}"), hr);
                x += 1;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Sidebar

#[derive(Clone, Debug)]
pub struct SidebarItem {
    pub id: String,
    pub label: String,
    pub glyph: String,
}

impl SidebarItem {
    pub fn new(id: impl Into<String>, label: impl Into<String>, glyph: impl Into<String>) -> Self {
        SidebarItem { id: id.into(), label: label.into(), glyph: glyph.into() }
    }
}

#[derive(Clone, Debug)]
pub struct SidebarSection {
    pub title: String,
    pub items: Vec<SidebarItem>,
}

impl SidebarSection {
    pub fn new(title: impl Into<String>, items: impl IntoIterator<Item = SidebarItem>) -> Self {
        SidebarSection { title: title.into(), items: items.into_iter().collect() }
    }
}

/// Sections a row apart: a SectionHeader inset a column, then rows of ` G  Label` (glyph `textDim`,
/// selected text when selected). Collapsed: the glyphs alone, a 5-column rail.
pub struct Sidebar {
    sections: Vec<SidebarSection>,
    selected: Option<String>,
    cursor: Option<String>,
    collapsed: bool,
    width: u16,
    key: Option<String>,
}

impl Sidebar {
    pub fn new(sections: impl IntoIterator<Item = SidebarSection>) -> Self {
        Sidebar { sections: sections.into_iter().collect(), selected: None, cursor: None, collapsed: false, width: 25, key: None }
    }
    pub fn selected(mut self, id: Option<&str>) -> Self {
        self.selected = id.map(Into::into);
        self
    }
    pub fn cursor(mut self, id: Option<&str>) -> Self {
        self.cursor = id.map(Into::into);
        self
    }
    pub fn collapsed(mut self, on: bool) -> Self {
        self.collapsed = on;
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    /// Rows keyed `<k>-<id>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    pub const RAIL: u16 = 5;
}

impl Draw for Sidebar {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let n = self.sections.len() as u16;
        let rows: u16 = self.sections.iter().map(|s| s.items.len() as u16).sum();
        let headers = if self.collapsed { 0 } else { n };
        (if self.collapsed { Self::RAIL } else { self.width }, rows + headers + n.saturating_sub(1))
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        let mut y = area.y;
        for (si, s) in self.sections.iter().enumerate() {
            if si > 0 {
                y += 1;
            }
            if !self.collapsed {
                text_fit(t, buf, area.x + 1, y, &s.title, area.width.saturating_sub(2), Ink::new(t.dim()).bold(true));
                y += 1;
            }
            for it in &s.items {
                if y >= area.bottom() {
                    return;
                }
                let selected = self.selected.as_deref() == Some(it.id.as_str());
                let (f, ink) = row_look(t, self.cursor.as_deref() == Some(it.id.as_str()), selected);
                fill(t, buf, Rect::new(area.x, y, area.width, 1), f);
                put(t, buf, area.x + if self.collapsed { 2 } else { 1 }, y, &it.glyph, if selected { c.selected.text } else { t.dim() });
                if !self.collapsed {
                    text_fit(t, buf, area.x + 4, y, &it.label, area.width.saturating_sub(5), ink);
                }
                if let Some(k) = &self.key {
                    t.hit(format!("{k}-{}", it.id), Rect::new(area.x, y, area.width, 1));
                }
                y += 1;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Calendar

/// A calendar date.
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord)]
pub struct Date {
    pub year: i32,
    pub month: u32,
    pub day: u32,
}

impl Date {
    pub fn new(year: i32, month: u32, day: u32) -> Self {
        Date { year, month, day }
    }
    pub fn days_in_month(year: i32, month: u32) -> u32 {
        match month {
            1 | 3 | 5 | 7 | 8 | 10 | 12 => 31,
            4 | 6 | 9 | 11 => 30,
            _ if (year % 4 == 0 && year % 100 != 0) || year % 400 == 0 => 29,
            _ => 28,
        }
    }
    /// Days since 1970-01-01.
    fn ordinal(self) -> i64 {
        // Howard Hinnant's days_from_civil.
        let (y, m, d) = (self.year as i64 - if self.month <= 2 { 1 } else { 0 }, self.month as i64, self.day as i64);
        let era = if y >= 0 { y } else { y - 399 } / 400;
        let yoe = y - era * 400;
        let doy = (153 * (m + if m > 2 { -3 } else { 9 }) + 2) / 5 + d - 1;
        let doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        era * 146097 + doe - 719468
    }
    fn from_ordinal(z: i64) -> Date {
        let z = z + 719468;
        let era = if z >= 0 { z } else { z - 146096 } / 146097;
        let doe = z - era * 146097;
        let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
        let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
        let mp = (5 * doy + 2) / 153;
        let d = (doy - (153 * mp + 2) / 5 + 1) as u32;
        let m = if mp < 10 { mp + 3 } else { mp - 9 } as u32;
        let y = (yoe + era * 400 + if m <= 2 { 1 } else { 0 }) as i32;
        Date::new(y, m, d)
    }
    /// 0 = Monday … 6 = Sunday.
    pub fn weekday(self) -> u32 {
        ((self.ordinal() + 3).rem_euclid(7)) as u32
    }
    pub fn add_days(self, n: i64) -> Date {
        Date::from_ordinal(self.ordinal() + n)
    }
    pub fn add_months(self, n: i32) -> Date {
        let m0 = self.year * 12 + self.month as i32 - 1 + n;
        let (y, m) = (m0.div_euclid(12), m0.rem_euclid(12) as u32 + 1);
        Date::new(y, m, self.day.min(Date::days_in_month(y, m)))
    }
    pub fn iso(self) -> String {
        format!("{:04}-{:02}-{:02}", self.year, self.month, self.day)
    }
}

const MONTHS: [&str; 12] = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];
const WEEKDAYS: [&str; 7] = ["MO", "TU", "WE", "TH", "FR", "SA", "SU"];

/// A month from Monday: a header row (`󰅁` at the start, `MONTH YEAR` in `textDim` centered, `󰅂`
/// at the end), the weekday row (`MO … SU` bold `textDim`), six week rows of 4-column day cells
/// (`▕dd▏`, number right-aligned in the middle two): today's edges in accent and bold, the selected
/// day the selected fill (bold, selected text), the cursor day the hover fill; weekends `textDim`,
/// days outside the month `textFaint`. 28 columns × 8 rows.
pub struct Calendar {
    year: i32,
    month: u32,
    today: Option<Date>,
    selected: Option<Date>,
    cursor: Option<Date>,
    key: Option<String>,
}

impl Calendar {
    pub fn new(year: i32, month: u32) -> Self {
        Calendar { year, month, today: None, selected: None, cursor: None, key: None }
    }
    pub fn today(mut self, d: Date) -> Self {
        self.today = Some(d);
        self
    }
    pub fn selected(mut self, d: Option<Date>) -> Self {
        self.selected = d;
        self
    }
    pub fn cursor(mut self, d: Option<Date>) -> Self {
        self.cursor = d;
        self
    }
    /// Days keyed `<k>-YYYY-MM-DD`, the steppers `<k>-prev` and `<k>-next`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    /// The first cell: the Monday on or before the 1st.
    pub fn first_cell(year: i32, month: u32) -> Date {
        let first = Date::new(year, month, 1);
        first.add_days(-(first.weekday() as i64))
    }
    /// Arrows (and h/j/k/l) move `cursor` by a day or a week, Page Up/Down by a month; the shown
    /// month follows the cursor. True when handled.
    pub fn handle_key(cursor: &mut Date, year: &mut i32, month: &mut u32, key: &KeyEvent) -> bool {
        *cursor = match key.code {
            KeyCode::Left | KeyCode::Char('h') => cursor.add_days(-1),
            KeyCode::Right | KeyCode::Char('l') => cursor.add_days(1),
            KeyCode::Up | KeyCode::Char('k') => cursor.add_days(-7),
            KeyCode::Down | KeyCode::Char('j') => cursor.add_days(7),
            KeyCode::PageUp => cursor.add_months(-1),
            KeyCode::PageDown => cursor.add_months(1),
            _ => return false,
        };
        *year = cursor.year;
        *month = cursor.month;
        true
    }
}

impl Draw for Calendar {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (28, 8)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        put(t, buf, area.x + 1, area.y, glyphs::CHEVRON_LEFT, t.fg());
        put(t, buf, area.x + 26, area.y, glyphs::CHEVRON_RIGHT, t.fg());
        let title = format!("{} {}", MONTHS[(self.month as usize).clamp(1, 12) - 1].to_uppercase(), self.year);
        text_center(t, buf, area.x + 3, area.y, 22, &title, t.dim());
        if let Some(k) = &self.key {
            t.hit(format!("{k}-prev"), Rect::new(area.x, area.y, 3, 1));
            t.hit(format!("{k}-next"), Rect::new(area.x + 25, area.y, 3, 1));
        }
        for (i, w) in WEEKDAYS.iter().enumerate() {
            text(t, buf, area.x + 4 * i as u16 + 1, area.y + 1, w, 2, Ink::new(t.dim()).bold(true));
        }
        let start = Calendar::first_cell(self.year, self.month);
        for n in 0..42u16 {
            let d = start.add_days(n as i64);
            let (col, row) = (n % 7, n / 7);
            let (x, y) = (area.x + col * 4, area.y + 2 + row);
            if y >= area.bottom() {
                break;
            }
            let cell = Rect::new(x, y, 4, 1);
            let selected = self.selected == Some(d);
            let today = self.today == Some(d);
            if self.cursor == Some(d) {
                fill(t, buf, cell, c.hover.fill);
            } else if selected {
                fill(t, buf, cell, c.selected.fill);
            }
            if today {
                edges(t, buf, x, y, 4, Some(t.accent()));
            }
            let in_month = d.month == self.month;
            let ink = if selected {
                Ink::new(c.selected.text).bold(true)
            } else if !in_month {
                Ink::new(t.faint())
            } else if col >= 5 {
                Ink::new(t.dim()).bold(today)
            } else {
                Ink::new(t.fg()).bold(today)
            };
            text(t, buf, x + 1, y, &format!("{:>2}", d.day), 2, ink);
            if let Some(k) = &self.key {
                t.hit(format!("{k}-{}", d.iso()), cell);
            }
        }
    }
}
