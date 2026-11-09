//! Checkbox, RadioGroup, MultiSelect, PasswordField, TextArea, ColorWell and SwatchStrip driven by
//! keys and clicks, as `qt/tests/tst_forms.qml` drives Qt's and the Rune gallery's
//! forms_check_and_choose, multiselect_toggles_values, multiselect_keys_as_in_qt and
//! choosers_follow_keys_as_in_qt drive Rune's.

mod common;

use common::{fixture, focus_first, host, take_log};
use gpui::{IntoElement, Modifiers, ParentElement, SharedString, Styled, TestAppContext, div, point, px};
use omarchy_gpui::ui::{Checkbox, ColorWell, MultiSelect, PasswordField, RadioGroup, SwatchStrip, TextArea, TextAreaInput, TextInput};

fn flip(list: &mut Vec<SharedString>, v: &SharedString) {
    match list.iter().position(|x| x == v) {
        Some(i) => {
            list.remove(i);
        }
        None => list.push(v.clone()),
    }
}

#[gpui::test]
fn checkbox_flips_on_click_and_keys_unless_disabled(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (false, false), |h, _, cx| {
        Checkbox::new("Show hidden files")
            .checked(h.state.0)
            .disabled(h.state.1)
            .on_toggle(cx.listener(|h, v: &bool, _, cx| {
                h.state.0 = *v;
                cx.notify();
            }))
            .into_any_element()
    });
    cx.simulate_click(point(px(5.), px(5.)), Modifiers::none());
    assert!(view.read_with(cx, |h, _| h.state.0), "a click checks it");
    focus_first(cx);
    cx.simulate_keystrokes("space");
    assert!(!view.read_with(cx, |h, _| h.state.0), "Space unchecks it");
    cx.simulate_keystrokes("enter");
    assert!(view.read_with(cx, |h, _| h.state.0), "Return checks it");
    view.update(cx, |h, cx| {
        h.state.1 = true;
        cx.notify();
    });
    cx.run_until_parked();
    cx.simulate_click(point(px(5.), px(5.)), Modifiers::none());
    cx.simulate_keystrokes("space");
    assert!(view.read_with(cx, |h, _| h.state.0), "a disabled checkbox doesn't flip");
}

#[gpui::test]
fn radios_choose_on_click_and_keys(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, SharedString::from("bottom"), |h, _, cx| {
        RadioGroup::new([("top", "Top"), ("bottom", "Bottom"), ("hidden", "Hidden")], h.state.clone())
            .on_choose(cx.listener(|h, v: &SharedString, _, cx| {
                h.state = v.clone();
                cx.notify();
            }))
            .into_any_element()
    });
    let t = fixture("tokyo-night");
    let pitch = t.font.line_px(t.font.body) as f32 + t.spacing.lg as f32;
    cx.simulate_click(point(px(5.), px(2.0 * pitch + 5.0)), Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), "hidden");
    cx.simulate_click(point(px(5.), px(5.0)), Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), "top");
    // The click focused the group; the keys move the choice and stop at the ends.
    for (key, want) in [
        ("down", "bottom"),
        ("j", "hidden"),
        ("down", "hidden"),
        ("k", "bottom"),
        ("up", "top"),
        ("up", "top"),
        ("right", "bottom"),
        ("left", "top"),
    ] {
        cx.simulate_keystrokes(key);
        assert_eq!(view.read_with(cx, |h, _| h.state.clone()), want, "radios after {key}");
    }
}

fn multiselect_host(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<Vec<SharedString>>>, &mut gpui::VisualTestContext) {
    host(cx, vec![SharedString::from("Mail"), SharedString::from("Updates")], |h, _, cx| {
        div()
            .size_full()
            .child(
                MultiSelect::new("notify", ["Calendar", "Mail", "Messages", "Updates", "Battery"].map(|m| (m, m)), h.state.clone())
                    .label("Notify for")
                    .width(260.0)
                    .on_choose(cx.listener(|h, v: &SharedString, _, cx| {
                        flip(&mut h.state, v);
                        h.log.push(v.to_string());
                        cx.notify();
                    })),
            )
            .into_any_element()
    })
}

#[gpui::test]
fn multiselect_toggles_on_click(cx: &mut TestAppContext) {
    let (view, cx) = multiselect_host(cx);
    let t = fixture("tokyo-night");
    let sp = &t.spacing;
    let trigger_y = t.font.line_px(t.font.caption) as f32 + sp.label_gap as f32;
    cx.simulate_click(point(px(20.), px(trigger_y + 10.0)), Modifiers::none());
    let line = t.controls.normal.border.max_width().max(1.0) as f32;
    let first = trigger_y
        + sp.control_height as f32
        + sp.xxs as f32
        + line
        + sp.hairline as f32
        + (sp.popup_row_height + sp.control_padding_x) as f32
        + 1.0;
    let row = |i: usize| point(px(40.), px(first + i as f32 * (sp.popup_row_height + sp.label_gap) as f32 + sp.popup_row_height as f32 / 2.0));
    cx.simulate_click(row(0), Modifiers::none());
    assert!(view.read_with(cx, |h, _| h.state.contains(&"Calendar".into())), "Calendar toggled on");
    cx.simulate_click(row(1), Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), ["Updates", "Calendar"]);
}

#[gpui::test]
fn multiselect_keys(cx: &mut TestAppContext) {
    let (view, cx) = multiselect_host(cx);
    focus_first(cx);
    cx.simulate_keystrokes("j");
    // The search has the keys: typing filters, Return toggles the first match.
    cx.simulate_input("mes");
    cx.simulate_keystrokes("enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), ["Mail", "Updates", "Messages"], "Return toggles the first match");
    cx.simulate_keystrokes("backspace backspace backspace down space");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), ["Mail", "Updates", "Messages", "Calendar"], "Down into the list, Space toggles");
    cx.simulate_keystrokes("j enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), ["Updates", "Messages", "Calendar"], "j walks, Return toggles");
    assert_eq!(take_log(&view, cx).len(), 3);
    // k off the first row goes back to the search; Escape closes, and the keys go nowhere.
    cx.simulate_keystrokes("k k");
    cx.simulate_input("bat");
    cx.simulate_keystrokes("enter escape");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), ["Updates", "Messages", "Calendar", "Battery"]);
    cx.simulate_keystrokes("space");
    cx.simulate_input("x");
    cx.simulate_keystrokes("enter");
    assert_eq!(take_log(&view, cx), ["Battery"], "reopened by Space with an empty search; x matches nothing");
}

#[gpui::test]
fn password_field_and_text_area_edit(cx: &mut TestAppContext) {
    let secret = cx.update(|cx| TextInput::new(cx, ""));
    let notes = cx.update(|cx| TextAreaInput::new(cx, "one"));
    let (s2, n2) = (secret.clone(), notes.clone());
    let (_view, cx) = host(cx, (), move |_, _, _| {
        div()
            .w(px(400.))
            .child(PasswordField::new(&s2).placeholder("Password").revealable(true))
            .child(TextArea::new(&n2))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_input("hunter2");
    assert_eq!(secret.read_with(cx, |i, _| i.text().clone()), "hunter2");
    cx.simulate_keystrokes("tab");
    cx.simulate_keystrokes("enter");
    cx.simulate_input("two");
    cx.simulate_keystrokes("backspace");
    assert_eq!(notes.read_with(cx, |i, _| i.text().clone()), "one\ntw");
}

#[gpui::test]
fn color_well_presses_and_swatches_pick(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, 2usize, |h, _, cx| {
        div()
            .child(ColorWell::new(gpui::red()).on_click(cx.listener(|h, _, _, _| h.log.push("well".into()))))
            .child(SwatchStrip::new([gpui::red(), gpui::green(), gpui::blue(), gpui::black()], Some(h.state)).on_pick(cx.listener(
                |h, i: &usize, _, cx| {
                    h.state = *i;
                    cx.notify();
                },
            )))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("space");
    assert_eq!(take_log(&view, cx), ["well"]);
    cx.simulate_keystrokes("tab l l l enter");
    assert_eq!(view.read_with(cx, |h, _| h.state), 3, "l walks from the chosen swatch and stops at the end");
    cx.simulate_keystrokes("h h space");
    assert_eq!(view.read_with(cx, |h, _| h.state), 1);
}
