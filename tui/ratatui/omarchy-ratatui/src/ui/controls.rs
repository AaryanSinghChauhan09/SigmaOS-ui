//! One-row controls (spec/terminal.md, Controls): `[edge][pad] content [pad][edge]`, the edge
//! cells reserved whenever any state shows a border, the fill across the whole width.

use omarchy_theme::Rgba;
use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::{Look, glyphs, look};
use crate::paint::{self, Ink, edges, fill, put, text_center, text_fit, width};
use crate::{Draw, Theme};

/// Paint a control's frame on row `y` over `w` cells: the fill across all of them, the edges on
/// its first and last cell.
pub(crate) fn control(t: &Theme, buf: &mut Buffer, x: u16, y: u16, w: u16, l: &Look) {
    if w < 2 {
        return;
    }
    fill(t, buf, Rect::new(x, y, w, 1), l.fill);
    edges(t, buf, x, y, w, l.border);
}

// ---------------------------------------------------------------------------------------------
// Button, IconButton, ButtonGroup

/// Label, optional icon glyph. Borderless at rest unless `bordered`; `selected` fills and bolds.
pub struct Button {
    label: String,
    icon: Option<String>,
    bordered: bool,
    selected: bool,
    cursor: bool,
    width: Option<u16>,
    key: Option<String>,
}

impl Button {
    pub fn new(label: impl Into<String>) -> Self {
        Button { label: label.into(), icon: None, bordered: false, selected: false, cursor: false, width: None, key: None }
    }
    pub fn icon(mut self, glyph: impl Into<String>) -> Self {
        self.icon = Some(glyph.into());
        self
    }
    pub fn bordered(mut self, on: bool) -> Self {
        self.bordered = on;
        self
    }
    pub fn selected(mut self, on: bool) -> Self {
        self.selected = on;
        self
    }
    /// The hover-cursor look (keyboard cursor, focus, pointer).
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// A fixed width (the label centers); by default label + 4.
    pub fn width(mut self, w: u16) -> Self {
        self.width = Some(w);
        self
    }
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for Button {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let natural = 4 + width(&self.label) + if self.icon.is_some() { 2 } else { 0 };
        (self.width.unwrap_or(natural), 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let l = look(t, self.cursor, self.selected, false, self.bordered);
        control(t, buf, area.x, area.y, area.width, &l);
        let inner = area.width.saturating_sub(4);
        let content = match &self.icon {
            Some(g) => format!("{g} {}", self.label),
            None => self.label.clone(),
        };
        text_center(t, buf, area.x + 2, area.y, inner, &content, Ink::new(l.text).bold(l.bold));
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

/// One glyph in a square-ish cell: `[edge] G [edge]`, 5 columns.
pub struct IconButton {
    glyph: String,
    bordered: bool,
    cursor: bool,
    key: Option<String>,
}

impl IconButton {
    pub fn new(glyph: impl Into<String>) -> Self {
        IconButton { glyph: glyph.into(), bordered: false, cursor: false, key: None }
    }
    pub fn bordered(mut self, on: bool) -> Self {
        self.bordered = on;
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
}

impl Draw for IconButton {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (5, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        // PanelActionButton shows the hover border only when bordered.
        let l = if self.cursor {
            Look { fill: c.hover.fill, border: if self.bordered { super::border_of(&c.hover.border) } else { None }, text: t.fg(), bold: false }
        } else {
            look(t, false, false, false, self.bordered)
        };
        control(t, buf, area.x, area.y, area.width, &l);
        put(t, buf, area.x + 2, area.y, &self.glyph, l.text);
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

/// Bordered buttons one column apart, one selected: a segmented choice. Left/Right (h/l) move the
/// choice ([`ButtonGroup::handle_key`]).
pub struct ButtonGroup {
    labels: Vec<String>,
    selected: usize,
    cursor: Option<usize>,
    key: Option<String>,
}

impl ButtonGroup {
    pub fn new(labels: impl IntoIterator<Item = impl Into<String>>, selected: usize) -> Self {
        ButtonGroup { labels: labels.into_iter().map(Into::into).collect(), selected, cursor: None, key: None }
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    /// Buttons keyed `<k>-<i>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    /// Left/h and Right/l move `selected` within `count`, stopping at the ends; true when handled.
    pub fn handle_key(selected: &mut usize, count: usize, key: &crossterm::event::KeyEvent) -> bool {
        use crossterm::event::KeyCode::*;
        match key.code {
            Left | Char('h') => *selected = selected.saturating_sub(1),
            Right | Char('l') => *selected = (*selected + 1).min(count.saturating_sub(1)),
            Home => *selected = 0,
            End => *selected = count.saturating_sub(1),
            _ => return false,
        }
        true
    }
    fn buttons(&self) -> Vec<Button> {
        self.labels
            .iter()
            .enumerate()
            .map(|(i, l)| {
                let b = Button::new(l.clone()).bordered(true).selected(i == self.selected).cursor(self.cursor == Some(i));
                match &self.key {
                    Some(k) => b.key(format!("{k}-{i}")),
                    None => b,
                }
            })
            .collect()
    }
}

impl Draw for ButtonGroup {
    fn size(&self, t: &Theme) -> (u16, u16) {
        let bs = self.buttons();
        (bs.iter().map(|b| b.size(t).0).sum::<u16>() + bs.len().saturating_sub(1) as u16, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut x = area.x;
        for b in self.buttons() {
            let w = b.size(t).0;
            b.draw(t, Rect::new(x, area.y, w, 1).intersection(area), buf);
            x += w + 1;
        }
    }
}

// ---------------------------------------------------------------------------------------------
// ToggleSwitch, Toggle

/// The bare switch: four cells of track, the knob `██` at the start (off) or the end (on). With
/// the cursor its edges show (the ring), so it is 6 columns; [`ToggleSwitch::bare`] is the 4.
pub struct ToggleSwitch {
    on: bool,
    cursor: bool,
    bare: bool,
    key: Option<String>,
}

impl ToggleSwitch {
    pub fn new(on: bool) -> Self {
        ToggleSwitch { on, cursor: false, bare: false, key: None }
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// The track alone (inside a Toggle or a ListRow): no ring, 4 columns.
    pub fn bare(mut self) -> Self {
        self.bare = true;
        self
    }
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

/// The four track cells at (x, y).
pub(crate) fn switch_track(t: &Theme, buf: &mut Buffer, x: u16, y: u16, on: bool) {
    let c = &t.shown().controls;
    fill(t, buf, Rect::new(x, y, 4, 1), if on { c.selected.fill } else { c.normal.fill });
    let knob = if on { c.selected.text } else { t.fg().darker(1.25) };
    let kx = if on { x + 2 } else { x };
    put(t, buf, kx, y, "█", knob);
    put(t, buf, kx + 1, y, "█", knob);
}

impl Draw for ToggleSwitch {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (if self.bare { 4 } else { 6 }, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        if self.bare {
            switch_track(t, buf, area.x, area.y, self.on);
        } else {
            let ring = if self.cursor { super::border_of(&t.shown().controls.hover.border) } else { None };
            edges(t, buf, area.x, area.y, 6, ring);
            switch_track(t, buf, area.x + 1, area.y, self.on);
        }
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

/// A labeled row with the switch at its end; the whole row is the control. Two rows with a
/// description (title bold, description `textDim` under it), one without.
pub struct Toggle {
    title: String,
    description: String,
    on: bool,
    cursor: bool,
    width: u16,
    key: Option<String>,
}

impl Toggle {
    pub fn new(title: impl Into<String>) -> Self {
        Toggle { title: title.into(), description: String::new(), on: false, cursor: false, width: 45, key: None }
    }
    pub fn description(mut self, d: impl Into<String>) -> Self {
        self.description = d.into();
        self
    }
    pub fn on(mut self, on: bool) -> Self {
        self.on = on;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
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

impl Draw for Toggle {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, if self.description.is_empty() { 1 } else { 2 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let l = look(t, self.cursor, false, true, true);
        for dy in 0..area.height {
            control(t, buf, area.x, area.y + dy, area.width, &l);
        }
        let words = area.width.saturating_sub(4 + 4 + 1);
        text_fit(t, buf, area.x + 2, area.y, &self.title, words, Ink::new(t.fg()).bold(true));
        if !self.description.is_empty() && area.height > 1 {
            text_fit(t, buf, area.x + 2, area.y + 1, &self.description, words, t.dim());
        }
        switch_track(t, buf, area.right().saturating_sub(6), area.y, self.on);
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// TextField, PasswordField, TextArea

/// A single-line input: normal fill and border at rest, the hover look with the cursor.
pub struct TextField {
    text: String,
    placeholder: String,
    width: u16,
    cursor: bool,
    caret: Option<usize>,
    key: Option<String>,
}

impl TextField {
    pub fn new(text: impl Into<String>) -> Self {
        TextField { text: text.into(), placeholder: String::new(), width: 28, cursor: false, caret: None, key: None }
    }
    pub fn placeholder(mut self, p: impl Into<String>) -> Self {
        self.placeholder = p.into();
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// Show the caret (reverse video) before this char index: the field has keyboard focus.
    pub fn caret(mut self, at: Option<usize>) -> Self {
        self.caret = at;
        self
    }
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

/// Draw a field's text (or placeholder) and caret in `w` cells at (x, y).
pub(crate) fn field_text(t: &Theme, buf: &mut Buffer, x: u16, y: u16, w: u16, s: &str, placeholder: &str, caret: Option<usize>, ink: Rgba) {
    if s.is_empty() {
        text_fit(t, buf, x, y, placeholder, w, t.faint());
    } else {
        text_fit(t, buf, x, y, s, w, ink);
    }
    if let Some(i) = caret {
        let cx = x + width(&s.chars().take(i).collect::<String>()).min(w.saturating_sub(1));
        if let Some(cell) = buf.cell_mut((cx, y)) {
            cell.modifier |= ratatui::style::Modifier::REVERSED;
        }
    }
}

impl Draw for TextField {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let l = look(t, self.cursor, false, true, true);
        control(t, buf, area.x, area.y, area.width, &l);
        field_text(t, buf, area.x + 2, area.y, area.width.saturating_sub(4), &self.text, &self.placeholder, self.caret, t.fg());
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

/// A TextField for secrets: a lock before the text, dots for it (and TextField's caret while
/// focused), an optional eye at the end, and with an error the urgent edges and the message on the
/// row under it.
pub struct PasswordField {
    text: String,
    placeholder: String,
    width: u16,
    cursor: bool,
    caret: Option<usize>,
    revealable: bool,
    revealed: bool,
    error: String,
    key: Option<String>,
}

impl PasswordField {
    pub fn new(text: impl Into<String>) -> Self {
        PasswordField { text: text.into(), placeholder: String::new(), width: 28, cursor: false, caret: None, revealable: false, revealed: false, error: String::new(), key: None }
    }
    pub fn placeholder(mut self, p: impl Into<String>) -> Self {
        self.placeholder = p.into();
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// Show the caret (reverse video) before this char index: the field has keyboard focus.
    pub fn caret(mut self, at: Option<usize>) -> Self {
        self.caret = at;
        self
    }
    pub fn revealable(mut self, on: bool) -> Self {
        self.revealable = on;
        self
    }
    pub fn revealed(mut self, on: bool) -> Self {
        self.revealed = on;
        self
    }
    pub fn error(mut self, msg: impl Into<String>) -> Self {
        self.error = msg.into();
        self
    }
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for PasswordField {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, if self.error.is_empty() { 1 } else { 2 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let s = t.shown();
        let mut l = look(t, self.cursor, false, true, true);
        let erring = !self.error.is_empty();
        if erring {
            l.border = Some(s.surfaces.polkit.border_error.color);
        }
        control(t, buf, area.x, area.y, area.width, &l);
        let glyph_ink = if erring { s.surfaces.polkit.text_error } else { t.dim() };
        put(t, buf, area.x + 2, area.y, "\u{f023}", glyph_ink);
        let eye = if self.revealable { 2 } else { 0 };
        let w = area.width.saturating_sub(6 + eye);
        let shown: String = if self.revealed { self.text.clone() } else { "•".repeat(self.text.chars().count()) };
        field_text(t, buf, area.x + 4, area.y, w, &shown, &self.placeholder, self.caret, t.fg());
        if self.revealable {
            put(t, buf, area.right().saturating_sub(3), area.y, if self.revealed { "\u{f0209}" } else { "\u{f0208}" }, t.dim());
        }
        if erring && area.height > 1 {
            text_fit(t, buf, area.x + 2, area.y + 1, &self.error, area.width.saturating_sub(2), s.surfaces.polkit.text_error);
        }
        if let Some(k) = &self.key {
            t.hit(k.clone(), Rect { height: 1, ..area });
        }
    }
}

/// Wrapped multi-line text in TextField's frame (edges on every row), `rows(min, max)` tall.
pub struct TextArea {
    text: String,
    placeholder: String,
    width: u16,
    min_rows: u16,
    max_rows: u16,
    cursor: bool,
}

impl TextArea {
    pub fn new(text: impl Into<String>) -> Self {
        TextArea { text: text.into(), placeholder: String::new(), width: 60, min_rows: 3, max_rows: 6, cursor: false }
    }
    pub fn placeholder(mut self, p: impl Into<String>) -> Self {
        self.placeholder = p.into();
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    pub fn rows(mut self, min: u16, max: u16) -> Self {
        self.min_rows = min;
        self.max_rows = max.max(min);
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    fn lines(&self) -> Vec<String> {
        wrap(&self.text, self.width.saturating_sub(4))
    }
}

/// Word-wrap `s` to `w` cells (hard newlines kept; a word longer than a line is cut).
pub fn wrap(s: &str, w: u16) -> Vec<String> {
    let mut out = Vec::new();
    for para in s.split('\n') {
        let mut line = String::new();
        for word in para.split(' ') {
            let candidate = if line.is_empty() { word.to_string() } else { format!("{line} {word}") };
            if width(&candidate) <= w || line.is_empty() {
                line = candidate;
            } else {
                out.push(std::mem::take(&mut line));
                line = word.to_string();
            }
        }
        out.push(line);
    }
    out
}

impl Draw for TextArea {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let n = (self.lines().len() as u16).clamp(self.min_rows, self.max_rows);
        (self.width, n)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let l = look(t, self.cursor, false, true, true);
        for dy in 0..area.height {
            control(t, buf, area.x, area.y + dy, area.width, &l);
        }
        let inner = area.width.saturating_sub(4);
        if self.text.is_empty() {
            text_fit(t, buf, area.x + 2, area.y, &self.placeholder, inner, t.faint());
        }
        for (i, line) in self.lines().iter().enumerate().take(area.height as usize) {
            text_fit(t, buf, area.x + 2, area.y + i as u16, line, inner, t.fg());
        }
    }
}

// ---------------------------------------------------------------------------------------------
// NumberField, Dropdown

/// An integer stepper: an optional label row (`textDim`), then `[−]  value  [+]` in a field.
pub struct NumberField {
    label: String,
    value: i64,
    min: i64,
    max: i64,
    width: u16,
    cursor: bool,
    key: Option<String>,
}

impl NumberField {
    pub fn new(value: i64) -> Self {
        NumberField { label: String::new(), value, min: i64::MIN, max: i64::MAX, width: 15, cursor: false, key: None }
    }
    pub fn label(mut self, l: impl Into<String>) -> Self {
        self.label = l.into();
        self
    }
    pub fn range(mut self, min: i64, max: i64) -> Self {
        self.min = min;
        self.max = max;
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    /// The field keyed `<k>`, the steppers `<k>-dec` and `<k>-inc`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    /// Up/k/+ and Down/j/− step within the range; true when handled.
    pub fn handle_key(value: &mut i64, min: i64, max: i64, key: &crossterm::event::KeyEvent) -> bool {
        use crossterm::event::KeyCode::*;
        match key.code {
            Up | Right | Char('k') | Char('l') | Char('+') => *value = (*value + 1).min(max),
            Down | Left | Char('j') | Char('h') | Char('-') => *value = (*value - 1).max(min),
            _ => return false,
        }
        true
    }
}

impl Draw for NumberField {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, if self.label.is_empty() { 1 } else { 2 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut y = area.y;
        if !self.label.is_empty() {
            text_fit(t, buf, area.x, y, &self.label, area.width, t.dim());
            y += 1;
        }
        let l = look(t, self.cursor, false, true, true);
        control(t, buf, area.x, y, area.width, &l);
        let dec = if self.value > self.min { t.fg() } else { t.faint() };
        let inc = if self.value < self.max { t.fg() } else { t.faint() };
        put(t, buf, area.x + 2, y, "−", dec);
        put(t, buf, area.right().saturating_sub(3), y, "+", inc);
        text_center(t, buf, area.x + 4, y, area.width.saturating_sub(8), &self.value.to_string(), t.fg());
        if let Some(k) = &self.key {
            t.hit(k.clone(), Rect::new(area.x, y, area.width, 1));
            t.hit(format!("{k}-dec"), Rect::new(area.x + 1, y, 2, 1));
            t.hit(format!("{k}-inc"), Rect::new(area.right().saturating_sub(3), y, 2, 1));
        }
    }
}

/// A labeled single choice: label row (bold `textDim`), then the trigger: value, chevron at the
/// end. Its list ([`DropdownList`]) is drawn by the app over the page, under the trigger.
pub struct Dropdown {
    label: String,
    value: String,
    width: u16,
    cursor: bool,
    key: Option<String>,
}

impl Dropdown {
    pub fn new(value: impl Into<String>) -> Self {
        Dropdown { label: String::new(), value: value.into(), width: 30, cursor: false, key: None }
    }
    pub fn label(mut self, l: impl Into<String>) -> Self {
        self.label = l.into();
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
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
}

/// A trigger row: field frame, text, chevron.
pub(crate) fn trigger(t: &Theme, buf: &mut Buffer, x: u16, y: u16, w: u16, hot: bool, s: &str, ink: Rgba) {
    let l = look(t, hot, false, true, true);
    control(t, buf, x, y, w, &l);
    text_fit(t, buf, x + 2, y, s, w.saturating_sub(6), ink);
    put(t, buf, x + w.saturating_sub(3), y, glyphs::CHEVRON_DOWN, t.fg().darker(1.2));
}

impl Draw for Dropdown {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, if self.label.is_empty() { 1 } else { 2 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut y = area.y;
        if !self.label.is_empty() {
            text_fit(t, buf, area.x, y, &self.label, area.width, Ink::new(t.dim()).bold(true));
            y += 1;
        }
        trigger(t, buf, area.x, y, area.width, self.cursor, &self.value, t.fg());
        if let Some(k) = &self.key {
            t.hit(k.clone(), Rect::new(area.x, y, area.width, 1));
        }
    }
}

/// A Dropdown's open list on the popups surface: a box in the popups border color, one row per
/// option, the highlighted row in the hover fill, the chosen one with an accent check.
pub struct DropdownList {
    options: Vec<String>,
    chosen: Option<usize>,
    highlighted: Option<usize>,
    width: u16,
    key: Option<String>,
}

impl DropdownList {
    pub fn new(options: impl IntoIterator<Item = impl Into<String>>, chosen: Option<usize>) -> Self {
        DropdownList { options: options.into_iter().map(Into::into).collect(), chosen, highlighted: None, width: 30, key: None }
    }
    pub fn highlighted(mut self, i: Option<usize>) -> Self {
        self.highlighted = i;
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    /// Rows keyed `<k>-<i>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for DropdownList {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, self.options.len() as u16 + 2)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        paint::clear(buf, area);
        let s = t.shown();
        let inner = paint::frame(t, buf, area, s.surfaces.popups.border.color, None);
        for (i, o) in self.options.iter().enumerate() {
            let y = inner.y + i as u16;
            if y >= inner.bottom() {
                break;
            }
            let hot = self.highlighted == Some(i);
            if hot {
                fill(t, buf, Rect::new(inner.x, y, inner.width, 1), s.controls.hover.fill);
            }
            let chosen = self.chosen == Some(i);
            text_fit(t, buf, inner.x + 1, y, o, inner.width.saturating_sub(4), Ink::new(t.fg()).bold(chosen));
            if chosen {
                put(t, buf, inner.right().saturating_sub(2), y, glyphs::CHECK, Ink::new(t.accent()).bold(true));
            }
            if let Some(k) = &self.key {
                t.hit(format!("{k}-{i}"), Rect::new(inner.x, y, inner.width, 1));
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Slider

/// A thin track: played `━` in the foreground, the knob `●`, the rest `─` in `track`; notches
/// `┼` on the rest and `╋` on the played part. Edges show with the cursor.
pub struct Slider {
    value: f32,
    notches: usize,
    width: u16,
    cursor: bool,
    key: Option<String>,
}

impl Slider {
    pub fn new(value: f32) -> Self {
        Slider { value: value.clamp(0.0, 1.0), notches: 0, width: 45, cursor: false, key: None }
    }
    /// `n` evenly spaced notches (and steps), ends included.
    pub fn notches(mut self, n: usize) -> Self {
        self.notches = n;
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
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
    /// The value at a column of a slider drawn at `area`.
    pub fn value_at(area: Rect, col: u16) -> f32 {
        let track = area.width.saturating_sub(2).max(2);
        (col.saturating_sub(area.x + 1) as f32 / (track - 1) as f32).clamp(0.0, 1.0)
    }
    /// Left/h and Right/l step by a notch (or 5%), Home/End jump; true when handled.
    pub fn handle_key(value: &mut f32, notches: usize, key: &crossterm::event::KeyEvent) -> bool {
        use crossterm::event::KeyCode::*;
        let step = if notches > 1 { 1.0 / (notches - 1) as f32 } else { 0.05 };
        let v = match key.code {
            Left | Char('h') => *value - step,
            Right | Char('l') => *value + step,
            Home => 0.0,
            End => 1.0,
            _ => return false,
        };
        *value = if notches > 1 { ((v / step).round() * step).clamp(0.0, 1.0) } else { v.clamp(0.0, 1.0) };
        true
    }
}

impl Draw for Slider {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let ring = if self.cursor { super::border_of(&t.shown().controls.hover.border) } else { None };
        edges(t, buf, area.x, area.y, area.width, ring);
        let w = area.width.saturating_sub(2);
        if w < 2 {
            return;
        }
        let knob = (self.value * (w - 1) as f32).round() as u16;
        let notch = |i: u16| self.notches > 1 && (0..self.notches).any(|n| ((n as f32 / (self.notches - 1) as f32) * (w - 1) as f32).round() as u16 == i);
        for i in 0..w {
            let x = area.x + 1 + i;
            if i < knob {
                put(t, buf, x, area.y, if notch(i) { "╋" } else { "━" }, t.fg());
            } else if i > knob {
                put(t, buf, x, area.y, if notch(i) { "┼" } else { "─" }, t.track());
            }
        }
        put(t, buf, area.x + 1 + knob, area.y, "●", t.fg());
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Checkbox, Radio, RadioGroup

/// `[✓] Label` (Radio: `(●) Label`): the box's brackets are its border, its three cells its fill;
/// the mark is accent. A description goes on the row under the label.
pub struct Checkbox {
    label: String,
    description: String,
    checked: bool,
    exclusive: bool,
    cursor: bool,
    disabled: bool,
    key: Option<String>,
}

impl Checkbox {
    pub fn new(label: impl Into<String>) -> Self {
        Checkbox { label: label.into(), description: String::new(), checked: false, exclusive: false, cursor: false, disabled: false, key: None }
    }
    pub fn description(mut self, d: impl Into<String>) -> Self {
        self.description = d.into();
        self
    }
    pub fn checked(mut self, on: bool) -> Self {
        self.checked = on;
        self
    }
    pub fn exclusive(mut self, on: bool) -> Self {
        self.exclusive = on;
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
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

/// A Checkbox with `exclusive`: `(●)`.
pub struct Radio;

impl Radio {
    #[allow(clippy::new_ret_no_self)]
    pub fn new(label: impl Into<String>) -> Checkbox {
        Checkbox::new(label).exclusive(true)
    }
}

/// The three-cell box at (x, y).
pub(crate) fn check_box(t: &Theme, buf: &mut Buffer, x: u16, y: u16, checked: bool, exclusive: bool, hot: bool, disabled: bool) {
    let c = &t.shown().controls;
    let (fill_c, border) = if hot && !disabled {
        (c.hover.fill, c.hover.border.color)
    } else if checked {
        (c.selected.fill, c.normal.border.color)
    } else {
        (c.normal.fill, c.normal.border.color)
    };
    let border = if disabled { border.times_alpha(0.5) } else { border };
    fill(t, buf, Rect::new(x, y, 3, 1), if disabled { fill_c.times_alpha(0.5) } else { fill_c });
    let (l, r) = if exclusive { ("(", ")") } else { ("[", "]") };
    put(t, buf, x, y, l, border);
    put(t, buf, x + 2, y, r, border);
    if checked {
        let mark = if exclusive { glyphs::DOT } else { glyphs::CHECK };
        let ink = if disabled { t.accent().times_alpha(0.5) } else { t.accent() };
        put(t, buf, x + 1, y, mark, Ink::new(ink).bold(true));
    }
}

impl Draw for Checkbox {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let w = 4 + width(&self.label).max(width(&self.description));
        (w, if self.description.is_empty() { 1 } else { 2 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        check_box(t, buf, area.x, area.y, self.checked, self.exclusive, self.cursor, self.disabled);
        let words = area.width.saturating_sub(4);
        let (ink, dim) = if self.disabled { (t.faint(), t.faint()) } else { (t.fg(), t.dim()) };
        text_fit(t, buf, area.x + 4, area.y, &self.label, words, ink);
        if !self.description.is_empty() && area.height > 1 {
            text_fit(t, buf, area.x + 4, area.y + 1, &self.description, words, dim);
        }
        if let Some(k) = &self.key
            && !self.disabled
        {
            t.hit(k.clone(), area);
        }
    }
}

/// Radios stacked, one chosen; Up/Down/Left/Right and j/k move the choice
/// ([`RadioGroup::handle_key`]).
pub struct RadioGroup {
    options: Vec<(String, String)>,
    value: String,
    cursor: Option<usize>,
    key: Option<String>,
}

impl RadioGroup {
    pub fn new(options: impl IntoIterator<Item = (impl Into<String>, impl Into<String>)>, value: impl Into<String>) -> Self {
        RadioGroup { options: options.into_iter().map(|(v, l)| (v.into(), l.into())).collect(), value: value.into(), cursor: None, key: None }
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    /// Radios keyed `<k>-<value>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
    /// Move `value` among `values` (arrows, j/k); stops at the ends; true when handled.
    pub fn handle_key(value: &mut String, values: &[&str], key: &crossterm::event::KeyEvent) -> bool {
        use crossterm::event::KeyCode::*;
        let i = values.iter().position(|v| v == value).unwrap_or(0);
        let next = match key.code {
            Down | Right | Char('j') | Char('l') => (i + 1).min(values.len().saturating_sub(1)),
            Up | Left | Char('k') | Char('h') => i.saturating_sub(1),
            _ => return false,
        };
        *value = values[next].to_string();
        true
    }
}

impl Draw for RadioGroup {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.options.iter().map(|(_, l)| 4 + width(l)).max().unwrap_or(0), self.options.len() as u16)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        for (i, (v, l)) in self.options.iter().enumerate() {
            let mut r = Radio::new(l.clone()).checked(*v == self.value).cursor(self.cursor == Some(i));
            if let Some(k) = &self.key {
                r = r.key(format!("{k}-{v}"));
            }
            r.draw(t, Rect::new(area.x, area.y + i as u16, area.width, 1).intersection(area), buf);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// ColorWell, SwatchStrip

/// A bordered well with the color inset: `▕███▏`, and a ring outside it with the cursor
/// (`▕▕███▏▏`): 7 columns.
pub struct ColorWell {
    color: Rgba,
    cursor: bool,
    key: Option<String>,
}

impl ColorWell {
    pub fn new(color: Rgba) -> Self {
        ColorWell { color, cursor: false, key: None }
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for ColorWell {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (7, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let c = &t.shown().controls;
        if self.cursor {
            edges(t, buf, area.x, area.y, 7, Some(c.hover.border.color));
        }
        edges(t, buf, area.x + 1, area.y, 5, super::border_of(&c.normal.border));
        for i in 0..3 {
            put(t, buf, area.x + 2 + i, area.y, "█", self.color);
        }
        if let Some(k) = &self.key {
            t.hit(k.clone(), area);
        }
    }
}

/// Color chips `██` one column apart; under the chosen one `▔▔` in accent, under the cursor's in
/// the hover border color (chosen wins). Two rows.
pub struct SwatchStrip {
    colors: Vec<Rgba>,
    chosen: Option<usize>,
    cursor: Option<usize>,
    key: Option<String>,
}

impl SwatchStrip {
    pub fn new(colors: impl IntoIterator<Item = Rgba>, chosen: Option<usize>) -> Self {
        SwatchStrip { colors: colors.into_iter().collect(), chosen, cursor: None, key: None }
    }
    pub fn cursor(mut self, i: Option<usize>) -> Self {
        self.cursor = i;
        self
    }
    /// Chips keyed `<k>-<i>`.
    pub fn key(mut self, k: impl Into<String>) -> Self {
        self.key = Some(k.into());
        self
    }
}

impl Draw for SwatchStrip {
    fn size(&self, _: &Theme) -> (u16, u16) {
        ((self.colors.len() as u16 * 3).saturating_sub(1), 2)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        for (i, c) in self.colors.iter().enumerate() {
            let x = area.x + i as u16 * 3;
            put(t, buf, x, area.y, "█", *c);
            put(t, buf, x + 1, area.y, "█", *c);
            let mark = if self.chosen == Some(i) {
                Some(t.accent())
            } else if self.cursor == Some(i) {
                Some(t.shown().controls.hover.border.color)
            } else {
                None
            };
            if let Some(m) = mark {
                put(t, buf, x, area.y + 1, "▔", m);
                put(t, buf, x + 1, area.y + 1, "▔", m);
            }
            if let Some(k) = &self.key {
                t.hit(format!("{k}-{i}"), Rect::new(x, area.y, 2, 2));
            }
        }
    }
}
