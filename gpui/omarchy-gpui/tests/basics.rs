//! Buttons, toggles, inputs and sliders driven by keys and clicks, as `qt/tests` drives Qt's
//! (tst_button, tst_toggle, tst_inputs, tst_slider).

mod common;

use common::{focus_first, host, take_log};
use gpui::{IntoElement, ParentElement, SharedString, Styled, TestAppContext, div};
use omarchy_gpui::ui::{Button, ButtonGroup, Dropdown, NumberField, Slider, Toggle, ToggleSwitch};

#[gpui::test]
fn button_activates_on_return_and_space(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (), |_, _, cx| {
        Button::new("Save").on_click(cx.listener(|h, _, _, _| h.log.push("save".into()))).into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("enter space");
    assert_eq!(take_log(&view, cx), ["save", "save"]);
    cx.simulate_keystrokes("a");
    assert!(take_log(&view, cx).is_empty());
}

#[gpui::test]
fn toggle_and_switch_flip(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (false, true), |h, _, cx| {
        div()
            .child(Toggle::new("Wi-Fi").checked(h.state.0).on_toggle(cx.listener(|h, v: &bool, _, cx| {
                h.state.0 = *v;
                cx.notify();
            })))
            .child(ToggleSwitch::new("sw", h.state.1).on_toggle(cx.listener(|h, v: &bool, _, cx| {
                h.state.1 = *v;
                cx.notify();
            })))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("space");
    assert!(view.read_with(cx, |h, _| h.state.0));
    cx.simulate_keystrokes("enter tab space");
    assert_eq!(view.read_with(cx, |h, _| h.state), (false, false));
}

#[gpui::test]
fn button_group_walks_with_arrows(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, SharedString::from("center"), |h, _, cx| {
        ButtonGroup::new([("left", "Left"), ("center", "Center"), ("right", "Right")], h.state.clone())
            .on_choose(cx.listener(|h, v: &SharedString, _, cx| {
                h.state = v.clone();
                cx.notify();
            }))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("right");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), "right");
    cx.simulate_keystrokes("right left left left");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), "left");
}

#[gpui::test]
fn number_field_steps_within_range(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, 39i64, |h, _, cx| {
        NumberField::new("Gaps", h.state)
            .range(0, 40)
            .on_change(cx.listener(|h, v: &i64, _, cx| {
                h.state = *v;
                cx.notify();
            }))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("up up up");
    assert_eq!(view.read_with(cx, |h, _| h.state), 40);
    cx.simulate_keystrokes("down home");
    assert_eq!(view.read_with(cx, |h, _| h.state), 0);
}

#[gpui::test]
fn dropdown_opens_walks_and_chooses(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, SharedString::from("tokyo-night"), |h, _, cx| {
        div()
            .size_full()
            .child(
                Dropdown::new("theme", [("tokyo-night", "Tokyo Night"), ("nord", "Nord"), ("gruvbox", "Gruvbox")], h.state.clone())
                    .label("Theme")
                    .on_choose(cx.listener(|h, v: &SharedString, _, cx| {
                        h.state = v.clone();
                        cx.notify();
                    })),
            )
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("j j j enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), "gruvbox");
    cx.simulate_keystrokes("space k escape");
    assert_eq!(view.read_with(cx, |h, _| h.state.clone()), "gruvbox");
}

#[gpui::test]
fn slider_steps_with_keys(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, 0.5f32, |h, _, cx| {
        Slider::new("level", h.state)
            .on_change(cx.listener(|h, v: &f32, _, cx| {
                h.state = *v;
                cx.notify();
            }))
            .into_any_element()
    });
    focus_first(cx);
    cx.simulate_keystrokes("right right");
    assert!((view.read_with(cx, |h, _| h.state) - 0.6).abs() < 1e-4);
    cx.simulate_keystrokes("end");
    assert_eq!(view.read_with(cx, |h, _| h.state), 1.0);
}

#[gpui::test]
fn text_input_edits(cx: &mut TestAppContext) {
    use omarchy_gpui::ui::{TextField, TextInput};
    let input = cx.update(|cx| TextInput::new(cx, "Omarchy"));
    let i2 = input.clone();
    let (_view, cx) = host(cx, (), move |_, _, _| div().w(gpui::px(300.)).child(TextField::new(&i2)).into_any_element());
    focus_first(cx);
    cx.simulate_keystrokes("backspace backspace backspace home");
    cx.simulate_input("Hi ");
    assert_eq!(input.read_with(cx, |i, _| i.text().clone()), "Hi Omar");
}
