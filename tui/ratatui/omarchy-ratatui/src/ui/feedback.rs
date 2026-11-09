//! Levels, work in progress, emptiness and outcomes (spec/terminal.md, Feedback).

use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use super::controls::control;
use super::{Button, Look, Tone, glyphs, wrap};
use crate::paint::{Ink, clear, fill, frame, hline, put, text_center, text_fit, text_right};
use crate::{Draw, Theme};

// ---------------------------------------------------------------------------------------------
// Meter, ProgressBar

/// A determinate level: an optional header row (label, value text at the end in `textDim`), then
/// the bar: `round(value × w)` cells of `━` in the tone, the rest `━` in `track`; markers `╋` in
/// the fill's color at `round(f × (w − 1))`.
pub struct Meter {
    value: f32,
    tone: Tone,
    alarming: bool,
    markers: Vec<f32>,
    label: String,
    value_text: String,
    width: u16,
    segment: Option<(u16, u16)>,
}

impl Meter {
    pub fn new(value: f32) -> Self {
        Meter { value: value.clamp(0.0, 1.0), tone: Tone::Neutral, alarming: false, markers: Vec::new(), label: String::new(), value_text: String::new(), width: 28, segment: None }
    }
    pub fn tone(mut self, t: Tone) -> Self {
        self.tone = t;
        self
    }
    pub fn alarming(mut self, on: bool) -> Self {
        self.alarming = on;
        self
    }
    pub fn markers(mut self, m: impl IntoIterator<Item = f32>) -> Self {
        self.markers = m.into_iter().collect();
        self
    }
    pub fn label(mut self, l: impl Into<String>) -> Self {
        self.label = l.into();
        self
    }
    pub fn value_text(mut self, v: impl Into<String>) -> Self {
        self.value_text = v.into();
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.width = w;
        self
    }
    fn header(&self) -> bool {
        !self.label.is_empty() || !self.value_text.is_empty()
    }
}

impl Draw for Meter {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, if self.header() { 2 } else { 1 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut y = area.y;
        if self.header() {
            text_fit(t, buf, area.x, y, &self.label, area.width, t.fg());
            text_right(t, buf, area.right(), y, &self.value_text, t.dim());
            y += 1;
        }
        let w = area.width;
        let ink = if self.alarming { t.urgent() } else { self.tone.color(t, t.fg()) };
        hline(t, buf, area.x, y, w, "━", t.track());
        let (from, to) = self.segment.unwrap_or((0, (self.value * w as f32).round() as u16));
        for i in from..to.min(w) {
            put(t, buf, area.x + i, y, "━", ink);
        }
        for m in &self.markers {
            let i = (m.clamp(0.0, 1.0) * (w.saturating_sub(1)) as f32).round() as u16;
            put(t, buf, area.x + i, y, "╋", ink);
        }
    }
}

/// A Meter in accent by default, plus an indeterminate mode: a segment a quarter of the track
/// wide sweeping across every 1.4 s; still (reduced motion, snapshots) it sits at
/// `[round(3w/8), round(3w/8) + round(w/4))`.
pub struct ProgressBar {
    meter: Meter,
    indeterminate: bool,
}

impl ProgressBar {
    pub fn new(value: f32) -> Self {
        ProgressBar { meter: Meter::new(value).tone(Tone::INFO), indeterminate: false }
    }
    pub fn indeterminate() -> Self {
        ProgressBar { meter: Meter::new(0.0).tone(Tone::INFO), indeterminate: true }
    }
    pub fn tone(mut self, t: Tone) -> Self {
        self.meter = self.meter.tone(t);
        self
    }
    pub fn label(mut self, l: impl Into<String>) -> Self {
        self.meter = self.meter.label(l);
        self
    }
    pub fn value_text(mut self, v: impl Into<String>) -> Self {
        self.meter = self.meter.value_text(v);
        self
    }
    pub fn width(mut self, w: u16) -> Self {
        self.meter = self.meter.width(w);
        self
    }
    /// True while it animates (an app's [`crate::App::animating`]).
    pub fn animating(&self, t: &Theme) -> bool {
        self.indeterminate && !t.reduce_motion()
    }
}

impl Draw for ProgressBar {
    fn size(&self, t: &Theme) -> (u16, u16) {
        self.meter.size(t)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        if !self.indeterminate {
            return self.meter.draw(t, area, buf);
        }
        let w = area.width as f64;
        let seg = (w / 4.0).round();
        let start = if t.reduce_motion() {
            (3.0 * w / 8.0).round()
        } else {
            let p = (t.clock() % 1400) as f64 / 1400.0;
            let e = crate::cubic_bezier([0.42, 0.0, 0.58, 1.0], p);
            (-seg + (w + seg) * e).round()
        };
        let from = start.max(0.0) as u16;
        let to = (start + seg).clamp(0.0, w) as u16;
        let m = Meter { segment: Some((from, to)), ..Meter::new(0.0).tone(self.meter.tone).width(area.width).label(self.meter.label.clone()).value_text(self.meter.value_text.clone()) };
        m.draw(t, area, buf);
    }
}

// ---------------------------------------------------------------------------------------------
// Spinner

/// Busy. Turning: the braille frames `⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏`, one per 90 ms (a turn in 900 ms); still
/// (reduced motion, snapshots) the first. Pulse: the glyph (`󰦖`) breathing between half and full
/// opacity, 420 ms each way; still at half.
pub struct Spinner {
    pulse: bool,
    glyph: String,
    color: Option<omarchy_theme::Rgba>,
}

impl Default for Spinner {
    fn default() -> Self {
        Self::new()
    }
}

impl Spinner {
    pub const TURN_MS: u64 = 900;
    pub const PULSE_MS: u64 = 840;

    pub fn new() -> Self {
        Spinner { pulse: false, glyph: glyphs::LOADING.into(), color: None }
    }
    pub fn pulse(mut self, on: bool) -> Self {
        self.pulse = on;
        self
    }
    /// The pulsing glyph.
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn color(mut self, c: omarchy_theme::Rgba) -> Self {
        self.color = Some(c);
        self
    }
    /// The frame index (turning) at `clock` ms.
    pub fn frame(clock: u64) -> usize {
        ((clock % Self::TURN_MS) / 90) as usize
    }
    /// The opacity (pulsing) at `clock` ms: 0.5 → 1 → 0.5, eased in and out.
    pub fn opacity(clock: u64) -> f64 {
        let p = (clock % Self::PULSE_MS) as f64 / 420.0;
        let x = if p <= 1.0 { p } else { 2.0 - p };
        0.5 + 0.5 * crate::cubic_bezier([0.42, 0.0, 0.58, 1.0], x)
    }
}

impl Draw for Spinner {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (1, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let ink = self.color.unwrap_or(t.fg());
        let still = t.reduce_motion();
        if self.pulse {
            let o = if still { 0.5 } else { Spinner::opacity(t.clock()) };
            put(t, buf, area.x, area.y, &self.glyph, ink.with_alpha(o));
        } else {
            let f = if still { 0 } else { Spinner::frame(t.clock()) };
            put(t, buf, area.x, area.y, glyphs::SPINNER[f], ink);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// EmptyState

/// What a list or panel shows when it has nothing: a centered column of glyph, title, hint and a
/// bordered action Button, a blank row before the action. Panel: glyph and title `textDim`, hint
/// `textFaint`. Overlay: glyph `menu.selectedText` at 0.8, title foreground at 0.7, hint `textDim`.
pub struct EmptyState {
    title: String,
    glyph: String,
    hint: String,
    action: String,
    overlay: bool,
    cursor: bool,
    width: u16,
}

impl EmptyState {
    pub fn new(title: impl Into<String>) -> Self {
        EmptyState { title: title.into(), glyph: String::new(), hint: String::new(), action: String::new(), overlay: false, cursor: false, width: 44 }
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn hint(mut self, h: impl Into<String>) -> Self {
        self.hint = h.into();
        self
    }
    pub fn action(mut self, a: impl Into<String>) -> Self {
        self.action = a.into();
        self
    }
    pub fn overlay(mut self, on: bool) -> Self {
        self.overlay = on;
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
    fn lines(&self) -> (Vec<String>, Vec<String>) {
        let w = self.width;
        (wrap(&self.title, w), if self.hint.is_empty() { Vec::new() } else { wrap(&self.hint, w) })
    }
}

impl Draw for EmptyState {
    fn size(&self, _: &Theme) -> (u16, u16) {
        let (title, hint) = self.lines();
        let mut h = title.len() as u16 + hint.len() as u16;
        if !self.glyph.is_empty() {
            h += 1;
        }
        if !self.action.is_empty() {
            h += 2;
        }
        (self.width, h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let s = t.shown();
        let (glyph_ink, title_ink, hint_ink) = if self.overlay {
            (s.surfaces.menu.selected_text.with_alpha(0.8), t.fg().with_alpha(0.7), t.dim())
        } else {
            (t.dim(), t.dim(), t.faint())
        };
        let mut y = area.y;
        if !self.glyph.is_empty() {
            text_center(t, buf, area.x, y, area.width, &self.glyph, glyph_ink);
            y += 1;
        }
        let (title, hint) = self.lines();
        for l in title {
            text_center(t, buf, area.x, y, area.width, &l, title_ink);
            y += 1;
        }
        for l in hint {
            text_center(t, buf, area.x, y, area.width, &l, hint_ink);
            y += 1;
        }
        if !self.action.is_empty() {
            let b = Button::new(self.action.clone()).bordered(true).cursor(self.cursor).key("empty-action");
            let bw = b.size(t).0;
            b.draw(t, Rect::new(area.x + (area.width.saturating_sub(bw)) / 2, y + 1, bw, 1), buf);
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Banner

/// An inline message, one row: the control frame with its edges in the tone, the normal fill,
/// the tone glyph, the text, then an optional bordered action Button and a dismiss `󰅖`.
pub struct Banner {
    text: String,
    tone: Tone,
    glyph: String,
    action: String,
    dismissible: bool,
    cursor: bool,
    width: u16,
}

impl Banner {
    pub fn new(text: impl Into<String>) -> Self {
        Banner { text: text.into(), tone: Tone::INFO, glyph: String::new(), action: String::new(), dismissible: false, cursor: false, width: 92 }
    }
    pub fn tone(mut self, t: Tone) -> Self {
        self.tone = t;
        self
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn action(mut self, a: impl Into<String>) -> Self {
        self.action = a.into();
        self
    }
    pub fn dismissible(mut self, on: bool) -> Self {
        self.dismissible = on;
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
    fn tone_glyph(&self) -> &str {
        if !self.glyph.is_empty() {
            return &self.glyph;
        }
        match self.tone {
            Tone::Success => "\u{f012c}",
            Tone::Warning => "\u{f002a}",
            Tone::Urgent => "\u{f015a}",
            _ => "\u{f02fd}",
        }
    }
}

impl Draw for Banner {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let tone = if self.tone == Tone::Neutral { Tone::INFO } else { self.tone };
        let ink = tone.color(t, t.accent());
        control(t, buf, area.x, area.y, area.width, &Look { fill: t.shown().controls.normal.fill, border: Some(ink), text: t.fg(), bold: false });
        put(t, buf, area.x + 2, area.y, self.tone_glyph(), ink);
        let mut right = area.right().saturating_sub(2);
        if self.dismissible {
            right = right.saturating_sub(1);
            put(t, buf, right, area.y, glyphs::CLOSE, t.dim());
            t.hit("banner-dismiss", Rect::new(right, area.y, 1, 1));
            right = right.saturating_sub(1);
        }
        if !self.action.is_empty() {
            let b = Button::new(self.action.clone()).bordered(true).cursor(self.cursor).key("banner-action");
            let bw = b.size(t).0;
            right = right.saturating_sub(bw);
            b.draw(t, Rect::new(right, area.y, bw, 1), buf);
            right = right.saturating_sub(1);
        }
        text_fit(t, buf, area.x + 4, area.y, &self.text, right.saturating_sub(area.x + 4), t.fg());
    }
}

// ---------------------------------------------------------------------------------------------
// Toast

/// A transient message on the `notifications` surface: a box in its border color (accent), the
/// glyph, the bold title over the text (foreground darker 1.15), an action Button at the end of
/// the first row; the countdown is the bottom edge: its first `round(countdown × (w − 2))` cells
/// `━` in `notifications.countdown`. Four rows.
pub struct Toast {
    title: String,
    text: String,
    glyph: String,
    action: String,
    countdown: Option<f32>,
    cursor: bool,
    width: u16,
}

impl Toast {
    pub fn new(title: impl Into<String>) -> Self {
        Toast { title: title.into(), text: String::new(), glyph: String::new(), action: String::new(), countdown: None, cursor: false, width: 45 }
    }
    pub fn text(mut self, s: impl Into<String>) -> Self {
        self.text = s.into();
        self
    }
    pub fn glyph(mut self, g: impl Into<String>) -> Self {
        self.glyph = g.into();
        self
    }
    pub fn action(mut self, a: impl Into<String>) -> Self {
        self.action = a.into();
        self
    }
    pub fn countdown(mut self, c: f32) -> Self {
        self.countdown = Some(c);
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
}

impl Draw for Toast {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.width, if self.text.is_empty() { 3 } else { 4 })
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        clear(buf, area);
        let n = &t.shown().surfaces.notifications;
        let inner = frame(t, buf, area, n.base.border.color, None);
        if self.cursor {
            fill(t, buf, inner, t.shown().controls.hover.fill);
        }
        let mut x = inner.x + 1;
        if !self.glyph.is_empty() {
            put(t, buf, x, inner.y, &self.glyph, n.base.text);
            x += 2;
        }
        let mut right = inner.right().saturating_sub(1);
        if !self.action.is_empty() {
            let b = Button::new(self.action.clone()).bordered(true).key("toast-action");
            let bw = b.size(t).0;
            right = right.saturating_sub(bw);
            b.draw(t, Rect::new(right, inner.y, bw, 1), buf);
            right = right.saturating_sub(1);
        }
        text_fit(t, buf, x, inner.y, &self.title, right.saturating_sub(x), Ink::new(n.base.text).bold(true));
        if !self.text.is_empty() {
            text_fit(t, buf, x, inner.y + 1, &self.text, right.saturating_sub(x), n.base.text.darker(1.15));
        }
        if let Some(c) = self.countdown
            && c >= 0.0
        {
            let w = area.width.saturating_sub(2);
            let k = (c.clamp(0.0, 1.0) * w as f32).round() as u16;
            hline(t, buf, area.x + 1, area.bottom() - 1, k, "━", n.countdown);
        }
    }
}
