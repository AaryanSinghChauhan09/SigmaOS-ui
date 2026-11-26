//! The `timeline` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Clip, CropFrame, DragHandle, DropIndicator, Edge, Scrubber, Timeline};
use rune::{El, col, el, row, text};

use crate::State;

pub struct Editing {
    pub position: f32,
    pub tracks: Vec<Vec<Clip>>,
    pub selected: Option<(usize, usize)>,
    pub zoom: f32,
    pub start: f32,
}

pub fn init() -> Editing {
    Editing {
        position: 12.0,
        tracks: vec![
            vec![
                Clip::new(0.0, 7.5, "Intro"),
                Clip::new(8.0, 18.0, "Demo"),
                Clip::new(18.5, 30.0, "Outro"),
            ],
            vec![
                Clip::new(2.0, 12.0, "Voiceover"),
                Clip::new(14.0, 26.0, "Music"),
            ],
        ],
        selected: Some((0, 1)),
        zoom: 1.0,
        start: 0.0,
    }
}

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let e = &s.timeline;
    let mut timeline = Timeline::new(30.0)
        .position(e.position)
        .selected(e.selected)
        .zoom(e.zoom, e.start)
        .size(720.0, 160.0);
    for track in &e.tracks {
        timeline = timeline.track(track.iter().cloned());
    }
    let timeline = timeline
        .on_seek(|s: &mut State, at| s.timeline.position = at)
        .on_select(|s: &mut State, track, clip| s.timeline.selected = Some((track, clip)))
        .on_trim(|s: &mut State, track, clip, edge, at| {
            let c = &mut s.timeline.tracks[track][clip];
            if edge == Edge::Left {
                c.start = at.min(c.end - 0.5)
            } else {
                c.end = at.max(c.start + 0.5)
            }
        })
        .on_view(|s: &mut State, zoom, start| {
            s.timeline.zoom = zoom;
            s.timeline.start = start;
        })
        .build(t);

    // Rows reordered by their grips: the second under the cursor, a drop slot between the third
    // and fourth.
    let row_h = sp.popup_row_height as f32;
    let body = t.text_style(t.tokens().font.body);
    let mut rows = col().w(360.0);
    for (i, label) in ["Workspaces", "Clock", "Battery", "Network"]
        .into_iter()
        .enumerate()
    {
        let cursor = i == 1;
        let mut r = row()
            .w(360.0)
            .h(row_h)
            .items_center()
            .pl(sp.sm as f32)
            .gap(sp.sm as f32)
            .radius(t.radius())
            .child(DragHandle::new().cursor(cursor).build(t))
            .child(text(label).font(body).color(t.fg()).single_line());
        if cursor {
            r = r.fill(omarchy_rune::color(t.tokens().controls.hover.fill));
        }
        rows = rows.child(r);
    }
    let line = DropIndicator::thickness(t);
    let list = el().w(360.0).h(row_h * 4.0).child(rows).child(
        DropIndicator::new(360.0)
            .build(t)
            .absolute(0.0, row_h * 3.0 - (line / 2.0).round()),
    );
    let crop = el()
        .w(240.0)
        .h(140.0)
        .fill(omarchy_rune::color(t.tokens().controls.normal.fill))
        .child(CropFrame::new(240.0, 140.0).build(t));

    vec![
        timeline,
        Scrubber::new(42.0, 130.0).width(360.0).build(t),
        row()
            .items_start()
            .gap(sp.huge as f32)
            .child(list)
            .child(crop),
    ]
}
