//! SearchList: the type-ahead overlay (spec/terminal.md, Composed).

use crossterm::event::{KeyCode, KeyEvent};
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::{CursorModel, EmptyState, ListRow, ScrollArea, printable, wrap};
use crate::paint::{Ink, clear, frame, text_fit, vline};
use crate::{Draw, Theme};

/// One result: title, subtitle, glyph, value, detail.
#[derive(Clone, Debug, Default)]
pub struct SearchItem {
    pub title: String,
    pub subtitle: String,
    pub glyph: String,
    pub value: String,
    pub detail: String,
}

impl SearchItem {
    pub fn new(title: impl Into<String>) -> Self {
        SearchItem { title: title.into(), ..Default::default() }
    }
    pub fn subtitle(mut self, s: impl Into<String>) -> Self {
        self.subtitle = s.into();
        self
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn value(mut self, v: impl Into<String>) -> Self {
        self.value = v.into();
        self
    }
    pub fn detail(mut self, d: impl Into<String>) -> Self {
        self.detail = d.into();
        self
    }
}

/// What a key did to a SearchList.
#[derive(Clone, Debug, PartialEq)]
pub enum SearchKey {
    /// The query changed (the caller filters again; the cursor is back on the first match).
    Query,
    Moved,
    Activate(usize),
    Delete(usize),
    Close,
    Ignored,
}

/// On the `menu` surface: a box in its border color, padded 2 × 1; the filter line (the query, or
/// the placeholder in `textDim`), a blank row, then the body: ListRows under the cursor and, with
/// the detail pane, two columns, a `│` rule in `track`, two columns, and the pane (the cursor
/// item's title bold over its detail in `textDim`, wrapped). No items: an overlay EmptyState
/// centered in the body.
pub struct SearchList {
    query: String,
    items: Vec<SearchItem>,
    cursor: Option<usize>,
    offset: u16,
    detail: bool,
    placeholder: Option<String>,
    empty: Option<(String, String)>,
    width: u16,
    height: u16,
    key: String,
}

impl SearchList {
    pub fn new(key: impl Into<String>, query: impl Into<String>) -> Self {
        SearchList { query: query.into(), items: Vec::new(), cursor: Some(0), offset: 0, detail: false, placeholder: None, empty: None, width: 60, height: 22, key: key.into() }
    }
    pub fn items(mut self, items: impl IntoIterator<Item = SearchItem>) -> Self {
        self.items = items.into_iter().collect();
        self
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    /// The first body row shown.
    pub fn offset(mut self, rows: u16) -> Self {
        self.offset = rows;
        self
    }
    pub fn detail(mut self, on: bool) -> Self {
        self.detail = on;
        self
    }
    pub fn placeholder(mut self, p: impl Into<String>) -> Self {
        self.placeholder = Some(p.into());
        self
    }
    /// What an empty query with no items shows.
    pub fn empty(mut self, glyph: impl Into<String>, text: impl Into<String>) -> Self {
        self.empty = Some((glyph.into(), text.into()));
        self
    }
    pub fn size_cells(mut self, w: u16, h: u16) -> Self {
        self.width = w;
        self.height = h;
        self
    }
    /// The plain case-insensitive substring test.
    pub fn matches(text: &str, query: &str) -> bool {
        text.to_lowercase().contains(&query.to_lowercase())
    }
    /// Keys, the filter line holding focus: printable keys and Space type, Backspace edits, Up,
    /// Down, Page Up/Down, Home/End move the cursor, Return activates, Delete asks to delete,
    /// Escape clears the query or, when empty, asks to close.
    pub fn handle_key(query: &mut String, cursor: &mut CursorModel, key: &KeyEvent) -> SearchKey {
        if let Some(s) = printable(key) {
            query.push_str(&s);
            cursor.set("", 0);
            return SearchKey::Query;
        }
        match key.code {
            KeyCode::Char(' ') => {
                query.push(' ');
                cursor.set("", 0);
                SearchKey::Query
            }
            KeyCode::Backspace => {
                query.pop();
                cursor.set("", 0);
                SearchKey::Query
            }
            KeyCode::Esc if !query.is_empty() => {
                query.clear();
                cursor.set("", 0);
                SearchKey::Query
            }
            KeyCode::Esc => SearchKey::Close,
            KeyCode::Up | KeyCode::Down | KeyCode::PageUp | KeyCode::PageDown | KeyCode::Home | KeyCode::End => {
                cursor.active = true;
                cursor.apply_key(key);
                SearchKey::Moved
            }
            KeyCode::Enter => SearchKey::Activate(cursor.index()),
            KeyCode::Delete => SearchKey::Delete(cursor.index()),
            _ => SearchKey::Ignored,
        }
    }
}

impl Draw for SearchList {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, self.height)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        clear(buf, area);
        let m = &t.shown().surfaces.menu;
        let inner = crate::paint::inset(frame(t, buf, area, m.base.border.color, None), 2, 1);
        if self.query.is_empty() {
            let p = self.placeholder.clone().unwrap_or_else(|| t.string("common.search", "Search..."));
            text_fit(t, buf, inner.x, inner.y, &p, inner.width, t.dim());
        } else {
            text_fit(t, buf, inner.x, inner.y, &self.query, inner.width, m.base.text);
        }
        t.hit(format!("{}-filter", self.key), Rect::new(inner.x, inner.y, inner.width, 1));
        let body = Rect::new(inner.x, inner.y + 2, inner.width, inner.height.saturating_sub(2));
        if self.items.is_empty() {
            let (glyph, text) = if self.query.is_empty() {
                self.empty.clone().unwrap_or(("\u{f0349}".into(), String::new()))
            } else {
                ("\u{f0349}".into(), t.string("common.noMatchesFor", "No matches for “%1”").replace("%1", &self.query))
            };
            let e = EmptyState::new(text).glyph(glyph).overlay(true).width(body.width);
            let (_, eh) = e.size(t);
            e.draw(t, Rect::new(body.x, body.y + body.height.saturating_sub(eh) / 2, body.width, eh), buf);
            return;
        }
        let list_w = if self.detail { (body.width as u32 * 55 / 100) as u16 } else { body.width };
        let mut list = ScrollArea::new(list_w).max_rows(body.height).offset(self.offset);
        for (i, it) in self.items.iter().enumerate() {
            let row = ListRow::new(it.title.clone()).glyph(it.glyph.clone()).subtitle(it.subtitle.clone()).value(it.value.clone()).cursor(self.cursor == Some(i)).width(list_w).key(format!("{}-row-{i}", self.key));
            list = list.child(row);
        }
        let (_, lh) = list.size(t);
        list.draw(t, Rect::new(body.x, body.y, list_w, lh), buf);
        if self.detail {
            let rx = body.x + list_w + 2;
            vline(t, buf, rx, body.y, body.height, "│", t.track());
            let px = rx + 3;
            let pw = body.right().saturating_sub(px);
            if let Some(it) = self.cursor.and_then(|i| self.items.get(i)) {
                text_fit(t, buf, px, body.y, &it.title, pw, Ink::new(m.base.text).bold(true));
                for (n, line) in wrap(&it.detail, pw).iter().enumerate() {
                    let y = body.y + 1 + n as u16;
                    if y >= body.bottom() {
                        break;
                    }
                    text_fit(t, buf, px, y, line, pw, t.dim());
                }
            }
        }
    }
}
