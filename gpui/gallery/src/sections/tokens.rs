//! The `tokens` section (spec/gallery.md).

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{border, label};
use omarchy_gpui::{Paint, color};

use crate::Gallery;

pub fn build(_: &mut Gallery, _: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let tk = t.tokens();
    let sp = &tk.spacing;
    let p = &tk.palette;
    let mut swatches = div().flex().flex_row().gap(px(sp.lg as f32));
    for (name, c) in [("foreground", p.foreground), ("background", p.background), ("accent", p.accent), ("urgent", p.urgent), ("muted", p.muted)] {
        swatches = swatches.child(
            div()
                .flex()
                .flex_col()
                .gap(px(sp.xs as f32))
                .child(
                    div()
                        .w(px(64.0))
                        .h(px(32.0))
                        .rounded(t.radius_small())
                        .bg(color(c))
                        .border_1()
                        .border_color(color(tk.controls.normal.border.color)),
                )
                .child(label(&t, name, tk.font.caption, false, t.fg())),
        );
    }
    let c = &tk.controls;
    let mut states = div().flex().flex_row().gap(px(sp.lg as f32));
    for (name, st) in [("normal", &c.normal), ("hover-cursor", &c.hover), ("focus", &c.focus), ("selected", &c.selected)] {
        let chip = div()
            .flex()
            .items_center()
            .justify_center()
            .w(px(120.0))
            .h(px(sp.control_height as f32))
            .rounded(t.radius())
            .bg(color(st.fill))
            .child(label(&t, name, tk.font.body, name == "selected", color(st.text)));
        states = states.child(border(chip, &st.border));
    }
    let f = &tk.font;
    let mut scale = div().flex().flex_col().gap(px(sp.xs as f32));
    for (name, size) in [
        ("displayLarge", f.display_large),
        ("display", f.display),
        ("heading", f.heading),
        ("title", f.title),
        ("subtitle", f.subtitle),
        ("body", f.body),
        ("bodySmall", f.body_small),
        ("caption", f.caption),
    ] {
        scale = scale.child(label(&t, format!("{name} {size}"), size, false, t.fg()));
    }
    vec![swatches.into_any_element(), states.into_any_element(), scale.into_any_element()]
}
