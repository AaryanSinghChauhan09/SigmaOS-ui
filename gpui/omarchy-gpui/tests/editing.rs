//! The editing parts driven by keys, clicks and drags, as the Rune gallery's
//! editing_selects_and_picks drives Rune's and the Qt files' Keys handlers say (Timeline.qml,
//! Scrubber.qml, TrimHandle.qml, DragHandle.qml, ColorPicker.qml).

mod common;

use common::{focus_first, host};
use gpui::{IntoElement, Modifiers, MouseButton, ParentElement, TestAppContext, div, point, px};
use omarchy_gpui::ui::{Clip, ColorPicker, CropFrame, DragHandle, DragPhase, Edge, Hsva, Scrubber, Timeline, Trim};

/// The gallery's timeline: 720 × 160 at the window's origin, 30 s, so 24 points a second.
#[derive(Clone)]
struct Edit {
    position: f32,
    tracks: Vec<Vec<Clip>>,
    selected: Option<(usize, usize)>,
}

fn edit() -> Edit {
    Edit {
        position: 12.0,
        tracks: vec![
            vec![Clip::new(0.0, 7.5, "Intro"), Clip::new(8.0, 18.0, "Demo"), Clip::new(18.5, 30.0, "Outro")],
            vec![Clip::new(2.0, 12.0, "Voiceover"), Clip::new(14.0, 26.0, "Music")],
        ],
        selected: Some((0, 1)),
    }
}

fn timeline(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<Edit>>, &mut gpui::VisualTestContext) {
    host(cx, edit(), |h, _, cx| {
        let mut tl = Timeline::new(30.0).position(h.state.position).selected(h.state.selected).size(720.0, 160.0);
        for track in &h.state.tracks {
            tl = tl.track(track.iter().cloned());
        }
        tl.on_seek(cx.listener(|h, at: &f32, _, cx| {
            h.state.position = *at;
            cx.notify();
        }))
        .on_select(cx.listener(|h, sel: &(usize, usize), _, cx| {
            h.state.selected = Some(*sel);
            cx.notify();
        }))
        .on_trim(cx.listener(|h, tr: &Trim, _, cx| {
            let c = &mut h.state.tracks[tr.track][tr.clip];
            if tr.edge == Edge::Left { c.start = tr.time } else { c.end = tr.time }
            h.log.push(format!("trim {:?} {}", tr.edge, tr.time));
            cx.notify();
        }))
        .into_any_element()
    })
}

#[gpui::test]
fn timeline_selects_a_clip_and_seeks_on_the_ruler(cx: &mut TestAppContext) {
    let (view, cx) = timeline(cx);
    // Voiceover: track 2 (lanes are 66 tall under the 24-point ruler, 3 apart), 2–12 s.
    cx.simulate_click(point(px(100.), px(120.)), Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.selected), Some((1, 0)));
    // A press on the ruler seeks without changing the selection; a drag scrubs.
    cx.simulate_mouse_down(point(px(240.), px(10.)), MouseButton::Left, Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.position), 10.0);
    cx.simulate_mouse_move(point(px(480.), px(10.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_up(point(px(480.), px(10.)), MouseButton::Left, Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| (h.state.position, h.state.selected)), (20.0, Some((1, 0))));
}

#[gpui::test]
fn timeline_keys_move_the_playhead(cx: &mut TestAppContext) {
    let (view, cx) = timeline(cx);
    focus_first(cx);
    cx.simulate_keystrokes("right right");
    assert_eq!(view.read_with(cx, |h, _| h.state.position), 14.0);
    // Shift: a major (labeled) step, 5 s at this zoom.
    cx.simulate_keystrokes("shift-left");
    assert_eq!(view.read_with(cx, |h, _| h.state.position), 9.0);
    cx.simulate_keystrokes("end");
    assert_eq!(view.read_with(cx, |h, _| h.state.position), 30.0);
    cx.simulate_keystrokes("home left");
    assert_eq!(view.read_with(cx, |h, _| h.state.position), 0.0);
}

#[gpui::test]
fn trim_handles_drag_and_nudge(cx: &mut TestAppContext) {
    let (view, cx) = timeline(cx);
    // Demo's left handle sits at x 192 (8 s); drag it 24 points right: 9 s.
    cx.simulate_mouse_down(point(px(195.), px(60.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_move(point(px(219.), px(60.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_up(point(px(219.), px(60.)), MouseButton::Left, Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.tracks[0][1].start), 9.0);
    assert_eq!(view.read_with(cx, |h, _| h.state.position), 12.0, "a press on a handle doesn't seek");
    // The handle has focus from the press: Shift+Right nudges it 10 points.
    cx.simulate_keystrokes("shift-right");
    let start = view.read_with(cx, |h, _| h.state.tracks[0][1].start);
    assert!((start - (9.0 + 10.0 / 24.0)).abs() < 1e-4, "{start}");
    // Up and Down don't move a trim cap.
    view.update(cx, |h, _| h.log.clear());
    cx.simulate_keystrokes("up down");
    assert!(view.read_with(cx, |h, _| h.log.is_empty()));
}

#[gpui::test]
fn scrubber_steps_with_keys(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, 42.0f32, |h, _, cx| {
        Scrubber::new(h.state, 130.0)
            .on_seek(cx.listener(|h, at: &f32, _, cx| {
                h.state = *at;
                cx.notify();
            }))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("right");
    assert_eq!(view.read_with(cx, |h, _| h.state), 47.0);
    cx.simulate_keystrokes("shift-left left");
    assert_eq!(view.read_with(cx, |h, _| h.state), 12.0);
    cx.simulate_keystrokes("shift-left");
    assert_eq!(view.read_with(cx, |h, _| h.state), 0.0);
    cx.simulate_keystrokes("end");
    assert_eq!(view.read_with(cx, |h, _| h.state), 130.0);
}

#[gpui::test]
fn drag_handle_reports_travel_and_moves(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (), |_, _, cx| {
        DragHandle::new("grip")
            .on_drag(cx.listener(|h, d: &omarchy_gpui::ui::HandleDrag, _, _| h.log.push(format!("{:?} {}", d.phase, d.dy))))
            .on_move(cx.listener(|h, delta: &i32, _, _| h.log.push(format!("move {delta}"))))
            .into_any_element()
    });
    cx.simulate_mouse_down(point(px(8.), px(11.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_move(point(px(8.), px(41.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_up(point(px(8.), px(41.)), MouseButton::Left, Modifiers::none());
    // The press focused it: Up and Down ask to move the row.
    cx.simulate_keystrokes("up down");
    let log = view.update(cx, |h, _| std::mem::take(&mut h.log));
    assert_eq!(log, ["Down 0", "Move 30", "Up 30", "move -1", "move 1"]);
    let _ = DragPhase::Down;
}

#[gpui::test]
fn crop_handles_drag_along_their_axes(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (), |_, _, cx| {
        div()
            .child(
                CropFrame::new(240.0, 140.0)
                    .on_drag(cx.listener(|h, (edge, d): &(Edge, omarchy_gpui::ui::HandleDrag), _, _| {
                        if d.phase == DragPhase::Up {
                            h.log.push(format!("{} {} {}", edge.key(), d.dx, d.dy));
                        }
                    }))
                    .on_nudge(cx.listener(|h, (edge, dx, dy): &(Edge, f32, f32), _, _| h.log.push(format!("nudge {} {dx} {dy}", edge.key())))),
            )
            .into_any_element()
    });
    // The bottom-right corner, dragged up and left.
    cx.simulate_mouse_down(point(px(236.), px(136.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_move(point(px(216.), px(126.)), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_up(point(px(216.), px(126.)), MouseButton::Left, Modifiers::none());
    // Focused by the press: arrows nudge a corner both ways.
    cx.simulate_keystrokes("left shift-down");
    let log = view.update(cx, |h, _| std::mem::take(&mut h.log));
    assert_eq!(log, ["bottom-right -20 -10", "nudge bottom-right -1 0", "nudge bottom-right 0 10"]);
}

/// The gallery's picker: #7aa2f7 at 300 × 260.
fn picker(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<Hsva>>, &mut gpui::VisualTestContext) {
    host(cx, Hsva::from_hex("#7aa2f7", 0.0).unwrap(), |h, _, cx| {
        ColorPicker::new(h.state)
            .size(300.0, 260.0)
            .on_change(cx.listener(|h, v: &Hsva, _, cx| {
                h.state = *v;
                cx.notify();
            }))
            .into_any_element()
    })
}

#[gpui::test]
fn color_picker_square_and_strips_follow_keys(cx: &mut TestAppContext) {
    let (view, cx) = picker(cx);
    let start = view.read_with(cx, |h, _| h.state);
    focus_first(cx);
    cx.simulate_keystrokes("right shift-down");
    let v = view.read_with(cx, |h, _| h.state);
    assert!((v.s - (start.s + 0.01)).abs() < 1e-4 && (v.v - (start.v - 0.1)).abs() < 1e-4, "{v:?}");
    // Tab to the hue strip: Down moves the hue down the strip.
    cx.simulate_keystrokes("tab down");
    assert!((view.read_with(cx, |h, _| h.state.h) - (v.h + 0.01)).abs() < 1e-4);
    // The alpha strip: Down makes it more transparent, and the hex grows an alpha byte.
    cx.simulate_keystrokes("tab shift-down");
    let v = view.read_with(cx, |h, _| h.state);
    assert!((v.a - 0.9).abs() < 1e-4);
    assert_eq!(v.hex().len(), 9);
}

#[gpui::test]
fn color_picker_picks_a_swatch_and_reads_typed_hex(cx: &mut TestAppContext) {
    let (view, cx) = picker(cx);
    // Square, hue, alpha, the hex field, then the first palette swatch: red (#f7768e).
    focus_first(cx);
    cx.simulate_keystrokes("tab tab tab tab enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.hex()), "#f7768e");
    // A valid hex typed into the field applies at once; an invalid one doesn't.
    cx.simulate_keystrokes("shift-tab");
    cx.simulate_keystrokes("ctrl-a backspace");
    cx.simulate_input("#9ece6a");
    assert_eq!(view.read_with(cx, |h, _| h.state.hex()), "#9ece6a");
    cx.simulate_input("z");
    assert_eq!(view.read_with(cx, |h, _| h.state.hex()), "#9ece6a");
}

#[test]
fn hsva_round_trips_hex() {
    let v = Hsva::from_hex("#7aa2f7", 0.0).unwrap();
    assert_eq!(v.hex(), "#7aa2f7");
    assert!((v.h * 360.0 - 220.8).abs() < 0.1 && (v.s - 0.506).abs() < 1e-3 && (v.v - 0.969).abs() < 1e-3);
    // Grey keeps the hue it's given; alpha shows as a fourth byte.
    assert_eq!(Hsva::from_hex("#808080", 0.4).unwrap().h, 0.4);
    assert_eq!(Hsva { a: 0.5, ..v }.hex(), "#7aa2f780");
    assert!(Hsva::from_hex("#12345", 0.0).is_none());
}

#[gpui::test]
fn timeline_wheel_zooms_around_the_pointer(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (1.0f32, 0.0f32), |h, _, cx| {
        Timeline::new(30.0)
            .size(720.0, 160.0)
            .zoom(h.state.0, h.state.1)
            .track([Clip::new(0.0, 30.0, "All")])
            .on_view(cx.listener(|h, v: &(f32, f32), _, cx| {
                h.state = *v;
                cx.notify();
            }))
            .into_any_element()
    });
    // One notch up at 15 s (x 360): ×1.25, 15 s stays under the pointer.
    cx.simulate_event(gpui::ScrollEvent { position: point(px(360.), px(80.)), delta: gpui::ScrollDelta::Lines(point(0., 1.)), ..Default::default() });
    let (zoom, start) = view.read_with(cx, |h, _| h.state);
    assert!((zoom - 1.25).abs() < 1e-4 && (start + 360.0 / (720.0 * 1.25 / 30.0) - 15.0).abs() < 1e-3, "{zoom} {start}");
    // Out past 1 clamps to the whole duration.
    cx.simulate_event(gpui::ScrollEvent { position: point(px(360.), px(80.)), delta: gpui::ScrollDelta::Lines(point(0., -3.)), ..Default::default() });
    assert_eq!(view.read_with(cx, |h, _| h.state), (1.0, 0.0));
}
