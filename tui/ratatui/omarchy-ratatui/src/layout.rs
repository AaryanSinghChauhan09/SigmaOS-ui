//! Columns, rows and boxes of [`Draw`] components, at their natural sizes: enough to lay out a
//! panel or a gallery section without Ratatui's constraint solver. (Ratatui's `Layout` works too:
//! draw each component with [`Draw::with`] into the `Rect` it gives.)

use ratatui::buffer::Buffer;
use ratatui::layout::Rect;

use crate::paint::{self, Ink};
use crate::{Draw, Theme};

/// Children top to bottom, `gap` rows apart, each at its natural size (or the column's width,
/// [`Col::stretch`]).
#[derive(Default)]
pub struct Col {
    gap: u16,
    width: Option<u16>,
    items: Vec<(Box<dyn Draw>, bool)>,
}

pub fn col(gap: u16) -> Col {
    Col { gap, ..Default::default() }
}

impl Col {
    pub fn child(mut self, d: impl Draw + 'static) -> Self {
        self.items.push((Box::new(d), false));
        self
    }
    /// A child given the column's whole width.
    pub fn stretch(mut self, d: impl Draw + 'static) -> Self {
        self.items.push((Box::new(d), true));
        self
    }
    pub fn children(mut self, ds: impl IntoIterator<Item = Box<dyn Draw>>) -> Self {
        self.items.extend(ds.into_iter().map(|d| (d, false)));
        self
    }
    /// A fixed width (else the widest child's).
    pub fn width(mut self, w: u16) -> Self {
        self.width = Some(w);
        self
    }
}

impl Draw for Col {
    fn size(&self, t: &Theme) -> (u16, u16) {
        let mut w = 0;
        let mut h = 0;
        for (i, (d, _)) in self.items.iter().enumerate() {
            let (cw, ch) = d.size(t);
            w = w.max(cw);
            h += ch + if i > 0 { self.gap } else { 0 };
        }
        (self.width.unwrap_or(w), h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let mut y = area.y;
        for (d, stretch) in &self.items {
            let (cw, ch) = d.size(t);
            let w = if *stretch { area.width } else { cw.min(area.width) };
            let r = Rect { x: area.x, y, width: w, height: ch }.intersection(area);
            if r.height > 0 {
                d.draw(t, Rect { height: ch.min(area.bottom().saturating_sub(y)), ..r }, buf);
            }
            y = y.saturating_add(ch + self.gap);
        }
    }
}

/// How a row places children shorter than itself.
#[derive(Clone, Copy, Default, PartialEq, Eq)]
pub enum Align {
    #[default]
    Top,
    Center,
}

/// Children left to right, `gap` columns apart, top-aligned (or centered) at their natural sizes.
#[derive(Default)]
pub struct Row {
    gap: u16,
    align: Align,
    items: Vec<Box<dyn Draw>>,
}

pub fn row(gap: u16) -> Row {
    Row { gap, ..Default::default() }
}

impl Row {
    pub fn child(mut self, d: impl Draw + 'static) -> Self {
        self.items.push(Box::new(d));
        self
    }
    pub fn children(mut self, ds: impl IntoIterator<Item = Box<dyn Draw>>) -> Self {
        self.items.extend(ds);
        self
    }
    pub fn center(mut self) -> Self {
        self.align = Align::Center;
        self
    }
}

impl Draw for Row {
    fn size(&self, t: &Theme) -> (u16, u16) {
        let mut w = 0;
        let mut h = 0;
        for (i, d) in self.items.iter().enumerate() {
            let (cw, ch) = d.size(t);
            w += cw + if i > 0 { self.gap } else { 0 };
            h = h.max(ch);
        }
        (w, h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let (_, h) = self.size(t);
        let mut x = area.x;
        for d in &self.items {
            let (cw, ch) = d.size(t);
            let dy = if self.align == Align::Center { h.saturating_sub(ch) / 2 } else { 0 };
            let r = Rect { x, y: area.y + dy, width: cw, height: ch }.intersection(area);
            if r.width > 0 && r.height > 0 {
                d.draw(t, r, buf);
            }
            x = x.saturating_add(cw + self.gap);
        }
    }
}

/// A box of a fixed size holding one child at its natural size: top-left, or centered.
pub struct Boxed {
    w: u16,
    h: u16,
    center: bool,
    inner: Box<dyn Draw>,
}

/// `inner` in a `w` × `h` box, at the top left.
pub fn sized(w: u16, h: u16, inner: impl Draw + 'static) -> Boxed {
    Boxed { w, h, center: false, inner: Box::new(inner) }
}

/// `inner` centered in a `w` × `h` box.
pub fn centered(w: u16, h: u16, inner: impl Draw + 'static) -> Boxed {
    Boxed { w, h, center: true, inner: Box::new(inner) }
}

/// `inner` given the whole `w` × `h` box (it stretches to fill).
pub fn filled(w: u16, h: u16, inner: impl Draw + 'static) -> Fill {
    Fill { w, h, inner: Box::new(inner) }
}

impl Draw for Boxed {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.w, self.h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        let (cw, ch) = self.inner.size(t);
        let (cw, ch) = (cw.min(area.width), ch.min(area.height));
        let (dx, dy) = if self.center { ((area.width - cw) / 2, (area.height - ch) / 2) } else { (0, 0) };
        self.inner.draw(t, Rect { x: area.x + dx, y: area.y + dy, width: cw, height: ch }, buf);
    }
}

/// See [`filled`].
pub struct Fill {
    w: u16,
    h: u16,
    inner: Box<dyn Draw>,
}

impl Draw for Fill {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.w, self.h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        self.inner.draw(t, area, buf);
    }
}

/// Empty space.
pub struct Blank(pub u16, pub u16);

impl Draw for Blank {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.0, self.1)
    }
    fn draw(&self, _: &Theme, _: Rect, _: &mut Buffer) {}
}

/// One line of text in an ink, elided to the area.
pub struct Label {
    text: String,
    ink: Box<dyn Fn(&Theme) -> Ink>,
}

/// A line of text; `ink` picks its color from the theme at draw time (so it fades).
pub fn label(text: impl Into<String>, ink: impl Fn(&Theme) -> Ink + 'static) -> Label {
    Label { text: text.into(), ink: Box::new(ink) }
}

impl Draw for Label {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (paint::width(&self.text), 1)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        paint::text_fit(t, buf, area.x, area.y, &self.text, area.width, (self.ink)(t));
    }
}

/// Anything drawn by a closure, at a fixed size: for one-off specimens.
pub struct Custom<F: Fn(&Theme, Rect, &mut Buffer)> {
    w: u16,
    h: u16,
    f: F,
}

pub fn custom<F: Fn(&Theme, Rect, &mut Buffer)>(w: u16, h: u16, f: F) -> Custom<F> {
    Custom { w, h, f }
}

impl<F: Fn(&Theme, Rect, &mut Buffer)> Draw for Custom<F> {
    fn size(&self, _: &Theme) -> (u16, u16) {
        (self.w, self.h)
    }
    fn draw(&self, t: &Theme, area: Rect, buf: &mut Buffer) {
        (self.f)(t, area, buf)
    }
}
