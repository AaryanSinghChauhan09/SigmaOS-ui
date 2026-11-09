//! Menu, Tabs and IconRail driven by clicks and keys, as `qt/tests/tst_navigation.qml` drives
//! Qt's and the Rune gallery's menu_rows_run, menu_walks_with_vim_keys_and_arrows,
//! tabs_and_rail_choose and choosers_follow_keys_as_in_qt drive Rune's.

mod common;

use common::{focus_first, host, take_log};
use gpui::{IntoElement, Modifiers, ParentElement, Styled, TestAppContext, VisualTestContext, div};
use omarchy_gpui::ui::{IconRail, KeyCombo, Menu, MenuItem, Tabs, Tooltip, key_combo_width, key_label, split_combo};

/// The navigation section's menu, held open: a disabled row and a submenu row among plain ones.
fn menu_host(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<bool>>, &mut VisualTestContext) {
    host(cx, true, |h, _, cx| {
        let run = |label: &'static str, cx: &mut gpui::Context<common::Host<bool>>| {
            MenuItem::new(label).on_click(cx.listener(move |h: &mut common::Host<bool>, _, _, _| h.log.push(label.into())))
        };
        let menu = Menu::new()
            .item(run("New", cx).icon("\u{f15b}"))
            .item(run("Open…", cx))
            .item(run("Save", cx))
            .item(run("Duplicate", cx).disabled(true))
            .separator()
            .item(MenuItem::new("Sidebar").check(h.state).on_click(cx.listener(|h: &mut common::Host<bool>, _, _, cx| {
                h.state ^= true;
                cx.notify();
            })))
            .item(MenuItem::new("Export").submenu(Menu::new().item(run("PDF", cx)).item(run("PNG", cx))));
        div().size_full().child(menu).into_any_element()
    })
}

fn click(cx: &mut VisualTestContext, selector: &'static str) {
    let b = cx.debug_bounds(selector).unwrap_or_else(|| panic!("{selector} is drawn"));
    cx.simulate_click(b.center(), Modifiers::none());
    cx.run_until_parked();
}

fn shows(cx: &mut VisualTestContext, selector: &'static str) -> bool {
    cx.run_until_parked();
    cx.debug_bounds(selector).is_some()
}

#[gpui::test]
fn menu_rows_run_on_click(cx: &mut TestAppContext) {
    let (view, cx) = menu_host(cx);
    click(cx, "menu-row-New");
    assert_eq!(take_log(&view, cx), ["New"]);
    click(cx, "menu-row-Sidebar");
    assert!(!view.read_with(cx, |h, _| h.state), "a check row flips");
    click(cx, "menu-row-Duplicate");
    assert!(take_log(&view, cx).is_empty(), "a disabled row doesn't run");
    click(cx, "menu-row-Export");
    assert!(shows(cx, "menu-row-PDF"), "a press on a submenu row opens it");
    click(cx, "menu-row-PNG");
    assert_eq!(take_log(&view, cx), ["PNG"]);
    assert!(!shows(cx, "menu-row-PDF"), "running a row closes the submenu");
}

#[gpui::test]
fn menu_walks_with_vim_keys_and_arrows(cx: &mut TestAppContext) {
    let (view, cx) = menu_host(cx);
    focus_first(cx);
    // The first key lands on the first row; Return, Enter and Space run the row under it.
    cx.simulate_keystrokes("j j enter");
    assert_eq!(take_log(&view, cx), ["Open…"]);
    cx.simulate_keystrokes("down space");
    assert_eq!(take_log(&view, cx), ["Save"]);
    cx.simulate_keystrokes("k kp_enter");
    assert_eq!(take_log(&view, cx), ["Open…"]);
    // Skips the disabled row.
    cx.simulate_keystrokes("j j enter");
    assert!(!view.read_with(cx, |h, _| h.state), "j j lands on Sidebar past Duplicate");
    // Wraps past the last row, and back past the first.
    cx.simulate_keystrokes("down down enter");
    assert_eq!(take_log(&view, cx), ["New"]);
    cx.simulate_keystrokes("up home enter");
    assert_eq!(take_log(&view, cx), ["New"]);
    cx.simulate_keystrokes("end k enter");
    assert!(view.read_with(cx, |h, _| h.state), "End then k is Sidebar");
}

#[gpui::test]
fn menu_submenu_opens_right_and_closes_left(cx: &mut TestAppContext) {
    let (view, cx) = menu_host(cx);
    focus_first(cx);
    cx.simulate_keystrokes("end l");
    assert!(shows(cx, "menu-row-PDF"), "l opens the submenu");
    cx.simulate_keystrokes("j enter");
    assert_eq!(take_log(&view, cx), ["PNG"], "its first row is under the cursor, j walks it");
    assert!(!shows(cx, "menu-row-PDF"), "running a row closes the submenu");
    cx.simulate_keystrokes("end right");
    assert!(shows(cx, "menu-row-PDF"));
    cx.simulate_keystrokes("h");
    assert!(!shows(cx, "menu-row-PDF"), "h goes back out");
    cx.simulate_keystrokes("right");
    assert!(shows(cx, "menu-row-PDF"), "and the held menu has the keys again");
    cx.simulate_keystrokes("left");
    assert!(!shows(cx, "menu-row-PDF"), "so does Left");
    cx.simulate_keystrokes("right escape");
    assert!(!shows(cx, "menu-row-PDF"), "and Escape");
}

#[gpui::test]
fn floating_menu_closes_on_escape_and_after_running(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, true, |h, _, cx| {
        let mut root = div().size_full().child(div().relative().w(gpui::px(80.)).h(gpui::px(20.)));
        if h.state {
            let close = cx.listener(|h: &mut common::Host<bool>, _: &(), _, cx| {
                h.state = false;
                h.log.push("closed".into());
                cx.notify();
            });
            let run = cx.listener(|h: &mut common::Host<bool>, _, _, _| h.log.push("Copy".into()));
            root = root.child(div().relative().child(Menu::new().item(MenuItem::new("Copy").on_click(run)).popup().on_close(close)));
        }
        root.into_any_element()
    });
    cx.run_until_parked();
    // It took the keyboard when it opened.
    cx.simulate_keystrokes("j enter");
    assert_eq!(take_log(&view, cx), ["Copy", "closed"], "running a row closes a floating menu");
    view.update(cx, |h, cx| {
        h.state = true;
        cx.notify();
    });
    cx.run_until_parked();
    cx.simulate_keystrokes("escape");
    assert_eq!(take_log(&view, cx), ["closed"]);
}

#[gpui::test]
fn tabs_choose_on_click_and_keys(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, 1usize, |h, _, cx| {
        Tabs::new(["General", "Display", "Audio", "Network"], h.state)
            .on_choose(cx.listener(|h: &mut common::Host<usize>, i: &usize, _, cx| {
                h.state = *i;
                h.log.push(format!("{i}"));
                cx.notify();
            }))
            .into_any_element()
    });
    click(cx, "tab-Network");
    assert_eq!(view.read_with(cx, |h, _| h.state), 3);
    assert_eq!(take_log(&view, cx).len(), 1, "one activation");
    view.update(cx, |h, cx| {
        h.state = 1;
        cx.notify();
    });
    for (key, want) in [("right", 2), ("l", 3), ("right", 3), ("h", 2), ("left", 1), ("home", 0), ("left", 0), ("end", 3), ("up", 3), ("down", 3)] {
        cx.simulate_keystrokes(key);
        assert_eq!(view.read_with(cx, |h, _| h.state), want, "tabs after {key}");
    }
}

#[gpui::test]
fn rail_chooses_on_click_and_keys(cx: &mut TestAppContext) {
    let items = [("\u{f07b}", "Files"), ("\u{f002}", "Search"), ("\u{f126}", "Source control"), ("\u{f188}", "Debug"), ("\u{f013}", "Settings")];
    let (view, cx) = host(cx, 0usize, move |h, _, cx| {
        IconRail::new(items, h.state)
            .on_choose(cx.listener(|h: &mut common::Host<usize>, i: &usize, _, cx| {
                h.state = *i;
                cx.notify();
            }))
            .into_any_element()
    });
    click(cx, "rail-4");
    assert_eq!(view.read_with(cx, |h, _| h.state), 4, "a press on the last button");
    view.update(cx, |h, cx| {
        h.state = 0;
        cx.notify();
    });
    for (key, want) in [("down", 1), ("j", 2), ("k", 1), ("up", 0), ("up", 0), ("end", 4), ("down", 4), ("home", 0), ("right", 0), ("left", 0)] {
        cx.simulate_keystrokes(key);
        assert_eq!(view.read_with(cx, |h, _| h.state), want, "rail after {key}");
    }
}

#[test]
fn keys_split_and_name_as_in_qt() {
    assert_eq!(split_combo("Ctrl+Shift+S"), ["Ctrl", "Shift", "S"]);
    assert_eq!(split_combo("Ctrl++"), ["Ctrl", "+"]);
    let labels: Vec<String> = ["super", "ctrl", "Shift", "enter", "backspace", "up", "esc", "k", "F11"].iter().map(|k| key_label(k)).collect();
    assert_eq!(labels, ["Super", "Ctrl", "\u{21e7}", "\u{23ce}", "\u{232b}", "\u{2191}", "Esc", "K", "F11"]);
}

#[gpui::test]
fn tooltip_shortcut_draws_a_key_combo(cx: &mut TestAppContext) {
    let (_view, cx) = host(cx, (), |_, _, _| {
        div().child(Tooltip::new("Save").shortcut("Ctrl+S")).child(KeyCombo::new("Ctrl+S")).into_any_element()
    });
    // Two caps: the combo's width is the caps' plus the gap between them.
    let w = cx.update(|window, cx| {
        let t = omarchy_gpui::Paint::of(cx);
        key_combo_width(window, &t, "Ctrl+S")
    });
    assert!(w > 0.0);
}
