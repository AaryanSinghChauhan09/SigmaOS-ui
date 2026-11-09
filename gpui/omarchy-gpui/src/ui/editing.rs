//! Editing (spec/components/editing.md): Timeline, Scrubber, TrimHandle, CropFrame, DragHandle,
//! DropIndicator, ColorPicker. Same names, anatomy and metrics as the Qt files of the same names.

mod color;
mod handles;
mod timeline;

pub use color::{ColorPicker, Hsva};
pub use handles::{CropFrame, DragHandle, DropIndicator, Edge, TrimHandle};
pub use timeline::{Clip, Scrubber, Timeline, TimelineView, Trim};

use std::cell::Cell;
use std::rc::Rc;

use gpui::prelude::*;
use gpui::{App, Bounds, Div, ElementId, MouseButton, MouseMoveEvent, MouseUpEvent, Pixels, Point, Stateful, TextRun, Window, canvas, px};
use omarchy_theme::Rgba;

use super::keyed_state;
use crate::Paint;

/// Where a pointer gesture on an editing part is.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DragPhase {
    Down,
    Move,
    Up,
}

/// A pointer gesture on an editing part: the phase, the pointer in the part's own coordinates,
/// the travel since the press, the part's size, and `data`, a value kept from Down to Up (where a
/// handler keeps the value it started from).
#[derive(Clone, Debug)]
pub struct HandleDrag {
    pub phase: DragPhase,
    pub x: f32,
    pub y: f32,
    pub dx: f32,
    pub dy: f32,
    pub w: f32,
    pub h: f32,
    pub data: Rc<Cell<f32>>,
}

#[derive(Default)]
pub(crate) struct DragState {
    bounds: Rc<Cell<Option<Bounds<Pixels>>>>,
    start: Rc<Cell<Option<Point<Pixels>>>>,
    last: Rc<Cell<Option<Point<Pixels>>>>,
    data: Rc<Cell<f32>>,
}

pub(crate) type DragFn = Rc<dyn Fn(&HandleDrag, &mut Window, &mut App)>;

/// True while the part `id` is being dragged.
pub(crate) fn dragging(id: &ElementId, window: &mut Window, cx: &mut App) -> bool {
    keyed_state(id, "drag", window, cx, DragState::default).read(cx).start.get().is_some()
}

/// Where the part `id` was laid out last frame (recorded by [`follow_drag`]).
pub(crate) fn drag_bounds(id: &ElementId, window: &mut Window, cx: &mut App) -> Rc<Cell<Option<Bounds<Pixels>>>> {
    keyed_state(id, "drag", window, cx, DragState::default).read(cx).bounds.clone()
}

/// Follow a left-button drag that starts on `e` window-wide (the pointer may leave it): `f` sees
/// Down, every Move while the button is held, and Up. With `stop`, the press doesn't reach the
/// parts under `e` (a clip over a lane that seeks).
pub(crate) fn follow_drag(e: Stateful<Div>, id: &ElementId, stop: bool, f: DragFn, window: &mut Window, cx: &mut App) -> Stateful<Div> {
    let st = keyed_state(id, "drag", window, cx, DragState::default);
    let (bounds, start, last, data) = {
        let s = st.read(cx);
        (s.bounds.clone(), s.start.clone(), s.last.clone(), s.data.clone())
    };
    let event = move |phase: DragPhase, p: Point<Pixels>, b: Bounds<Pixels>, origin: Point<Pixels>, data: &Rc<Cell<f32>>| HandleDrag {
        phase,
        x: f32::from(p.x - b.left()),
        y: f32::from(p.y - b.top()),
        dx: f32::from(p.x - origin.x),
        dy: f32::from(p.y - origin.y),
        w: f32::from(b.size.width),
        h: f32::from(b.size.height),
        data: data.clone(),
    };
    let (b1, s1, l1, d1, f1) = (bounds.clone(), start.clone(), last.clone(), data.clone(), f.clone());
    let e = e.on_mouse_down(MouseButton::Left, move |ev, window, cx| {
        let Some(b) = b1.get() else { return };
        s1.set(Some(ev.position));
        l1.set(Some(ev.position));
        f1(&event(DragPhase::Down, ev.position, b, ev.position, &d1), window, cx);
        if stop {
            cx.stop_propagation();
        }
    });
    let record = bounds.clone();
    e.child(
        canvas(
            move |b, _, _| record.set(Some(b)),
            move |_, _, window, _| {
                let (b2, s2, l2, d2, f2) = (bounds.clone(), start.clone(), last.clone(), data.clone(), f.clone());
                window.on_mouse_event(move |ev: &MouseMoveEvent, _, window, cx| {
                    // Moves that don't move (a repeat at the same point) aren't reported.
                    if let (Some(origin), Some(b)) = (s2.get(), b2.get())
                        && ev.pressed_button == Some(MouseButton::Left)
                        && l2.replace(Some(ev.position)) != Some(ev.position)
                    {
                        f2(&event(DragPhase::Move, ev.position, b, origin, &d2), window, cx);
                    }
                });
                let (b3, s3, d3, f3) = (bounds.clone(), start.clone(), data.clone(), f.clone());
                window.on_mouse_event(move |ev: &MouseUpEvent, _, window, cx| {
                    if let (Some(origin), Some(b)) = (s3.take(), b3.get()) {
                        f3(&event(DragPhase::Up, ev.position, b, origin, &d3), window, cx);
                        window.refresh();
                    }
                });
            },
        )
        .absolute()
        .top_0()
        .left_0()
        .size_full(),
    )
}

/// The width of `s` set in the Omarchy font at `size` px.
pub(crate) fn text_width(window: &Window, t: &Paint, s: &str, size: i64) -> f32 {
    let run = TextRun { len: s.len(), font: gpui::font(t.family()), color: t.fg(), background_color: None, underline: None, strikethrough: None };
    f32::from(window.text_system().shape_line(s.to_string().into(), px(size as f32), &[run], None).width)
}

/// The hot look of an accent part: a quarter of the foreground mixed in (Qt's
/// `Qt.tint(accent, alpha(foreground, 0.25))`).
pub(crate) fn hot_accent(t: &Paint, base: Rgba) -> Rgba {
    base.mix(t.tokens().palette.foreground, 0.25)
}

/// The group the editing parts' hot layers follow the pointer with.
pub(crate) const HOT: &str = "omarchy-editing";
