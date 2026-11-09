//! The `inputs` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Dropdown, NumberField, TextField};

use crate::Gallery;

pub fn build(g: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = &t.tokens().spacing;
    vec![
        div()
            .flex()
            .flex_row()
            .gap(px(sp.control_gap as f32))
            .child(TextField::new(&g.search).placeholder("Search…").width(220.0))
            .child(TextField::new(&g.name).width(220.0))
            .child(TextField::new(&g.hovered).cursor(true).width(220.0))
            .into_any_element(),
        div()
            .flex()
            .flex_row()
            .items_start()
            .gap(px(sp.xxl as f32))
            .child(NumberField::new("Gaps", g.gaps).range(0, 40).on_change(cx.listener(|g: &mut Gallery, v: &i64, _, cx| {
                g.gaps = *v;
                cx.notify();
            })))
            .child(
                Dropdown::new("theme-dropdown", [("tokyo-night", "Tokyo Night"), ("nord", "Nord")], g.theme_choice.clone())
                    .label("Theme")
                    .on_choose(cx.listener(|g: &mut Gallery, v: &SharedString, _, cx| {
                        g.theme_choice = v.clone();
                        cx.notify();
                    })),
            )
            .into_any_element(),
    ]
}
