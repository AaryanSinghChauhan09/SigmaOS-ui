//! The `menus` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{ContextMenu, KeyCombo, Keycap, Menu, MenuItem, ShortcutSheet, Tooltip};
use rune::{El, row};

use crate::State;

/// What the section's menus change.
pub struct Menus {
    pub sidebar: bool,
    pub wrap: bool,
    pub sort: usize,
    pub ran: Option<&'static str>,
}

impl Menus {
    pub fn init() -> Menus {
        Menus { sidebar: true, wrap: false, sort: 0, ran: None }
    }
}

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let m = &s.menus;
    let run = |label: &'static str| MenuItem::new(label).on_press(move |s: &mut State| s.menus.ran = Some(label));

    // A menu held open: section labels, leading glyphs, shortcut hints, a check, a disabled
    // row, separators, a submenu row, and the keyboard cursor on "Open…".
    let menu = Menu::new()
        .section("File")
        .item(run("New").icon("\u{f15b}").shortcut("Ctrl+N").key("menu-new"))
        .item(run("Open…").icon("\u{f07b}").shortcut("Ctrl+O").cursor(true))
        .item(run("Save").icon("\u{f0c7}").shortcut("Ctrl+S"))
        .item(MenuItem::new("Duplicate").icon("\u{f0c5}").disabled(true))
        .separator()
        .section("View")
        .item(MenuItem::new("Sidebar").check(m.sidebar).key("menu-sidebar").on_press(|s: &mut State| s.menus.sidebar ^= true))
        .item(MenuItem::new("Word wrap").check(m.wrap).on_press(|s: &mut State| s.menus.wrap ^= true))
        .separator()
        .item(MenuItem::new("Export").icon("\u{f064}").submenu(true))
        .build(t);

    // A context menu held open: radio choices and a destructive row.
    let context = ContextMenu::new()
        .item(run("Cut").shortcut("Ctrl+X"))
        .item(run("Copy").shortcut("Ctrl+C"))
        .item(run("Paste").shortcut("Ctrl+V"))
        .separator()
        .item(MenuItem::new("By name").radio(m.sort == 0).on_press(|s: &mut State| s.menus.sort = 0))
        .item(MenuItem::new("By date").radio(m.sort == 1).on_press(|s: &mut State| s.menus.sort = 1))
        .separator()
        .item(run("Delete").shortcut("Del").destructive(true))
        .build(t);

    let keys = ["Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K"];
    vec![
        row()
            .items_start()
            .gap(sp.huge as f32)
            .child(menu)
            .child(context)
            .child(Tooltip::new("Save").shortcut("Ctrl+S").build(t)),
        row()
            .items_center()
            .gap(sp.lg as f32)
            .children(keys.iter().map(|k| Keycap::new(*k).build(t)))
            .child(KeyCombo::new("Ctrl+Shift+P").build(t)),
        ShortcutSheet::new()
            .group("File", [("Ctrl+N", "New file"), ("Ctrl+O", "Open"), ("Ctrl+S", "Save")])
            .group("View", [("Ctrl+B", "Sidebar"), ("Ctrl+Shift+P", "Commands"), ("F11", "Full screen")])
            .build(t),
    ]
}
