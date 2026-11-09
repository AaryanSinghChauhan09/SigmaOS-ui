//! The `timeline` section (spec/gallery.md).

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Clip, CropFrame, DragHandle, DropIndicator, Edge, Scrubber, Timeline, Trim, label};
use omarchy_gpui::{Paint, color};

use crate::Gallery;

/// What this section keeps between frames.
pub struct State {
    pub position: f32,
    pub tracks: Vec<Vec<Clip>>,
    pub selected: Option<(usize, usize)>,
    pub zoom: f32,
    pub start: f32,
    pub scrub: f32,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State {
            position: 12.0,
            tracks: vec![
                vec![Clip::new(0.0, 7.5, "Intro"), Clip::new(8.0, 18.0, "Demo"), Clip::new(18.5, 30.0, "Outro")],
                vec![Clip::new(2.0, 12.0, "Voiceover"), Clip::new(14.0, 26.0, "Music")],
            ],
            selected: Some((0, 1)),
            zoom: 1.0,
            start: 0.0,
            scrub: 42.0,
        }
    }
}

pub fn build(g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let tk = t.tokens();
    let sp = &tk.spacing;
    let s = &g.more.timeline;
    let mut timeline = Timeline::new(30.0).position(s.position).selected(s.selected).zoom(s.zoom, s.start).size(720.0, 160.0);
    for track in &s.tracks {
        timeline = timeline.track(track.iter().cloned());
    }
    let timeline = timeline
        .on_seek(cx.listener(|g: &mut Gallery, at: &f32, _, cx| {
            g.more.timeline.position = *at;
            cx.notify();
        }))
        .on_select(cx.listener(|g: &mut Gallery, sel: &(usize, usize), _, cx| {
            g.more.timeline.selected = Some(*sel);
            cx.notify();
        }))
        .on_trim(cx.listener(|g: &mut Gallery, tr: &Trim, _, cx| {
            let c = &mut g.more.timeline.tracks[tr.track][tr.clip];
            if tr.edge == Edge::Left { c.start = tr.time.min(c.end - 0.5) } else { c.end = tr.time.max(c.start + 0.5) }
            cx.notify();
        }))
        .on_view(cx.listener(|g: &mut Gallery, (zoom, start): &(f32, f32), _, cx| {
            g.more.timeline.zoom = *zoom;
            g.more.timeline.start = *start;
            cx.notify();
        }));

    // Rows reordered by their grips: the second under the cursor, a drop slot between the third
    // and fourth.
    let row_h = sp.popup_row_height as f32;
    let mut rows = div().flex().flex_col().w(px(360.0));
    for (i, name) in ["Workspaces", "Clock", "Battery", "Network"].into_iter().enumerate() {
        let cursor = i == 1;
        let mut r = div()
            .flex()
            .flex_row()
            .w(px(360.0))
            .h(px(row_h))
            .items_center()
            .pl(px(sp.sm as f32))
            .gap(px(sp.sm as f32))
            .rounded(t.radius())
            .child(DragHandle::new(SharedString::from(format!("grip-{name}"))).cursor(cursor))
            .child(label(&t, name, tk.font.body, false, t.fg()));
        if cursor {
            r = r.bg(color(tk.controls.hover.fill));
        }
        rows = rows.child(r);
    }
    let line = DropIndicator::thickness(&t);
    let list = div()
        .relative()
        .flex_shrink_0()
        .w(px(360.0))
        .h(px(row_h * 4.0))
        .child(rows)
        .child(div().absolute().left_0().top(px(row_h * 3.0 - (line / 2.0).round())).child(DropIndicator::new(360.0)));
    let crop = div().flex_shrink_0().w(px(240.0)).h(px(140.0)).bg(color(tk.controls.normal.fill)).child(CropFrame::new(240.0, 140.0));

    vec![
        timeline.into_any_element(),
        Scrubber::new(s.scrub, 130.0)
            .width(360.0)
            .on_seek(cx.listener(|g: &mut Gallery, at: &f32, _, cx| {
                g.more.timeline.scrub = *at;
                cx.notify();
            }))
            .into_any_element(),
        div().flex().flex_row().items_start().gap(px(sp.huge as f32)).child(list).child(crop).into_any_element(),
    ]
}
