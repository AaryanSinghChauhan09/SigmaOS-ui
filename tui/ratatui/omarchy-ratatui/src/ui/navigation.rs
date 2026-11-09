//! Menus, keys and the chrome apps move around with (spec/terminal.md, Navigation).

use crossterm::event::{KeyCode, KeyEvent};
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::controls::control;
use super::{Look, glyphs};
use crate::paint::{Ink, clear, fill, frame, hline, put, text, text_fit, text_right, vline, width};
use crate::{Draw, Theme};

// ---------------------------------------------------------------------------------------------
// Keycap, KeyCombo

/// How a key name is drawn on a cap (the same table in every binding).
pub fn key_label(key: &str) -> String {
    let k = key.trim();
    let lower = k.to_lowercase();
    let named = match lower.as_str() {
        "super" | "meta" | "win" | "cmd" | "logo" => "Super",
        "ctrl" | "control" => "Ctrl",
        "alt" | "option" => "Alt",
        "shift" => "⇧",
        "enter" | "return" => "⏎",
        "backspace" => "⌫",
        "esc" | "escape" => "Esc",
        "delete" | "del" => "Del",
        "up" => "↑",
        "down" => "↓",
        "left" => "←",
        "right" => "→",
        "pageup" => "PgUp",
        "pagedown" => "PgDn",
        "tab" => "Tab",
        "space" => "Space",
        "home" => "Home",
        "end" => "End",
        _ => "",
    };
    if !named.is_empty() {
        return named.into();
    }
    if k.chars().count() == 1 {
        return k.to_uppercase();
    }
    k.to_string()
}

/// "Ctrl+Shift+S" as its keys; a lone `+` is the plus key ("Ctrl++").
pub fn split_combo(combo: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut cur = String::new();
    let chars: Vec<char> = combo.chars().collect();
    let mut i = 0;
    while i < chars.len() {
        let c = chars[i];
        if c == '+' && !cur.is_empty() {
            out.push(std::mem::take(&mut cur));
        } else if c == '+' {
            cur.push('+');
            out.push(std::mem::take(&mut cur));
            if i + 1 < chars.len() && chars[i + 1] == '+' {
                i += 1;
            }
        } else {
            cur.push(c);
        }
        i += 1;
    }
    if !cur.is_empty() {
        out.push(cur);
    }
    out
}

/// One key: ` Label ` in the normal fill between the normal border's edges.
pub struct Keycap(pub String);

impl Keycap {
    pub fn new(key: impl Into<String>) -> Self {
        Keycap(key.into())
    }
}

impl Draw for Keycap {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (width(&key_label(&self.0)) + 4, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        control(t, buf, area.x, area.y, area.width, &Look { fill: c.normal.fill, border: Some(c.normal.border.color), text: t.fg(), bold: false });
        text(t, buf, area.x + 2, area.y, &key_label(&self.0), area.width.saturating_sub(4), t.fg());
    }
}

/// A combination, "Ctrl+Shift+P", drawn as Keycaps side by side.
pub struct KeyCombo(pub String);

impl KeyCombo {
    pub fn new(combo: impl Into<String>) -> Self {
        KeyCombo(combo.into())
    }
    fn caps(&self) -> Vec<Keycap> {
        split_combo(&self.0).into_iter().map(Keycap).collect()
    }
}

impl Draw for KeyCombo {
    fn size(&self, t: &Theme) -> (u16, u16) {
        (self.caps().iter().map(|c| c.size(t).0).sum(), 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut x = area.x;
        for c in self.caps() {
            let w = c.size(t).0;
            c.draw(t, Rect::new(x, area.y, w, 1).intersection(area), buf);
            x += w;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Menu

/// One row of a [`Menu`].
#[derive(Clone, Default)]
pub struct MenuItem {
    label: String,
    icon: String,
    shortcut: String,
    check: Option<bool>,
    radio: Option<bool>,
    submenu: bool,
    disabled: bool,
    destructive: bool,
    cursor: bool,
    key: Option<String>,
}

impl MenuItem {
    pub fn new(label: impl Into<String>) -> Self {
        MenuItem { label: label.into(), ..Default::default() }
    }
    pub fn icon(mut self, g: impl Into<String>) -> Self {
        self.icon = g.into();
        self
    }
    pub fn shortcut(mut self, s: impl Into<String>) -> Self {
        self.shortcut = s.into();
        self
    }
    /// A checkable row, checked or not.
    pub fn check(mut self, on: bool) -> Self {
        self.check = Some(on);
        self
    }
    /// An exclusive row, chosen or not.
    pub fn radio(mut self, on: bool) -> Self {
        self.radio = Some(on);
        self
    }
    pub fn submenu(mut self, on: bool) -> Self {
        self.submenu = on;
        self
    }
    pub fn disabled(mut self, on: bool) -> Self {
        self.disabled = on;
        self
    }
    pub fn destructive(mut self, on: bool) -> Self {
        self.destructive = on;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    pub fn is_disabled(&self) -> bool {
        self.disabled
    }
    fn mark(&self) -> Option<&'static str> {
        if self.check == Some(true) {
            Some(glyphs::CHECK)
        } else if self.radio == Some(true) {
            Some(glyphs::DOT)
        } else if self.submenu {
            Some(glyphs::SUBMENU)
        } else {
            None
        }
    }
    fn has_mark_slot(&self) -> bool {
        self.check.is_some() || self.radio.is_some() || self.submenu
    }
}

#[derive(Clone)]
enum Entry {
    Item(MenuItem),
    Separator,
    Section(String),
}

/// A menu on the `menu` surface: a box in the menu border color, one row per item:
/// `[glyph ]label ··· hint [mark]`, the cursor row in `menu.selectedBackground` with
/// `menu.selectedText`; separators are `track` rules, sections SectionHeaders.
#[derive(Clone)]
pub struct Menu {
    entries: Vec<Entry>,
    min_width: u16,
}

impl Default for Menu {
    fn default() -> Self {
        Self::new()
    }
}

impl Menu {
    pub fn new() -> Self {
        Menu { entries: Vec::new(), min_width: 20 }
    }
    pub fn item(mut self, i: MenuItem) -> Self {
        self.entries.push(Entry::Item(i));
        self
    }
    pub fn items(mut self, is: impl IntoIterator<Item = MenuItem>) -> Self {
        self.entries.extend(is.into_iter().map(Entry::Item));
        self
    }
    pub fn separator(mut self) -> Self {
        self.entries.push(Entry::Separator);
        self
    }
    pub fn section(mut self, label: impl Into<String>) -> Self {
        self.entries.push(Entry::Section(label.into()));
        self
    }
    pub fn min_width(mut self, w: u16) -> Self {
        self.min_width = w;
        self
    }
    fn items_iter(&self) -> impl Iterator<Item = &MenuItem> {
        self.entries.iter().filter_map(|e| if let Entry::Item(i) = e { Some(i) } else { None })
    }
    fn columns(&self) -> (bool, bool) {
        (self.items_iter().any(|i| !i.icon.is_empty()), self.items_iter().any(|i| i.has_mark_slot()))
    }
    fn row_width(&self, i: &MenuItem) -> u16 {
        let (icons, marks) = self.columns();
        2 + if icons { 2 } else { 0 } + width(&i.label) + if i.shortcut.is_empty() { 0 } else { 2 + width(&i.shortcut) } + if marks { 2 } else { 0 }
    }

    /// Move a menu's cursor (an index among its items, disabled ones skipped) for a key:
    /// Up/Down and k/j wrap, Home/End jump. Returns true when it moved. `enabled[i]` says whether
    /// item `i` takes the cursor.
    pub fn handle_key(cursor: &mut usize, enabled: &[bool], key: &KeyEvent) -> bool {
        let n = enabled.len();
        if n == 0 || !enabled.iter().any(|e| *e) {
            return false;
        }
        let step = |from: usize, d: isize| {
            let mut i = from;
            for _ in 0..n {
                i = (i as isize + d).rem_euclid(n as isize) as usize;
                if enabled[i] {
                    return i;
                }
            }
            from
        };
        match key.code {
            KeyCode::Down | KeyCode::Char('j') => *cursor = step(*cursor, 1),
            KeyCode::Up | KeyCode::Char('k') => *cursor = step(*cursor, -1),
            KeyCode::Home => *cursor = step(n - 1, 1),
            KeyCode::End => *cursor = step(0, -1),
            _ => return false,
        }
        true
    }
}

impl Draw for Menu {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let w = self.items_iter().map(|i| self.row_width(i)).max().unwrap_or(0).max(self.min_width);
        (w + 2, self.entries.len() as u16 + 2)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        clear(buf, area);
        let s = t.shown();
        let m = &s.surfaces.menu;
        let inner = frame(t, buf, area, m.base.border.color, None);
        let (icons, marks) = self.columns();
        for (n, e) in self.entries.iter().enumerate() {
            let y = inner.y + n as u16;
            if y >= inner.bottom() {
                break;
            }
            match e {
                Entry::Separator => hline(t, buf, inner.x, y, inner.width, "─", t.track()),
                Entry::Section(label) => {
                    text_fit(t, buf, inner.x + 1, y, label, inner.width.saturating_sub(2), Ink::new(t.dim()).bold(true));
                }
                Entry::Item(i) => {
                    let row = Rect::new(inner.x, y, inner.width, 1);
                    let hot = i.cursor && !i.disabled;
                    if hot {
                        fill(t, buf, row, m.selected_background);
                    }
                    let ink = if i.disabled {
                        t.faint()
                    } else if i.destructive {
                        t.urgent()
                    } else if hot {
                        m.selected_text
                    } else {
                        m.base.text
                    };
                    let mut x = row.x + 1;
                    if icons {
                        if !i.icon.is_empty() {
                            put(t, buf, x, y, &i.icon, ink);
                        }
                        x += 2;
                    }
                    let right = row.right().saturating_sub(1 + if marks { 2 } else { 0 });
                    let hint_w = if i.shortcut.is_empty() { 0 } else { width(&i.shortcut) + 2 };
                    text_fit(t, buf, x, y, &i.label, right.saturating_sub(x + hint_w), ink);
                    if !i.shortcut.is_empty() {
                        text_right(t, buf, right, y, &i.shortcut, if i.disabled { t.faint() } else { t.dim() });
                    }
                    if let Some(mark) = i.mark() {
                        let mx = row.right().saturating_sub(2);
                        let mink = if i.disabled {
                            Ink::new(t.faint())
                        } else if i.submenu && i.check.is_none() && i.radio.is_none() {
                            Ink::new(t.dim())
                        } else {
                            Ink::new(t.accent()).bold(true)
                        };
                        put(t, buf, mx, y, mark, mink);
                    }
                    if let Some(k) = &i.key
                        && !i.disabled
                    {
                        t.hit(k.clone(), row);
                    }
                }
            }
        }
    }
}

/// A Menu that opens at a point rather than under a trigger: the same component; the app places
/// it (at the pointer, `t.hit_at` / the mouse event's cell).
pub type ContextMenu = Menu;

// ---------------------------------------------------------------------------------------------
// ShortcutSheet

/// Grouped shortcuts in `columns` columns (4 apart): each group a SectionHeader, then rows of a
/// KeyCombo column and the description two columns after it.
pub struct ShortcutSheet {
    groups: Vec<(String, Vec<(String, String)>)>,
    columns: usize,
}

impl Default for ShortcutSheet {
    fn default() -> Self {
        Self::new()
    }
}

impl ShortcutSheet {
    pub fn new() -> Self {
        ShortcutSheet { groups: Vec::new(), columns: 2 }
    }
    pub fn group(mut self, title: impl Into<String>, rows: impl IntoIterator<Item = (impl Into<String>, impl Into<String>)>) -> Self {
        self.groups.push((title.into(), rows.into_iter().map(|(a, b)| (a.into(), b.into())).collect()));
        self
    }
    pub fn columns(mut self, n: usize) -> Self {
        self.columns = n.max(1);
        self
    }
    fn group_size(t: &Theme, rows: &[(String, String)]) -> (u16, u16, u16) {
        let combo = rows.iter().map(|(c, _)| KeyCombo::new(c.clone()).size(t).0).max().unwrap_or(0);
        let desc = rows.iter().map(|(_, d)| width(d)).max().unwrap_or(0);
        (combo + 2 + desc, 1 + rows.len() as u16, combo)
    }
    fn layout(&self, t: &Theme) -> Vec<Vec<usize>> {
        let mut cols = vec![Vec::new(); self.columns];
        for i in 0..self.groups.len() {
            cols[i % self.columns].push(i);
        }
        let _ = t;
        cols
    }
}

impl Draw for ShortcutSheet {
    fn size(&self, t: &Theme) -> (u16, u16) {
        let mut w = 0;
        let mut h = 0;
        for (ci, col) in self.layout(t).iter().enumerate() {
            let cw = col.iter().map(|&g| Self::group_size(t, &self.groups[g].1).0.max(width(&self.groups[g].0))).max().unwrap_or(0);
            let ch: u16 = col.iter().map(|&g| Self::group_size(t, &self.groups[g].1).1).sum::<u16>() + col.len().saturating_sub(1) as u16;
            w += cw + if ci > 0 { 4 } else { 0 };
            h = h.max(ch);
        }
        (w, h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut x = area.x;
        for col in self.layout(t) {
            let cw = col.iter().map(|&g| Self::group_size(t, &self.groups[g].1).0.max(width(&self.groups[g].0))).max().unwrap_or(0);
            let mut y = area.y;
            for g in col {
                let (title, rows) = &self.groups[g];
                let (_, gh, combo_w) = Self::group_size(t, rows);
                text_fit(t, buf, x, y, title, cw, Ink::new(t.dim()).bold(true));
                for (i, (combo, desc)) in rows.iter().enumerate() {
                    let ry = y + 1 + i as u16;
                    let kc = KeyCombo::new(combo.clone());
                    let kw = kc.size(t).0;
                    kc.draw(t, Rect::new(x, ry, kw, 1).intersection(area), buf);
                    text_fit(t, buf, x + combo_w + 2, ry, desc, cw.saturating_sub(combo_w + 2), t.fg());
                }
                y += gh + 1;
            }
            x += cw + 4;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Toolbar

/// A `track` rule `│` with a column clear each side: splits a toolbar's groups.
pub struct ToolbarSeparator;

impl Draw for ToolbarSeparator {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (3, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        put(t, buf, area.x + 1, area.y, "│", t.track());
    }
}

/// See [`ToolbarSeparator`].
pub fn toolbar_separator() -> ToolbarSeparator {
    ToolbarSeparator
}

/// Three slots on one row: leading at the left, center centered on the bar, trailing at the
/// right, then the overflow IconButton; a `track` rule along the bottom row when divided.
pub struct Toolbar {
    leading: Vec<Box<dyn Draw>>,
    center: Vec<Box<dyn Draw>>,
    trailing: Vec<Box<dyn Draw>>,
    overflow: bool,
    overflow_cursor: bool,
    divided: bool,
    width: u16,
}

impl Toolbar {
    pub fn new(width: u16) -> Self {
        Toolbar { leading: Vec::new(), center: Vec::new(), trailing: Vec::new(), overflow: false, overflow_cursor: false, divided: true, width }
    }
    pub fn leading(mut self, d: impl Draw + 'static) -> Self {
        self.leading.push(Box::new(d));
        self
    }
    pub fn center(mut self, d: impl Draw + 'static) -> Self {
        self.center.push(Box::new(d));
        self
    }
    pub fn trailing(mut self, d: impl Draw + 'static) -> Self {
        self.trailing.push(Box::new(d));
        self
    }
    pub fn overflow(mut self, on: bool) -> Self {
        self.overflow = on;
        self
    }
    pub fn overflow_cursor(mut self, on: bool) -> Self {
        self.overflow_cursor = on;
        self
    }
    pub fn divided(mut self, on: bool) -> Self {
        self.divided = on;
        self
    }
    fn slot_w(t: &Theme, ds: &[Box<dyn Draw>]) -> u16 {
        ds.iter().map(|d| d.size(t).0).sum()
    }
    fn height(&self, t: &Theme) -> u16 {
        self.leading.iter().chain(&self.center).chain(&self.trailing).map(|d| d.size(t).1).max().unwrap_or(1)
    }
}

impl Draw for Toolbar {
    fn size(&self, t: &Theme) -> (u16, u16) {
        (self.width, self.height(t) + if self.divided { 1 } else { 0 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let h = self.height(t);
        let place = |buf: &mut Buffer, mut x: u16, ds: &[Box<dyn Draw>]| {
            for d in ds {
                let (w, dh) = d.size(t);
                d.draw(t, Rect::new(x, area.y + (h - dh) / 2, w, dh).intersection(area), buf);
                x += w;
            }
        };
        place(buf, area.x, &self.leading);
        let cw = Self::slot_w(t, &self.center);
        place(buf, area.x + (area.width.saturating_sub(cw)) / 2, &self.center);
        let ow = if self.overflow { 5 } else { 0 };
        let tw = Self::slot_w(t, &self.trailing);
        place(buf, area.right().saturating_sub(tw + ow), &self.trailing);
        if self.overflow {
            super::IconButton::new("\u{f141}").cursor(self.overflow_cursor).key("toolbar-overflow").draw(t, Rect::new(area.right() - 5, area.y + (h - 1) / 2, 5, 1), buf);
        }
        if self.divided {
            hline(t, buf, area.x, area.y + h, area.width, "─", t.track());
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Tabs, IconRail

/// Text tabs ` Label ` over a `track` rule the control's width; the selected tab bold in
/// `controls.selected.text` with its span of the rule `━` in accent. Two rows.
pub struct Tabs {
    labels: Vec<String>,
    current: usize,
    cursor: Option<usize>,
    width: Option<u16>,
}

impl Tabs {
    pub fn new(labels: impl IntoIterator<Item = impl Into<String>>, current: usize) -> Self {
        Tabs { labels: labels.into_iter().map(Into::into).collect(), current, cursor: None, width: None }
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    /// The rule's width (else the tabs').
    pub fn width(mut self, w: u16) -> Self {
        self.width = Some(w);
        self
    }
    /// Left/h, Right/l move `current` within `count`, Home/End jump; true when handled.
    pub fn handle_key(current: &mut usize, count: usize, key: &KeyEvent) -> bool {
        super::ButtonGroup::handle_key(current, count, key)
    }
    fn tabs_w(&self) -> u16 {
        self.labels.iter().map(|l| width(l) + 2).sum()
    }
}

impl Draw for Tabs {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width.unwrap_or(self.tabs_w()), 2)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        hline(t, buf, area.x, area.y + 1, area.width, "─", t.track());
        let mut x = area.x;
        for (i, l) in self.labels.iter().enumerate() {
            let w = width(l) + 2;
            let r = Rect::new(x, area.y, w, 1).intersection(area);
            let selected = i == self.current;
            let hot = self.cursor == Some(i);
            if hot {
                fill(t, buf, r, c.hover.fill);
            }
            let ink = if selected {
                Ink::new(c.selected.text).bold(true)
            } else if hot {
                Ink::new(t.fg())
            } else {
                Ink::new(t.dim())
            };
            text(t, buf, x + 1, area.y, l, w.saturating_sub(2), ink);
            if selected {
                hline(t, buf, x, area.y + 1, w.min(area.right().saturating_sub(x)), "━", t.accent());
            }
            t.hit(format!("tab-{i}"), r);
            x += w;
        }
    }
}

/// A vertical tab rail: one row per item ` G `, the selected glyph accent with a `▎` accent bar
/// at the rail's leading edge, the cursor item in the hover fill, a `track` rule `│` down its
/// trailing edge. Five columns.
pub struct IconRail {
    items: Vec<(String, String)>,
    current: usize,
    cursor: Option<usize>,
    height: Option<u16>,
}

impl IconRail {
    pub fn new(items: impl IntoIterator<Item = (impl Into<String>, impl Into<String>)>, current: usize) -> Self {
        IconRail { items: items.into_iter().map(|(g, l)| (g.into(), l.into())).collect(), current, cursor: None, height: None }
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    /// The rule's height (else one row per item).
    pub fn height(mut self, h: u16) -> Self {
        self.height = Some(h);
        self
    }
    /// Up/k, Down/j move `current`, Home/End jump; true when handled.
    pub fn handle_key(current: &mut usize, count: usize, key: &KeyEvent) -> bool {
        match key.code {
            KeyCode::Up | KeyCode::Char('k') => *current = current.saturating_sub(1),
            KeyCode::Down | KeyCode::Char('j') => *current = (*current + 1).min(count.saturating_sub(1)),
            KeyCode::Home => *current = 0,
            KeyCode::End => *current = count.saturating_sub(1),
            _ => return false,
        }
        true
    }
}

impl Draw for IconRail {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (5, self.height.unwrap_or(self.items.len() as u16))
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        vline(t, buf, area.x + 4, area.y, area.height, "│", t.track());
        for (i, (g, _)) in self.items.iter().enumerate() {
            let y = area.y + i as u16;
            if y >= area.bottom() {
                break;
            }
            let r = Rect::new(area.x + 1, y, 3, 1);
            let hot = self.cursor == Some(i);
            if hot {
                fill(t, buf, r, c.hover.fill);
            }
            let selected = i == self.current;
            if selected {
                put(t, buf, area.x, y, "▎", t.accent());
            }
            let ink = if selected {
                t.accent()
            } else if hot {
                t.fg()
            } else {
                t.dim()
            };
            put(t, buf, area.x + 2, y, g, ink);
            t.hit(format!("rail-{i}"), Rect::new(area.x, y, 4, 1));
        }
    }
}
