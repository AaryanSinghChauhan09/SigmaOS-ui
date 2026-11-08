//! The `forms` section (spec/components/forms.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Checkbox, Dropdown, NumberField, PasswordField, RadioGroup, SettingRow, TextArea, ToggleSwitch};
use rune::{El, TextEdit, col, row};

use crate::State;

pub struct Forms {
    pub night: bool,
    pub theme: String,
    pub theme_open: bool,
    pub gaps: i64,
    pub checks: [bool; 5],
    pub position: String,
    pub password: TextEdit,
    pub filled: TextEdit,
    pub wrong: TextEdit,
    pub revealed: bool,
    pub notes: TextEdit,
}

pub fn init() -> Forms {
    Forms {
        night: true,
        theme: "tokyo-night".into(),
        theme_open: false,
        gaps: 10,
        checks: [false, true, false, false, true],
        position: "bottom".into(),
        password: TextEdit::new(""),
        filled: TextEdit::new("hunter22"),
        wrong: TextEdit::new("letmein"),
        revealed: false,
        notes: TextEdit::new("Back up your dotfiles\nbefore switching themes,\nthen run omarchy update."),
    }
}

pub fn build(s: &State, t: &Theme, layer: &mut Vec<El>) -> Vec<El> {
    let f = &s.forms;
    let sp = &t.tokens().spacing;
    let settings = col()
        .w(480.0)
        .child(
            SettingRow::new("Night light")
                .description("Warm the screen after sunset")
                .control(ToggleSwitch::new(f.night).key("night").on_toggle(|s: &mut State| s.forms.night ^= true).build(t))
                .build(t),
        )
        .child(
            SettingRow::new("Theme")
                .description("Applies to every app")
                .control(
                    Dropdown::new("forms-theme", [("tokyo-night", "Tokyo Night"), ("nord", "Nord")], f.theme.clone())
                        .width(180.0)
                        .open(f.theme_open)
                        .on_toggle(|s: &mut State| s.forms.theme_open ^= true)
                        .on_choose(|s: &mut State, v: &str| {
                            s.forms.theme = v.into();
                            s.forms.theme_open = false;
                        })
                        .build(t, layer),
                )
                .build(t),
        )
        .child(
            SettingRow::new("Gaps")
                .description("Space between windows")
                .control(NumberField::new("", f.gaps).range(0, 40).on_change(|s: &mut State, v| s.forms.gaps = v).build(t))
                .build(t),
        );

    let check = |i: usize, label: &str| {
        Checkbox::new(label).checked(f.checks[i]).key(format!("check-{i}")).on_toggle(move |s: &mut State| s.forms.checks[i] ^= true)
    };
    let checks = col()
        .gap(sp.lg as f32)
        .child(check(0, "Show seconds").build(t))
        .child(check(1, "Show date").build(t))
        .child(check(2, "Show weekday").cursor(true).build(t))
        .child(check(3, "Show week number").disabled(true).build(t))
        .child(check(4, "Twelve-hour clock").description("Show AM and PM").build(t));
    let radios = RadioGroup::new([("top", "Top"), ("bottom", "Bottom"), ("hidden", "Hidden")], f.position.clone())
        .on_choose(|s: &mut State, v: &str| s.forms.position = v.into())
        .build(t);

    let passwords = row()
        .items_start()
        .gap(sp.control_gap as f32)
        .child(PasswordField::new(&f.password).placeholder("Password").width(220.0).on_edit(|s: &mut State| &mut s.forms.password).build(t))
        .child(
            PasswordField::new(&f.filled)
                .width(220.0)
                .revealed(f.revealed)
                .on_reveal(|s: &mut State| s.forms.revealed ^= true)
                .on_edit(|s: &mut State| &mut s.forms.filled)
                .build(t),
        )
        .child(
            PasswordField::new(&f.wrong)
                .width(220.0)
                .error("Wrong password, try again")
                .on_edit(|s: &mut State| &mut s.forms.wrong)
                .build(t),
        );

    vec![
        settings,
        row().items_start().gap(sp.huge as f32 * 2.0).child(checks).child(radios),
        passwords,
        TextArea::new(&f.notes).width(480.0).on_edit(|s: &mut State| &mut s.forms.notes).build(t),
    ]
}
