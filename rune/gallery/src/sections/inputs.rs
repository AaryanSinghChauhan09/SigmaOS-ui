//! The `inputs` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Dropdown, NumberField, TextField};
use rune::{El, row};

use crate::State;

pub fn build(s: &State, t: &Theme, layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    vec![
        row()
            .gap(sp.control_gap as f32)
            .child(
                TextField::new(&s.search)
                    .placeholder("Search…")
                    .width(220.0)
                    .on_edit(|s: &mut State| &mut s.search)
                    .build(t),
            )
            .child(
                TextField::new(&s.name)
                    .width(220.0)
                    .on_edit(|s: &mut State| &mut s.name)
                    .build(t),
            )
            .child(
                TextField::new(&s.hovered)
                    .cursor(true)
                    .width(220.0)
                    .on_edit(|s: &mut State| &mut s.hovered)
                    .build(t),
            ),
        row()
            .items_start()
            .gap(sp.xxl as f32)
            .child(
                NumberField::new("Gaps", s.gaps)
                    .range(0, 40)
                    .on_change(|s: &mut State, v| s.gaps = v)
                    .build(t),
            )
            .child(
                Dropdown::new(
                    "theme-dropdown",
                    [("tokyo-night", "Tokyo Night"), ("nord", "Nord")],
                    s.theme_choice.clone(),
                )
                .label("Theme")
                .open(s.dropdown_open)
                .on_toggle(|s: &mut State| s.dropdown_open ^= true)
                .on_choose(|s: &mut State, v: &str| {
                    s.theme_choice = v.into();
                    s.dropdown_open = false;
                })
                .build(t, layer),
            ),
    ]
}
