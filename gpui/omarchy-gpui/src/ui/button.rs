//! Button, IconButton and ButtonGroup: the shell's `Button.qml`, `PanelActionButton.qml` and
//! `ButtonGroup.qml`.

use std::rc::Rc;

use gpui::prelude::*;
use gpui::{App, ClickEvent, ElementId, KeyDownEvent, SharedString, Window, div, px};
use omarchy_theme::{BorderSpec, Rgba};

use super::{GROUP, Handler, Look, chrome, focus_handle, id_for, is_activate, label, reserve, tab_keys};
use crate::{Paint, color};

/// A glyph (a Nerd Font codepoint) in the Omarchy font, one line.
pub(crate) fn glyph(t: &Paint, s: impl Into<SharedString>, size: i64, ink: gpui::Hsla) -> gpui::Div {
    label(t, s, size, false, ink)
}

/// The button. Borderless at rest unless `bordered`; the hover-cursor border under the pointer,
/// keyboard focus or `cursor`; `selected` fills and bolds the label. Return or Space presses it
/// when focused.
#[derive(IntoElement)]
pub struct Button {
    id: ElementId,
    label: SharedString,
    icon: Option<SharedString>,
    bordered: bool,
    selected: bool,
    cursor: bool,
    on_click: Option<Handler<ClickEvent>>,
}

impl Button {
    pub fn new(label: impl Into<SharedString>) -> Button {
        let label = label.into();
        Button { id: id_for("button", &label), label, icon: None, bordered: false, selected: false, cursor: false, on_click: None }
    }
    /// The element id (focus and state are kept under it); the label by default.
    pub fn id(mut self, id: impl Into<ElementId>) -> Self {
        self.id = id.into();
        self
    }
    /// A glyph before the label (a Nerd Font codepoint, as the shell uses).
    pub fn icon(mut self, glyph: impl Into<SharedString>) -> Self {
        self.icon = Some(glyph.into());
        self
    }
    pub fn bordered(mut self, on: bool) -> Self {
        self.bordered = on;
        self
    }
    pub fn selected(mut self, on: bool) -> Self {
        self.selected = on;
        self
    }
    /// Force the hover-cursor look, as a keyboard-driven panel's cursor does.
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    pub fn on_click(mut self, f: impl Fn(&ClickEvent, &mut Window, &mut App) + 'static) -> Self {
        self.on_click = Some(Rc::new(f));
        self
    }
}

/// Click and keyboard activation for a focusable control.
pub(crate) fn pressable(
    e: gpui::Stateful<gpui::Div>,
    focus: &gpui::FocusHandle,
    on_click: Option<Handler<ClickEvent>>,
) -> gpui::Stateful<gpui::Div> {
    let keys = on_click.clone();
    let e = e.track_focus(focus).on_key_down(move |ev: &KeyDownEvent, window, cx| {
        if tab_keys(ev, window, cx) {
            cx.stop_propagation();
        } else if is_activate(ev) {
            if let Some(f) = &keys {
                f(&ClickEvent::default(), window, cx);
            }
            cx.stop_propagation();
        }
    });
    match on_click {
        Some(f) => e.on_click(move |ev, window, cx| f(ev, window, cx)),
        None => e,
    }
}

impl RenderOnce for Button {
    fn render(self, window: &mut Window, cx: &mut App) -> impl IntoElement {
        let t = Paint::of(cx);
        let tk = t.tokens();
        let c = &tk.controls;
        let sp = &tk.spacing;
        let focus = focus_handle(&self.id, window, cx);
        let hot_now = self.cursor || focus.is_focused(window);
        let rest_border = if self.selected && !c.selected.border.is_none() {
            c.selected.border.clone()
        } else if self.bordered {
            c.normal.border.clone()
        } else {
            BorderSpec::none()
        };
        let rest = Look::new(if self.selected { c.selected.fill } else { Rgba::TRANSPARENT }, rest_border);
        let hot = Look::new(c.hover.fill, c.hover.border.clone());
        let mut looks = vec![&c.hover.border, &c.selected.border, &c.focus.border];
        if self.bordered {
            looks.push(&c.normal.border);
        }
        let [rt, rr, rb, rl] = reserve(&looks);

        let ink = if self.selected { color(c.selected.text) } else { t.fg() };
        let base = div()
            .id(self.id)
            .flex()
            .flex_row()
            .flex_shrink_0()
            .items_center()
            .justify_center()
            .gap(px(sp.control_gap as f32))
            .pt(px(sp.control_padding_y as f32 + rt))
            .pb(px(sp.control_padding_y as f32 + rb))
            .pl(px(sp.control_padding_x as f32 + rl))
            .pr(px(sp.control_padding_x as f32 + rr));
        let mut e = chrome(base, &t, &rest, &hot, hot_now, t.radius());
        if let Some(icon) = self.icon {
            e = e.child(glyph(&t, icon, tk.font.icon, ink));
        }
        if !self.label.is_empty() {
            e = e.child(label(&t, self.label, tk.font.body, self.selected, ink));
        }
        pressable(e, &focus, self.on_click)
    }
}

/// A square icon-only button (the shell's PanelActionButton): odd-sized so the glyph centers on
/// a pixel.
#[derive(IntoElement)]
pub struct IconButton {
    id: ElementId,
    glyph: SharedString,
    bordered: bool,
    cursor: bool,
    on_click: Option<Handler<ClickEvent>>,
}

impl IconButton {
    pub fn new(glyph: impl Into<SharedString>) -> IconButton {
        let glyph = glyph.into();
        IconButton { id: id_for("icon-button", &glyph), glyph, bordered: false, cursor: false, on_click: None }
    }
    pub fn id(mut self, id: impl Into<ElementId>) -> Self {
        self.id = id.into();
        self
    }
    pub fn bordered(mut self, on: bool) -> Self {
        self.bordered = on;
        self
    }
    pub fn cursor(mut self, on: bool) -> Self {
        self.cursor = on;
        self
    }
    pub fn on_click(mut self, f: impl Fn(&ClickEvent, &mut Window, &mut App) + 'static) -> Self {
        self.on_click = Some(Rc::new(f));
        self
    }
}

impl RenderOnce for IconButton {
    fn render(self, window: &mut Window, cx: &mut App) -> impl IntoElement {
        let t = Paint::of(cx);
        let tk = t.tokens();
        let c = &tk.controls;
        let icon = tk.font.icon;
        let size = ((t.space(22.0)).max((icon + tk.spacing.sm * 2) as f32).round() as i64 | 1) as f32;
        let focus = focus_handle(&self.id, window, cx);
        let hot_now = self.cursor || focus.is_focused(window);
        let rest = Look::new(Rgba::TRANSPARENT, if self.bordered { c.normal.border.clone() } else { BorderSpec::none() });
        // PanelActionButton shows the hover border only when bordered.
        let hot = Look::new(c.hover.fill, if self.bordered { c.hover.border.clone() } else { BorderSpec::none() });
        let base = div().id(self.id).flex().flex_shrink_0().items_center().justify_center().size(px(size));
        let e = chrome(base, &t, &rest, &hot, hot_now, t.radius()).child(glyph(&t, self.glyph, icon, t.fg()));
        let _ = GROUP;
        pressable(e, &focus, self.on_click)
    }
}

/// A row of bordered buttons, `md` apart, one selected: a segmented choice. Left and Right move
/// the choice when one of its buttons has focus.
#[derive(IntoElement)]
pub struct ButtonGroup {
    id: ElementId,
    options: Vec<(SharedString, SharedString)>,
    value: SharedString,
    cursor: Option<usize>,
    on_choose: Option<Handler<SharedString>>,
}

impl ButtonGroup {
    /// `options` are (value, label) pairs.
    pub fn new<V: Into<SharedString>, L: Into<SharedString>>(options: impl IntoIterator<Item = (V, L)>, value: impl Into<SharedString>) -> ButtonGroup {
        let options: Vec<(SharedString, SharedString)> = options.into_iter().map(|(v, l)| (v.into(), l.into())).collect();
        let key = options.iter().map(|(v, _)| v.as_ref()).collect::<Vec<_>>().join("|");
        ButtonGroup { id: id_for("button-group", &key), options, value: value.into(), cursor: None, on_choose: None }
    }
    pub fn id(mut self, id: impl Into<ElementId>) -> Self {
        self.id = id.into();
        self
    }
    /// Force the hover-cursor look on one option.
    pub fn cursor(mut self, index: Option<usize>) -> Self {
        self.cursor = index;
        self
    }
    /// Called with the chosen option's value.
    pub fn on_choose(mut self, f: impl Fn(&SharedString, &mut Window, &mut App) + 'static) -> Self {
        self.on_choose = Some(Rc::new(f));
        self
    }
}

impl RenderOnce for ButtonGroup {
    fn render(self, _window: &mut Window, cx: &mut App) -> impl IntoElement {
        let t = Paint::of(cx);
        let gap = t.tokens().spacing.md as f32;
        let mut r = div().flex().flex_row().gap(px(gap));
        let values: Rc<Vec<SharedString>> = Rc::new(self.options.iter().map(|(v, _)| v.clone()).collect());
        let current = values.iter().position(|v| *v == self.value);
        for (i, (value, text)) in self.options.into_iter().enumerate() {
            let mut b = Button::new(text)
                .id(ElementId::NamedChild(std::sync::Arc::new(self.id.clone()), value.to_string().into()))
                .bordered(true)
                .selected(value == self.value)
                .cursor(self.cursor == Some(i));
            if let Some(f) = self.on_choose.clone() {
                let v = value.clone();
                b = b.on_click(move |_, window, cx| f(&v, window, cx));
            }
            r = r.child(b);
        }
        // Arrow keys walk the choice (Qt's ButtonGroup), bubbling up from the focused button.
        if let Some(f) = self.on_choose {
            r = r.on_key_down(move |ev: &KeyDownEvent, window, cx| {
                let step: isize = match ev.keystroke.key.as_str() {
                    "left" | "h" => -1,
                    "right" | "l" => 1,
                    _ => return,
                };
                let Some(at) = current else { return };
                let next = (at as isize + step).clamp(0, values.len() as isize - 1) as usize;
                if next != at {
                    f(&values[next], window, cx);
                }
                cx.stop_propagation();
            });
        }
        r
    }
}
