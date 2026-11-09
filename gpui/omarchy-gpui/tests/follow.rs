//! The theme layer (spec/following.md) on GPUI's test clock: a change is in effect at once, the
//! changed hook and `observe_global::<Theme>` fire once with the new tokens, the revision counts,
//! colors fade over `motion.theme` and land exactly, a second change mid-fade starts from what is
//! on screen, and reduced motion or an unchanged theme doesn't fade.

mod common;

use std::cell::RefCell;
use std::rc::Rc;
use std::time::Duration;

use common::fixture;
use gpui::TestAppContext;
use omarchy_gpui::{Paint, Theme, Tokens};

fn setup(cx: &mut TestAppContext) -> (Rc<RefCell<Vec<String>>>, Rc<RefCell<usize>>) {
    let hooks: Rc<RefCell<Vec<String>>> = Rc::default();
    let observed: Rc<RefCell<usize>> = Rc::default();
    let (h, o) = (hooks.clone(), observed.clone());
    cx.update(|cx| {
        omarchy_gpui::install(cx, fixture("tokyo-night"));
        omarchy_gpui::on_change(cx, move |t: &Tokens, _| h.borrow_mut().push(t.palette.background.hex())).detach();
        cx.observe_global::<Theme>(move |_| *o.borrow_mut() += 1).detach();
    });
    cx.run_until_parked();
    (hooks, observed)
}

fn bg(cx: &mut TestAppContext) -> (String, String, bool) {
    cx.update(|cx| (Theme::get(cx).tokens().palette.background.hex(), Paint::of(cx).tokens().palette.background.hex(), Paint::fading(cx)))
}

#[gpui::test]
fn a_change_fades_and_lands(cx: &mut TestAppContext) {
    let (hooks, observed) = setup(cx);
    assert_eq!(bg(cx), ("#1a1b26ff".into(), "#1a1b26ff".into(), false), "first paint is exact, no fade");
    assert!(hooks.borrow().is_empty(), "never for the initial load");
    let latte = fixture("catppuccin-latte");
    cx.update(|cx| omarchy_gpui::set_tokens(cx, latte.clone()));
    cx.run_until_parked();
    assert_eq!(*hooks.borrow(), ["#eff1f5ff"]);
    assert_eq!(*observed.borrow(), 1);
    assert_eq!(cx.update(|cx| Theme::get(cx).revision()), 1);
    let (target, shown, fading) = bg(cx);
    assert_eq!((target.as_str(), shown.as_str(), fading), ("#eff1f5ff", "#1a1b26ff", true), "the fade starts from the screen");

    cx.executor().advance_clock(Duration::from_millis(140));
    let (_, mid, fading) = bg(cx);
    assert!(fading && mid != "#1a1b26ff" && mid != "#eff1f5ff", "mid-fade: {mid}");

    cx.executor().advance_clock(Duration::from_millis(200));
    assert_eq!(bg(cx), ("#eff1f5ff".into(), "#eff1f5ff".into(), false), "landed exactly");
    cx.update(|cx| assert_eq!(*Paint::of(cx).tokens(), latte));

    // The same tokens again are no change.
    cx.update(|cx| omarchy_gpui::set_tokens(cx, latte));
    cx.run_until_parked();
    assert_eq!(hooks.borrow().len(), 1);
    assert_eq!(cx.update(|cx| Theme::get(cx).revision()), 1);
}

#[gpui::test]
fn a_second_change_starts_from_the_screen(cx: &mut TestAppContext) {
    let (hooks, _) = setup(cx);
    cx.update(|cx| omarchy_gpui::set_tokens(cx, fixture("catppuccin-latte")));
    cx.executor().advance_clock(Duration::from_millis(100));
    let (_, mid, _) = bg(cx);
    cx.update(|cx| omarchy_gpui::set_tokens(cx, fixture("gruvbox")));
    cx.run_until_parked();
    let (target, shown, fading) = bg(cx);
    assert_eq!(shown, mid, "the new fade starts from the colors on screen");
    assert!(fading);
    cx.executor().advance_clock(Duration::from_millis(300));
    let (_, landed, fading) = bg(cx);
    assert_eq!((landed, fading), (target, false));
    assert_eq!(hooks.borrow().len(), 2);
    assert_eq!(cx.update(|cx| Theme::get(cx).revision()), 2);
}

#[gpui::test]
fn reduced_motion_lands_at_once(cx: &mut TestAppContext) {
    let (hooks, _) = setup(cx);
    let mut latte = fixture("catppuccin-latte");
    latte.reduce_motion = true;
    cx.update(|cx| omarchy_gpui::set_tokens(cx, latte));
    cx.run_until_parked();
    assert_eq!(bg(cx), ("#eff1f5ff".into(), "#eff1f5ff".into(), false));
    assert_eq!(hooks.borrow().len(), 1);
}

#[gpui::test]
fn layout_switches_at_once(cx: &mut TestAppContext) {
    setup(cx);
    let mut rounded = fixture("catppuccin-latte");
    rounded.radius = 8;
    cx.update(|cx| omarchy_gpui::set_tokens(cx, rounded));
    cx.run_until_parked();
    // Mid-fade, the rounding on screen is already the new one.
    assert_eq!(cx.update(|cx| (Paint::fading(cx), Paint::of(cx).tokens().radius)), (true, 8));
}
