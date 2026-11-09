//! The `forms` section (spec/components/forms.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{
    Checkbox, Dropdown, NumberField, PasswordField, RadioGroup, SettingRow, TextArea, TextAreaInput, TextInput, ToggleSwitch,
};

use crate::Gallery;

/// What this section keeps between frames.
pub struct State {
    pub night: bool,
    pub theme: SharedString,
    pub gaps: i64,
    pub checks: [bool; 5],
    pub position: SharedString,
    pub password: Entity<TextInput>,
    pub filled: Entity<TextInput>,
    pub wrong: Entity<TextInput>,
    pub notes: Entity<TextAreaInput>,
}

impl State {
    pub fn new(cx: &mut App) -> State {
        State {
            night: true,
            theme: "tokyo-night".into(),
            gaps: 10,
            checks: [false, true, false, false, true],
            position: "bottom".into(),
            password: TextInput::new(cx, ""),
            filled: TextInput::new(cx, "hunter22"),
            wrong: TextInput::new(cx, "letmein"),
            notes: TextAreaInput::new(cx, "Back up your dotfiles\nbefore switching themes,\nthen run omarchy update."),
        }
    }
}

pub fn build(g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = t.tokens().spacing.clone();
    let f = &g.more.forms;
    let settings = div()
        .flex()
        .flex_col()
        .w(px(480.0))
        .child(
            SettingRow::new("Night light").description("Warm the screen after sunset").control(ToggleSwitch::new("forms-night", f.night).on_toggle(
                cx.listener(|g: &mut Gallery, v: &bool, _, cx| {
                    g.more.forms.night = *v;
                    cx.notify();
                }),
            )),
        )
        .child(
            SettingRow::new("Theme").description("Applies to every app").control(
                Dropdown::new("forms-theme", [("tokyo-night", "Tokyo Night"), ("nord", "Nord")], f.theme.clone())
                    .width(180.0)
                    .on_choose(cx.listener(|g: &mut Gallery, v: &SharedString, _, cx| {
                        g.more.forms.theme = v.clone();
                        cx.notify();
                    })),
            ),
        )
        .child(
            SettingRow::new("Gaps").description("Space between windows").control(
                NumberField::new("", f.gaps).id("forms-gaps").range(0, 40).on_change(cx.listener(|g: &mut Gallery, v: &i64, _, cx| {
                    g.more.forms.gaps = *v;
                    cx.notify();
                })),
            ),
        );

    let check = |i: usize, label: &'static str, f: &State, cx: &mut Context<Gallery>| {
        Checkbox::new(label).checked(f.checks[i]).on_toggle(cx.listener(move |g: &mut Gallery, v: &bool, _, cx| {
            g.more.forms.checks[i] = *v;
            cx.notify();
        }))
    };
    let checks = div()
        .flex()
        .flex_col()
        .gap(px(sp.lg as f32))
        .child(check(0, "Show seconds", f, cx))
        .child(check(1, "Show date", f, cx))
        .child(check(2, "Show weekday", f, cx).cursor(true))
        .child(check(3, "Show week number", f, cx).disabled(true))
        .child(check(4, "Twelve-hour clock", f, cx).description("Show AM and PM"));
    let radios = RadioGroup::new([("top", "Top"), ("bottom", "Bottom"), ("hidden", "Hidden")], f.position.clone()).on_choose(cx.listener(
        |g: &mut Gallery, v: &SharedString, _, cx| {
            g.more.forms.position = v.clone();
            cx.notify();
        },
    ));

    let passwords = div()
        .flex()
        .flex_row()
        .items_start()
        .gap(px(sp.control_gap as f32))
        .child(PasswordField::new(&f.password).placeholder("Password").width(220.0))
        .child(PasswordField::new(&f.filled).revealable(true).width(220.0))
        .child(PasswordField::new(&f.wrong).error("Wrong password, try again").width(220.0));

    vec![
        settings.into_any_element(),
        div().flex().flex_row().items_start().gap(px(sp.huge as f32 * 2.0)).child(checks).child(radios).into_any_element(),
        passwords.into_any_element(),
        TextArea::new(&f.notes).width(480.0).into_any_element(),
    ]
}
