//! SplitView: panes side by side (or stacked) with a draggable handle between each pair: a
//! `md`-wide strip holding a hairline in the track color, lit with the hover glass under the
//! pointer, while dragged, or with `cursor`.

use std::cell::Cell;
use std::rc::Rc;

use gpui::prelude::*;
use gpui::{AnyElement, App, CursorStyle, MouseButton, MouseMoveEvent, MouseUpEvent, SharedString, Window, canvas, div, px};
use omarchy_theme::Rgba;

use crate::ui::{Handler, keyed_state};
use crate::{Paint, color};

/// One pane: its content and either a size (the pane a handle resizes) or a share of what the
/// sized panes leave.
pub struct SplitPane {
    content: AnyElement,
    size: Option<f32>,
    min: f32,
    max: f32,
    visible: bool,
}

impl SplitPane {
    pub fn new(content: impl IntoElement) -> SplitPane {
        SplitPane { content: content.into_any_element(), size: None, min: 0.0, max: f32::INFINITY, visible: true }
    }
    /// A size along the split (the width when horizontal); the handle beside it changes it.
    pub fn size(mut self, s: f32) -> Self {
        self.size = Some(s);
        self
    }
    pub fn min(mut self, s: f32) -> Self {
        self.min = s;
        self
    }
    pub fn max(mut self, s: f32) -> Self {
        self.max = s;
        self
    }
    /// Collapse the pane (and its handle) when false.
    pub fn visible(mut self, on: bool) -> Self {
        self.visible = on;
        self
    }
}

/// A drag in progress: which handle, where the pointer went down, the pane's size then.
#[derive(Default)]
struct Drag {
    active: Rc<Cell<Option<(usize, f32, f32)>>>,
}

/// Panes with draggable handles between them. The app stores the sizes: `on_resize` is called
/// with `(pane index, new size)` while a handle is dragged, clamped to the pane's min and max.
#[derive(IntoElement)]
pub struct SplitView {
    id: SharedString,
    panes: Vec<SplitPane>,
    vertical: bool,
    cursor: bool,
    on_resize: Option<Handler<(usize, f32)>>,
}

impl SplitView {
    pub fn new(id: impl Into<SharedString>) -> SplitView {
        SplitView { id: id.into(), panes: Vec::new(), vertical: false, cursor: false, on_resize: None }
    }
    pub fn pane(mut self, p: SplitPane) -> Self {
        self.panes.push(p);
        self
    }
    /// Stack the panes top to bottom instead of side by side.
    pub fn vertical(mut self, on: bool) -> Self {
        self.vertical = on;
        self
    }
    /// Force the handles' hot look.
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    pub fn on_resize(mut self, f: impl Fn(&(usize, f32), &mut Window, &mut App) + 'static) -> Self {
        self.on_resize = Some(Rc::new(f));
        self
    }
}

impl RenderOnce for SplitView {
    fn render(self, window: &mut Window, cx: &mut App) -> impl IntoElement {
        let t = Paint::of(cx);
        let tk = t.tokens();
        let thick = tk.spacing.md as f32;
        let hair = tk.spacing.hairline as f32;
        let hover = color(tk.controls.hover.fill);
        let across = !self.vertical;
        let id: gpui::ElementId = self.id.clone().into();
        let drag = keyed_state(&id, "split", window, cx, Drag::default).read(cx).active.clone();

        let visible: Vec<(usize, SplitPane)> = self.panes.into_iter().enumerate().filter(|(_, p)| p.visible).collect();
        // Which pane each handle resizes: the sized one beside it, the leading one first.
        let beside = |i: usize| -> Option<(usize, f32, f32, f32, f32)> {
            let (ai, a) = &visible[i];
            let (bi, b) = &visible[i + 1];
            match (a.size, b.size) {
                (Some(s), _) => Some((*ai, s, 1.0, a.min, a.max)),
                (None, Some(s)) => Some((*bi, s, -1.0, b.min, b.max)),
                _ => None,
            }
        };
        let handles: Vec<_> = (0..visible.len().saturating_sub(1)).map(beside).collect();

        let mut out = div().id(id).relative().flex().size_full();
        out = if across { out.flex_row() } else { out.flex_col() };
        for (k, (_, p)) in visible.into_iter().enumerate() {
            if k > 0 {
                let n = k - 1;
                let at = ((thick - hair) / 2.0).floor();
                let line = div().absolute().bg(t.track());
                let line = if across {
                    line.top_0().bottom_0().left(px(at)).w(px(hair))
                } else {
                    line.left_0().right_0().top(px(at)).h(px(hair))
                };
                let dragging = drag.get().is_some_and(|(h, _, _)| h == n);
                let hot = self.cursor || dragging;
                let selector = format!("{}-handle-{n}", self.id);
                let mut h = div()
                    .id(n)
                    .debug_selector(|| selector)
                    .relative()
                    .flex_shrink_0()
                    .rounded(t.radius_small())
                    .bg(if hot { hover } else { color(Rgba::TRANSPARENT) })
                    .hover(move |s| s.bg(hover))
                    .cursor(if across { CursorStyle::ResizeColumn } else { CursorStyle::ResizeRow })
                    .child(line);
                h = if across { h.w(px(thick)).h_full() } else { h.h(px(thick)).w_full() };
                if let Some((index, size, sign, min, max)) = handles[n] {
                    let d = drag.clone();
                    h = h.on_mouse_down(MouseButton::Left, move |ev, window, cx| {
                        let pos = if across { ev.position.x } else { ev.position.y };
                        d.set(Some((n, f32::from(pos), size)));
                        cx.stop_propagation();
                        window.refresh();
                    });
                    // Follow the drag window-wide, so it keeps going off the handle.
                    let (d1, d2) = (drag.clone(), drag.clone());
                    let f = self.on_resize.clone();
                    h = h.child(
                        canvas(
                            |_, _, _| (),
                            move |_, _, window, _| {
                                let (d1, f) = (d1.clone(), f.clone());
                                window.on_mouse_event(move |ev: &MouseMoveEvent, _, window, cx| {
                                    let Some((hn, start, from)) = d1.get() else { return };
                                    if hn != n || ev.pressed_button != Some(MouseButton::Left) {
                                        return;
                                    }
                                    let pos = f32::from(if across { ev.position.x } else { ev.position.y });
                                    let to = (from + sign * (pos - start)).clamp(min, max.max(min));
                                    if let Some(f) = &f {
                                        f(&(index, to), window, cx);
                                    }
                                });
                                let d2 = d2.clone();
                                window.on_mouse_event(move |_: &MouseUpEvent, _, window, _| {
                                    if d2.get().is_some_and(|(hn, _, _)| hn == n) {
                                        d2.set(None);
                                        window.refresh();
                                    }
                                });
                            },
                        )
                        .absolute()
                        .size_0(),
                    );
                }
                out = out.child(h);
            }
            let mut pane = div().relative().overflow_hidden().flex().child(p.content);
            pane = match (p.size, across) {
                (Some(s), true) => pane.w(px(s.clamp(p.min, p.max.max(p.min)))).h_full().flex_shrink_0(),
                (Some(s), false) => pane.h(px(s.clamp(p.min, p.max.max(p.min)))).w_full().flex_shrink_0(),
                (None, true) => pane.flex_1().min_w_0().h_full(),
                (None, false) => pane.flex_1().min_h_0().w_full(),
            };
            out = out.child(pane);
        }
        out
    }
}
