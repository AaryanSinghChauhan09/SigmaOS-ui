//! Surfaces: Card (the shell's panels and popups), section headers, separators, menu rows,
//! tooltips and the confirm dialog.

use omarchy_theme::Rgba;
use rune::{El, col, el, row, text};

use super::Attach;
use crate::{Theme, border, color};

/// A caption-sized bold header, foreground darkened 1.4: what heads a panel section.
pub fn section_header(t: &Theme, label: impl Into<String>) -> El {
    let size = t.tokens().font.caption;
    text(label.into())
        .font(t.text_style_bold(size))
        .color(t.fg_darker(1.4))
        .pt((size as f32 * 0.15).ceil())
        .role("heading")
}

/// A 1px rule in the foreground at 12%.
pub fn separator(t: &Theme) -> El {
    el().w_full().h(1.0).fill(color(t.tokens().palette.foreground.with_alpha(0.12)))
}

/// A raised surface: [popups] background and border (Hyprland's active border by default),
/// Hyprland's rounding, panel padding, children panelGap apart.
pub struct Card {
    title: String,
    width: Option<f32>,
    children: Vec<El>,
}

impl Card {
    pub fn new() -> Card {
        Card { title: String::new(), width: None, children: Vec::new() }
    }
    pub fn title(mut self, title: impl Into<String>) -> Self {
        self.title = title.into();
        self
    }
    pub fn width(mut self, w: f32) -> Self {
        self.width = Some(w);
        self
    }
    pub fn child(mut self, e: El) -> Self {
        self.children.push(e);
        self
    }
    pub fn children(mut self, es: impl IntoIterator<Item = El>) -> Self {
        self.children.extend(es);
        self
    }

    pub fn build(self, t: &Theme) -> El {
        let tk = t.tokens();
        let pop = &tk.surfaces.popups;
        let bw = pop.border.widths;
        let pad = tk.spacing.panel_padding as f32;
        let mut c = col()
            .gap(tk.spacing.panel_gap as f32)
            .pt(pad + bw[0] as f32)
            .pr(pad + bw[1] as f32)
            .pb(pad + bw[2] as f32)
            .pl(pad + bw[3] as f32)
            .fill(color(pop.background))
            .radius(t.radius());
        c = border(c, &pop.border);
        if let Some(w) = self.width {
            c = c.w(w);
        }
        if !self.title.is_empty() {
            c = c.child(section_header(t, self.title));
        }
        c.child(col().w_full().gap(tk.spacing.panel_gap as f32).children(self.children))
    }
}

impl Default for Card {
    fn default() -> Self {
        Card::new()
    }
}

/// One row of a menu or list: label, a check in the accent when `checked`, the hover fill under
/// the pointer or when `highlighted` (a keyboard-walked menu).
pub struct MenuRow {
    label: String,
    checked: bool,
    highlighted: bool,
    press: Option<Attach>,
}

impl MenuRow {
    pub fn new(label: impl Into<String>) -> MenuRow {
        MenuRow { label: label.into(), checked: false, highlighted: false, press: None }
    }
    pub fn checked(mut self, on: bool) -> Self {
        self.checked = on;
        self
    }
    pub fn highlighted(mut self, on: bool) -> Self {
        self.highlighted = on;
        self
    }
    pub fn on_press<S: 'static>(mut self, f: impl Fn(&mut S) + 'static) -> Self {
        self.press = Some(Box::new(move |e: El| e.on_press(f)));
        self
    }

    pub fn build(self, t: &Theme) -> El {
        let tk = t.tokens();
        let sp = &tk.spacing;
        let hover = color(tk.controls.hover.fill);
        let mut r = row()
            .w_full()
            .h(sp.popup_row_height as f32)
            .items_center()
            .justify_between()
            .px(sp.control_padding_x as f32)
            .radius(t.radius())
            .fill(if self.highlighted { hover } else { color(Rgba::TRANSPARENT) })
            .hover(hover)
            .role("menuitem")
            .label(self.label.clone())
            .child(text(self.label).font(t.text_style(tk.font.body)).color(color(tk.surfaces.popups.text)).single_line());
        if self.checked {
            r = r.checked(true).child(super::glyph(t, "\u{2713}", tk.font.body, t.accent()));
        }
        match self.press {
            Some(attach) => attach(r),
            None => r,
        }
    }
}

/// A tooltip as drawn (the app owns when it shows): [tooltip] surface, small text.
pub struct Tooltip {
    label: String,
}

impl Tooltip {
    pub fn new(label: impl Into<String>) -> Tooltip {
        Tooltip { label: label.into() }
    }

    pub fn build(self, t: &Theme) -> El {
        let tk = t.tokens();
        let s = &tk.surfaces.tooltip;
        let sp = &tk.spacing;
        let bw = s.border.max_width() as f32;
        let e = row()
            .px(sp.control_padding_x as f32 + bw)
            .py(sp.control_padding_y as f32 + bw)
            .fill(color(s.background))
            .child(text(self.label).font(t.text_style(tk.font.body_small)).color(color(s.text)));
        border(e, &s.border)
    }
}

/// A yes/no prompt card: message, then Cancel and a destructive Confirm, one of them selected
/// (arrow keys move it in the shell; here the app owns `selected`). A [`super::Dialog`] preset:
/// no title, the message in `title` size.
pub struct ConfirmDialog {
    message: String,
    cancel: String,
    confirm: String,
    selected: usize,
    width: Option<f32>,
    on_cancel: Option<Attach>,
    on_confirm: Option<Attach>,
}

impl ConfirmDialog {
    pub fn new(message: impl Into<String>) -> ConfirmDialog {
        ConfirmDialog {
            message: message.into(),
            cancel: "Cancel".into(),
            confirm: "Confirm".into(),
            selected: 1,
            width: None,
            on_cancel: None,
            on_confirm: None,
        }
    }
    pub fn labels(mut self, cancel: impl Into<String>, confirm: impl Into<String>) -> Self {
        self.cancel = cancel.into();
        self.confirm = confirm.into();
        self
    }
    /// 0 for Cancel, 1 for Confirm (the default).
    pub fn selected(mut self, index: usize) -> Self {
        self.selected = index;
        self
    }
    pub fn width(mut self, w: f32) -> Self {
        self.width = Some(w);
        self
    }
    pub fn on_cancel<S: 'static>(mut self, f: impl Fn(&mut S) + 'static) -> Self {
        self.on_cancel = Some(Box::new(move |e: El| e.on_press(f)));
        self
    }
    pub fn on_confirm<S: 'static>(mut self, f: impl Fn(&mut S) + 'static) -> Self {
        self.on_confirm = Some(Box::new(move |e: El| e.on_press(f)));
        self
    }

    pub fn build(self, t: &Theme) -> El {
        let tk = t.tokens();
        let message = text(self.message).font(t.text_style(tk.font.title)).color(t.fg()).wrap();
        let mut d = super::Dialog::new("")
            .body(message)
            .labels(self.cancel, self.confirm)
            .destructive(true)
            .selected(self.selected);
        if let Some(w) = self.width {
            d = d.width(w);
        }
        d.attach(self.on_cancel, self.on_confirm).build(t)
    }
}
