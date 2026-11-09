//! Setting rows and the multi-choice dropdown (spec/terminal.md, Forms). Checkbox, Radio,
//! RadioGroup, PasswordField, TextArea, ColorWell and SwatchStrip are with the controls; Dialog with
//! the surfaces.

use crossterm::event::{KeyCode, KeyEvent};
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::controls::{check_box, trigger};
use crate::paint::{Ink, clear, fill, frame, hline, put, text_fit};
use crate::{Draw, Theme};

/// A row of a settings list: the label (and a `textDim` description under it) at the start, any
/// control at the end of the first row. No frame; the cursor row takes the hover fill.
pub struct SettingRow {
    label: String,
    description: String,
    control: Option<Box<dyn Draw>>,
    cursor: bool,
    disabled: bool,
    width: u16,
    key: Option<String>,
}

impl SettingRow {
    pub fn new(label: impl Into<String>) -> Self {
        SettingRow { label: label.into(), description: String::new(), control: None, cursor: false, disabled: false, width: 60, key: None }
    }
    pub fn description(mut self, d: impl Into<String>) -> Self {
        self.description = d.into();
        self
    }
    pub fn control(mut self, d: impl Draw + 'static) -> Self {
        self.control = Some(Box::new(d));
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
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
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for SettingRow {
    fn size(&self, t: &Theme) -> (u16, u16) {
        let ch = self.control.as_ref().map_or(1, |c| c.size(t).1);
        (self.width, (if self.description.is_empty() { 1 } else { 2 }).max(ch))
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        if self.cursor && !self.disabled {
            fill(t, buf, area, t.shown().controls.hover.fill);
        }
        let cw = self.control.as_ref().map_or(0, |c| c.size(t).0);
        let words = area.width.saturating_sub(cw + 3);
        let (ink, dim) = if self.disabled { (t.faint(), t.faint()) } else { (t.fg(), t.dim()) };
        text_fit(t, buf, area.x + 1, area.y, &self.label, words, ink);
        if !self.description.is_empty() {
            text_fit(t, buf, area.x + 1, area.y + 1, &self.description, words, dim);
        }
        if let Some(c) = &self.control {
            let (w, h) = c.size(t);
            c.draw(t, Rect::new(area.right().saturating_sub(w + 1), area.y, w, h), buf);
        }
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

/// The searchable multi-choice dropdown: a label row (bold `textDim`), the trigger (the chosen
/// labels joined by ", ", "N selected" past three, "None selected" in `textFaint`), and when open
/// the popup under it: a box in the popups border color with the search row (`󰍉` and the query or
/// placeholder, in the focus look), a rule, then a checkbox row per option.
pub struct MultiSelect {
    label: String,
    options: Vec<(String, String)>,
    values: Vec<String>,
    open: bool,
    cursor: bool,
    highlighted: Option<usize>,
    search: String,
    width: u16,
    key: String,
}

impl MultiSelect {
    pub fn new(key: impl Into<String>, options: impl IntoIterator<Item = (impl Into<String>, impl Into<String>)>, values: impl IntoIterator<Item = impl Into<String>>) -> Self {
        MultiSelect {
            label: String::new(),
            options: options.into_iter().map(|(v, l)| (v.into(), l.into())).collect(),
            values: values.into_iter().map(Into::into).collect(),
            open: false,
            cursor: false,
            highlighted: None,
            search: String::new(),
            width: 33,
            key: key.into(),
        }
    }
    pub fn label(mut self, l: impl Into<String>) -> Self {
        self.label = l.into();
        self
    }
    pub fn open(mut self, on: bool) -> Self {
        self.open = on;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// The row under the keyboard among those the search shows (None: the keys are in the search).
    pub fn highlighted(mut self, i: Option<usize>) -> Self {
        self.highlighted = i;
        self
    }
    pub fn search(mut self, q: impl Into<String>) -> Self {
        self.search = q.into();
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    /// The options the search shows (case-insensitive substring of the label).
    pub fn shown(&self) -> Vec<&(String, String)> {
        let q = self.search.to_lowercase();
        self.options.iter().filter(|(_, l)| l.to_lowercase().contains(&q)).collect()
    }
    fn trigger_text(&self, t: &Theme) -> (String, bool) {
        let chosen: Vec<&str> = self.options.iter().filter(|(v, _)| self.values.contains(v)).map(|(_, l)| l.as_str()).collect();
        match chosen.len() {
            0 => (t.string("common.noneSelected", "None selected"), false),
            n if n > 3 => (t.string("multiSelect.selectedCount", "%1 selected").replace("%1", &n.to_string()).replace("{count}", &n.to_string()), true),
            _ => (chosen.join(", "), true),
        }
    }
    fn popup_rows(&self) -> u16 {
        self.shown().len().min(6) as u16
    }

    /// The keys, as the GUI bindings take them. On the closed trigger: j, Down, Return or Space
    /// open. Open, in the search (`highlighted` None): typing filters (`search`), Return toggles
    /// the first match, Down goes into the list. In the list: j/k and Down/Up walk (Up off the
    /// first row goes back to the search), Return or Space toggles. Escape closes, clearing the
    /// search. Returns the value to toggle, if any; `handled` says whether the key was used.
    pub fn handle_key(state: &mut MultiSelectState, options: &[(&str, &str)], key: &KeyEvent) -> (bool, Option<String>) {
        let q = state.search.to_lowercase();
        let shown: Vec<&(&str, &str)> = options.iter().filter(|(_, l)| l.to_lowercase().contains(&q)).collect();
        if !state.open {
            return match key.code {
                KeyCode::Char('j') | KeyCode::Down | KeyCode::Enter | KeyCode::Char(' ') => {
                    state.open = true;
                    state.row = None;
                    (true, None)
                }
                _ => (false, None),
            };
        }
        match (key.code, state.row) {
            (KeyCode::Esc, _) => {
                state.open = false;
                state.search.clear();
                state.row = None;
                (true, None)
            }
            (KeyCode::Enter, None) => (true, shown.first().map(|(v, _)| v.to_string())),
            (KeyCode::Down, None) => {
                state.row = (!shown.is_empty()).then_some(0);
                (true, None)
            }
            (KeyCode::Backspace, None) => {
                state.search.pop();
                (true, None)
            }
            (KeyCode::Char(c), None) => {
                state.search.push(c);
                (true, None)
            }
            (KeyCode::Down | KeyCode::Char('j'), Some(r)) => {
                state.row = Some((r + 1).min(shown.len().saturating_sub(1)));
                (true, None)
            }
            (KeyCode::Up | KeyCode::Char('k'), Some(r)) => {
                state.row = r.checked_sub(1);
                (true, None)
            }
            (KeyCode::Enter | KeyCode::Char(' '), Some(r)) => (true, shown.get(r).map(|(v, _)| v.to_string())),
            _ => (false, None),
        }
    }
}

/// The app's state for a MultiSelect's keys.
#[derive(Clone, Debug, Default)]
pub struct MultiSelectState {
    pub open: bool,
    pub search: String,
    pub row: Option<usize>,
}

impl Draw for MultiSelect {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let label = if self.label.is_empty() { 0 } else { 1 };
        let popup = if self.open { 2 + 2 + self.popup_rows().max(1) } else { 0 };
        (self.width, label + 1 + popup)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut y = area.y;
        if !self.label.is_empty() {
            text_fit(t, buf, area.x, y, &self.label, area.width, Ink::new(t.dim()).bold(true));
            y += 1;
        }
        let (s, chosen) = self.trigger_text(t);
        trigger(t, buf, area.x, y, area.width, self.cursor, &s, if chosen { t.fg() } else { t.faint() });
        t.hit(self.key.clone(), Rect::new(area.x, y, area.width, 1));
        if !self.open {
            return;
        }
        let rows = self.popup_rows().max(1);
        let pop = Rect::new(area.x, y + 1, area.width, rows + 4).intersection(area);
        clear(buf, pop);
        let c = &t.shown().controls;
        let inner = frame(t, buf, pop, t.shown().surfaces.popups.border.color, None);
        // The search row holds the focus while open: the focus look.
        let search_hot = self.highlighted.is_none();
        if search_hot {
            fill(t, buf, Rect::new(inner.x, inner.y, inner.width, 1), c.focus.fill);
        }
        put(t, buf, inner.x + 1, inner.y, "\u{f0349}", t.dim());
        if self.search.is_empty() {
            text_fit(t, buf, inner.x + 3, inner.y, &t.string("common.search", "Search..."), inner.width.saturating_sub(4), t.faint());
        } else {
            text_fit(t, buf, inner.x + 3, inner.y, &self.search, inner.width.saturating_sub(4), t.fg());
        }
        hline(t, buf, inner.x, inner.y + 1, inner.width, "─", t.fg().with_alpha(0.10));
        let shown = self.shown();
        if shown.is_empty() {
            text_fit(t, buf, inner.x + 1, inner.y + 2, &t.string("common.noOptions", "No options"), inner.width.saturating_sub(2), t.dim());
        }
        for (i, (v, l)) in shown.iter().enumerate().take(rows as usize) {
            let ry = inner.y + 2 + i as u16;
            let hot = self.highlighted == Some(i);
            if hot {
                fill(t, buf, Rect::new(inner.x, ry, inner.width, 1), c.hover.fill);
            }
            let on = self.values.contains(v);
            check_box(t, buf, inner.x + 1, ry, on, false, false, false);
            text_fit(t, buf, inner.x + 5, ry, l, inner.width.saturating_sub(6), Ink::new(t.fg()).bold(on));
            t.hit(format!("{}-{v}", self.key), Rect::new(inner.x, ry, inner.width, 1));
        }
    }
}
