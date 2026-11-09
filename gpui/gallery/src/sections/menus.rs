//! The `menus` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{ContextMenu, KeyCombo, Keycap, Menu, MenuItem, ShortcutSheet, Tooltip};

use crate::Gallery;

/// What the section's menus change.
pub struct State {
    pub sidebar: bool,
    pub wrap: bool,
    pub sort: usize,
    pub ran: Option<&'static str>,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State { sidebar: true, wrap: false, sort: 0, ran: None }
    }
}

fn flip(f: fn(&mut State), cx: &mut Context<Gallery>) -> impl Fn(&ClickEvent, &mut Window, &mut App) + 'static {
    cx.listener(move |g: &mut Gallery, _: &ClickEvent, _, cx| {
        f(&mut g.more.menus);
        cx.notify();
    })
}

pub fn build(g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let sp = &t.tokens().spacing;
    let m = &g.more.menus;
    let run = |label: &'static str, cx: &mut Context<Gallery>| {
        MenuItem::new(label).on_click(cx.listener(move |g: &mut Gallery, _, _, cx| {
            g.more.menus.ran = Some(label);
            cx.notify();
        }))
    };

    // A menu held open: section labels, leading glyphs, shortcut hints, a check, a disabled
    // row, separators, a submenu row, and the keyboard cursor on "Open…".
    let menu = Menu::new()
        .id("gallery-menu")
        .section("File")
        .item(run("New", cx).icon("\u{f15b}").shortcut("Ctrl+N"))
        .item(run("Open…", cx).icon("\u{f07b}").shortcut("Ctrl+O").cursor(true))
        .item(run("Save", cx).icon("\u{f0c7}").shortcut("Ctrl+S"))
        .item(MenuItem::new("Duplicate").icon("\u{f0c5}").disabled(true))
        .separator()
        .section("View")
        .item(MenuItem::new("Sidebar").check(m.sidebar).on_click(flip(|m| m.sidebar ^= true, cx)))
        .item(MenuItem::new("Word wrap").check(m.wrap).on_click(flip(|m| m.wrap ^= true, cx)))
        .separator()
        .item(MenuItem::new("Export").icon("\u{f064}").submenu(Menu::new().item(run("PDF", cx)).item(run("PNG", cx))));

    // A context menu held open: radio choices and a destructive row.
    let context = ContextMenu::new()
        .id("gallery-context-menu")
        .item(run("Cut", cx).shortcut("Ctrl+X"))
        .item(run("Copy", cx).shortcut("Ctrl+C"))
        .item(run("Paste", cx).shortcut("Ctrl+V"))
        .separator()
        .item(MenuItem::new("By name").radio(m.sort == 0).on_click(flip(|m| m.sort = 0, cx)))
        .item(MenuItem::new("By date").radio(m.sort == 1).on_click(flip(|m| m.sort = 1, cx)))
        .separator()
        .item(run("Delete", cx).shortcut("Del").destructive(true));

    let keys = ["Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K"];
    vec![
        div()
            .flex()
            .flex_row()
            .items_start()
            .gap(px(sp.huge as f32))
            .child(menu)
            .child(context)
            .child(Tooltip::new("Save").shortcut("Ctrl+S"))
            .into_any_element(),
        div()
            .flex()
            .flex_row()
            .items_center()
            .gap(px(sp.lg as f32))
            .children(keys.iter().map(|k| Keycap::new(*k)))
            .child(KeyCombo::new("Ctrl+Shift+P"))
            .into_any_element(),
        ShortcutSheet::new()
            .group("File", [("Ctrl+N", "New file"), ("Ctrl+O", "Open"), ("Ctrl+S", "Save")])
            .group("View", [("Ctrl+B", "Sidebar"), ("Ctrl+Shift+P", "Commands"), ("F11", "Full screen")])
            .into_any_element(),
    ]
}
