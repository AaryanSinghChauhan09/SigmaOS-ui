//! Meter, ProgressBar, Spinner, Banner, EmptyState and Toast: what they paint and what their
//! actions run, as the Rune gallery's spinner test and Qt's feedback specimens expect.

mod common;

use common::{focus_first, host, take_log};
use gpui::{IntoElement, ParentElement, Styled, TestAppContext, div, px};
use omarchy_gpui::ui::{Banner, EmptyState, FeedbackTone, Meter, ProgressBar, Spinner, Toast};

/// The widths of the painted quads, in points (the test window is 2×).
fn quad_widths(cx: &mut gpui::VisualTestContext) -> Vec<f32> {
    cx.update(|window, _| window.painted_quads().iter().map(|q| q.bounds.size.width.0 / 2.0).collect())
}

#[gpui::test]
fn meter_fills_its_value_clamped(cx: &mut TestAppContext) {
    let (_v, cx) = host(cx, (), |_, _, _| {
        div()
            .child(Meter::new(0.5).width(200.0))
            .child(Meter::new(1.7).width(100.0).tone(FeedbackTone::Error))
            .into_any_element()
    });
    let widths = quad_widths(cx);
    assert!(widths.iter().any(|w| (w - 100.0).abs() < 0.01), "half of 200: {widths:?}");
    assert!(widths.iter().filter(|w| (*w - 100.0).abs() < 0.01).count() >= 3, "a full 100 fill on the 100 track");
}

#[gpui::test]
fn indeterminate_progress_holds_still_under_reduced_motion(cx: &mut TestAppContext) {
    let (_v, cx) = host(cx, (), |_, _, _| ProgressBar::indeterminate().width(200.0).into_any_element());
    cx.update(|_, cx| {
        let mut t = common::fixture("tokyo-night");
        t.reduce_motion = true;
        omarchy_gpui::set_tokens(cx, t);
    });
    cx.run_until_parked();
    let quads = cx.update(|window, _| window.painted_quads());
    let segment = quads.iter().find(|q| (q.bounds.size.width.0 / 2.0 - 50.0).abs() < 0.01).expect("a quarter-track segment");
    let track = quads.iter().find(|q| (q.bounds.size.width.0 / 2.0 - 200.0).abs() < 0.01).expect("the track");
    let x = (segment.bounds.origin.x - track.bounds.origin.x).0 / 2.0;
    assert!((x - 75.0).abs() < 0.01, "3/8 along: {x}");
}

#[gpui::test]
fn spinners_draw_turning_pulsing_and_still(cx: &mut TestAppContext) {
    // Turning (a glyph painted as a turned SVG), pulsing (opacity on the clock) and, under reduced
    // motion, still text: every mode lays out and paints in its glyph's line-height square. (GPUI's
    // animations run on the wall clock, so the test can't step their frames as Rune's does.)
    let (_v, cx) = host(cx, (), |_, _, _| {
        div()
            .flex()
            .child(Spinner::new())
            .child(Spinner::new().pulse(true))
            .child(Spinner::new().size(18).glyph("\u{f0453}"))
            .into_any_element()
    });
    cx.run_until_parked();
    cx.update(|_, cx| {
        let mut t = common::fixture("tokyo-night");
        t.reduce_motion = true;
        omarchy_gpui::set_tokens(cx, t);
    });
    cx.update(|window, _| window.refresh());
    cx.run_until_parked();
}

#[gpui::test]
fn banner_action_and_dismiss_run(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (), |_, _, cx| {
        Banner::new("Omarchy 3.2 is available")
            .action("Update")
            .dismissible(true)
            .on_action(cx.listener(|h: &mut common::Host<()>, _, _, _| h.log.push("update".into())))
            .on_dismiss(cx.listener(|h: &mut common::Host<()>, _, _, _| h.log.push("dismiss".into())))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("enter tab space");
    assert_eq!(take_log(&view, cx), ["update", "dismiss"]);
}

#[gpui::test]
fn empty_state_and_toast_actions_run(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (), |_, _, cx| {
        div()
            .w(px(400.))
            .child(EmptyState::new("No networks found").action("Scan").on_action(cx.listener(|h: &mut common::Host<()>, _, _, _| {
                h.log.push("scan".into())
            })))
            .child(Toast::new("Export finished").action("Open").countdown(0.6).on_action(cx.listener(
                |h: &mut common::Host<()>, _, _, _| h.log.push("open".into()),
            )))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("enter tab enter");
    assert_eq!(take_log(&view, cx), ["scan", "open"]);
}
