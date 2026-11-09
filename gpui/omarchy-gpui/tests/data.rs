//! Table, Tree, Sidebar, SplitView and Calendar driven by clicks and keys, as `qt/tests/tst_data.qml`
//! drives Qt's and the Rune gallery's data_selects_and_folds, data_keys_as_in_qt and
//! layout_picks_drags_and_steps drive Rune's. The app owns the state; the components ask.

mod common;

use common::{focus_first, host};
use gpui::{IntoElement, Modifiers, MouseButton, ParentElement, Point, SharedString, Styled, TestAppContext, VisualTestContext, div, point, px};
use omarchy_gpui::ui::{Calendar, Column, Date, Sidebar, SidebarItem, SidebarSection, SplitPane, SplitView, Table, Tree, TreeNode};

/// `e` inset from the window's corner, where the test pointer rests: a row under it would take
/// the cursor on hover, as the pointer does in Qt.
fn away(e: gpui::AnyElement) -> gpui::AnyElement {
    div().p(px(40.)).child(e).into_any_element()
}

trait Pipe: Sized {
    fn pipe<R>(self, f: impl FnOnce(Self) -> R) -> R {
        f(self)
    }
}
impl<T> Pipe for T {}

fn center(cx: &mut VisualTestContext, selector: &'static str) -> Point<gpui::Pixels> {
    cx.debug_bounds(selector).unwrap_or_else(|| panic!("no element {selector}")).center()
}

fn click(cx: &mut VisualTestContext, selector: &'static str) {
    let at = center(cx, selector);
    cx.simulate_click(at, Modifiers::none());
    cx.run_until_parked();
}

#[derive(Default)]
struct Files {
    selected: Option<usize>,
    sort: (usize, bool),
    picks: usize,
}

fn table(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<Files>>, &mut VisualTestContext) {
    host(cx, Files { sort: (0, true), ..Default::default() }, |h, _, cx| {
        Table::new([Column::new("Name"), Column::new("Kind").width(90.0), Column::new("Size").width(70.0).right()])
            .id("files")
            .rows([["assets", "Folder", "—"], ["Cargo.toml", "TOML", "1.2 KB"], ["README.md", "Markdown", "4.8 KB"], ["src", "Folder", "—"], ["target", "Folder", "—"]])
            .sort(h.state.sort.0, h.state.sort.1)
            .selected(h.state.selected)
            .width(480.0)
            .on_select(cx.listener(|h, r: &usize, _, cx| {
                h.state.selected = Some(*r);
                h.state.picks += 1;
                cx.notify();
            }))
            .on_sort(cx.listener(|h, c: &usize, _, cx| {
                h.state.sort = if h.state.sort.0 == *c { (*c, !h.state.sort.1) } else { (*c, true) };
                cx.notify();
            }))
            .into_any_element()
    })
}

#[gpui::test]
fn table_selects_on_click_and_sorts_on_header(cx: &mut TestAppContext) {
    let (view, cx) = table(cx);
    click(cx, "files-3");
    assert_eq!(view.read_with(cx, |h, _| h.state.selected), Some(3));
    click(cx, "files-col-0");
    assert_eq!(view.read_with(cx, |h, _| h.state.sort), (0, false), "the sorted column's header flips the order");
    click(cx, "files-col-1");
    assert_eq!(view.read_with(cx, |h, _| h.state.sort), (1, true));
}

#[gpui::test]
fn table_keys_move_the_cursor_and_choose(cx: &mut TestAppContext) {
    let (view, cx) = table(cx);
    focus_first(cx);
    let selected = |cx: &mut VisualTestContext| view.read_with(cx, |h, _| h.state.selected);
    cx.simulate_keystrokes("down enter");
    assert_eq!(selected(cx), Some(0), "the first key lands on the first row");
    cx.simulate_keystrokes("down enter");
    assert_eq!(selected(cx), Some(1));
    cx.simulate_keystrokes("end down space");
    assert_eq!(selected(cx), Some(4), "stops at the last row");
    cx.simulate_keystrokes("home up enter");
    assert_eq!(selected(cx), Some(0), "stops at the first row");
    cx.simulate_keystrokes("pagedown enter");
    assert_eq!(selected(cx), Some(4), "a page is the whole table when it isn't scrolled");
    assert_eq!(view.read_with(cx, |h, _| h.state.picks), 5);
}

struct Files2 {
    expanded: Vec<SharedString>,
    selected: Option<SharedString>,
}

fn tree(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<Files2>>, &mut VisualTestContext) {
    let state = Files2 { expanded: vec!["root".into(), "qt".into(), "spec".into()], selected: None };
    host(cx, state, |h, _, cx| {
        Tree::new([TreeNode::new("root", "omarchy-ui").children([
            TreeNode::new("qt", "qt").children([TreeNode::new("qt/button", "Button.qml"), TreeNode::new("qt/table", "Table.qml")]),
            TreeNode::new("rune", "rune").children([TreeNode::new("rune/lib", "lib.rs")]),
            TreeNode::new("spec", "spec").children([TreeNode::new("spec/tokens", "tokens.md")]),
        ])])
        .id("tree")
        .expanded(h.state.expanded.clone())
        .selected(h.state.selected.clone())
        .width(220.0)
        .on_toggle(cx.listener(|h, id: &SharedString, _, cx| {
            match h.state.expanded.iter().position(|e| e == id) {
                Some(i) => {
                    h.state.expanded.remove(i);
                }
                None => h.state.expanded.push(id.clone()),
            }
            cx.notify();
        }))
        .on_select(cx.listener(|h, id: &SharedString, _, cx| {
            h.state.selected = Some(id.clone());
            cx.notify();
        }))
        .into_any_element()
        .pipe(away)
    })
}

fn open(view: &gpui::Entity<common::Host<Files2>>, cx: &mut VisualTestContext, id: &str) -> bool {
    view.read_with(cx, |h, _| h.state.expanded.iter().any(|e| e == id))
}

#[gpui::test]
fn tree_folds_and_selects_on_click(cx: &mut TestAppContext) {
    let (view, cx) = tree(cx);
    click(cx, "tree-qt-toggle");
    assert!(!open(&view, cx, "qt"), "the chevron folds qt");
    assert!(cx.debug_bounds("tree-qt/button").is_none(), "its children are gone");
    click(cx, "tree-spec/tokens");
    assert_eq!(view.read_with(cx, |h, _| h.state.selected.clone()).as_deref(), Some("spec/tokens"));
}

#[gpui::test]
fn tree_keys_fold_unfold_and_choose(cx: &mut TestAppContext) {
    let (view, cx) = tree(cx);
    focus_first(cx);
    cx.simulate_keystrokes("down enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.selected.clone()).as_deref(), Some("root"));
    cx.simulate_keystrokes("down left");
    assert!(!open(&view, cx, "qt"), "Left folds an open branch");
    cx.simulate_keystrokes("left");
    assert!(!open(&view, cx, "qt"), "and does nothing to a folded one");
    cx.simulate_keystrokes("right");
    assert!(open(&view, cx, "qt"), "Right unfolds it");
    cx.simulate_keystrokes("right");
    assert!(open(&view, cx, "qt"), "and leaves an open one");
    cx.simulate_keystrokes("down right");
    assert_eq!(view.read_with(cx, |h, _| h.state.expanded.len()), 3, "a leaf doesn't fold");
    cx.simulate_keystrokes("enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.selected.clone()).as_deref(), Some("qt/button"));
}

struct Layout {
    place: SharedString,
    width: f32,
    day: Date,
    month: (i32, u32),
}

fn layout(cx: &mut TestAppContext) -> (gpui::Entity<common::Host<Layout>>, &mut VisualTestContext) {
    let state = Layout { place: "recent".into(), width: 200.0, day: Date::new(2026, 11, 14), month: (2026, 11) };
    host(cx, state, |h, _, cx| {
        let sidebar = Sidebar::new([
            SidebarSection::new("Library", [SidebarItem::new("all", "All files", "\u{f024b}"), SidebarItem::new("recent", "Recent", "\u{f02da}")]),
            SidebarSection::new("Places", [SidebarItem::new("documents", "Documents", "\u{f0219}"), SidebarItem::new("downloads", "Downloads", "\u{f01da}")]),
        ])
        .id("places")
        .selected(Some(h.state.place.clone()))
        .on_select(cx.listener(|h, id: &SharedString, _, cx| {
            h.state.place = id.clone();
            cx.notify();
        }));
        let split = SplitView::new("split")
            .pane(SplitPane::new(sidebar).size(h.state.width).min(120.0))
            .pane(SplitPane::new(div()))
            .on_resize(cx.listener(|h, (_, w): &(usize, f32), _, cx| {
                h.state.width = *w;
                cx.notify();
            }));
        let calendar = Calendar::new(h.state.month.0, h.state.month.1)
            .id("cal")
            .today(Date::new(2026, 11, 8))
            .selected(Some(h.state.day))
            .on_pick(cx.listener(|h, d: &Date, _, cx| {
                h.state.day = *d;
                cx.notify();
            }))
            .on_month(cx.listener(|h, m: &(i32, u32), _, cx| {
                h.state.month = *m;
                cx.notify();
            }));
        away(div().flex().flex_col().child(div().w(px(720.)).h(px(220.)).child(split)).child(calendar).into_any_element())
    })
}

#[gpui::test]
fn layout_picks_drags_and_steps(cx: &mut TestAppContext) {
    let (view, cx) = layout(cx);
    click(cx, "places-downloads");
    assert_eq!(view.read_with(cx, |h, _| h.state.place.clone()), "downloads");

    let at = center(cx, "split-handle-0");
    cx.simulate_mouse_down(at, MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_move(point(at.x + px(40.), at.y), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_up(point(at.x + px(40.), at.y), MouseButton::Left, Modifiers::none());
    cx.run_until_parked();
    assert_eq!(view.read_with(cx, |h, _| h.state.width), 240.0);
    let at = center(cx, "split-handle-0");
    cx.simulate_mouse_down(at, MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_move(point(at.x - px(400.), at.y), MouseButton::Left, Modifiers::none());
    cx.simulate_mouse_up(point(at.x - px(400.), at.y), MouseButton::Left, Modifiers::none());
    cx.run_until_parked();
    assert_eq!(view.read_with(cx, |h, _| h.state.width), 120.0, "clamped to the pane's minimum");

    click(cx, "cal-2026-11-20");
    assert_eq!(view.read_with(cx, |h, _| h.state.day), Date::new(2026, 11, 20));
    click(cx, "cal-next");
    assert_eq!(view.read_with(cx, |h, _| h.state.month), (2026, 12));
}

#[gpui::test]
fn sidebar_and_calendar_keys(cx: &mut TestAppContext) {
    let (view, cx) = layout(cx);
    // The sidebar is the first tab stop: Up/Down walk every row across sections.
    focus_first(cx);
    cx.simulate_keystrokes("down down down down enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.place.clone()), "downloads", "the walk crosses into Places");
    cx.simulate_keystrokes("down up up up up space");
    assert_eq!(view.read_with(cx, |h, _| h.state.place.clone()), "all", "and stops at the ends");

    // The calendar: arrows move a day or a week from the selected day, Return picks.
    click(cx, "cal-2026-11-14");
    cx.simulate_keystrokes("right down enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.day), Date::new(2026, 11, 22));
    cx.simulate_keystrokes("down down");
    assert_eq!(view.read_with(cx, |h, _| h.state.month), (2026, 12), "the cursor leaving the month asks for the next");
    cx.simulate_keystrokes("pageup pageup");
    assert_eq!(view.read_with(cx, |h, _| h.state.month), (2026, 10));
}
