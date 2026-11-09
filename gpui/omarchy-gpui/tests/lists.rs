//! Lists driven by keys and the pointer, as the Rune gallery tests and `qt/tests` drive theirs
//! (tst_cursormodel.qml, tst_searchlist.qml): the CursorModel's pointer gate and keys over a
//! ListRow list, a row's revealed action, a TileGrid's tiles, and SearchList's keys and strings.

mod common;

use common::{focus_first, host, take_log};
use gpui::{
    AnyElement, Context, Entity, FocusHandle, IntoElement, InteractiveElement, KeyDownEvent, Modifiers, ParentElement, Pixels, Point,
    ScrollHandle, SharedString, Styled, TestAppContext, VisualTestContext, Window, div, point, px,
};
use omarchy_gpui::ui::{Avatar, CursorKey, CursorModel, ListRow, ScrollArea, SearchItem, SearchList, SearchState, Tile, TileGrid};

struct Rows {
    cursor: CursorModel,
    scroll: ScrollHandle,
    focus: Option<FocusHandle>,
}

fn rows(h: &mut common::Host<Rows>, _: &mut Window, cx: &mut Context<common::Host<Rows>>) -> AnyElement {
    let focus = h.state.focus.get_or_insert_with(|| cx.focus_handle().tab_stop(true)).clone();
    let mut list = ScrollArea::new("rows").track_scroll(&h.state.scroll).width(300.0).max_height(150.0);
    for i in 0..9 {
        list = list.child(
            ListRow::new(format!("Row {i}"))
                .id(SharedString::from(format!("row-{i}")))
                .glyph("\u{F0928}")
                .subtitle("Open")
                .cursor(h.state.cursor.is_at("rows", i))
                .on_pointer_move(cx.listener(move |h: &mut common::Host<Rows>, p: &Point<Pixels>, _, cx| {
                    if h.state.cursor.pointer_moved(p.x.into(), p.y.into(), "rows", i) {
                        cx.notify();
                    }
                }))
                .on_click(cx.listener(move |h: &mut common::Host<Rows>, _, _, _| h.log.push(format!("row {i}")))),
        );
    }
    div()
        .id("keys")
        .track_focus(&focus)
        .on_key_down(cx.listener(|h: &mut common::Host<Rows>, e: &KeyDownEvent, _, cx| {
            let s = &mut h.state;
            match s.cursor.apply_key(&e.keystroke) {
                CursorKey::Moved => ScrollArea::ensure_visible(&s.scroll, s.cursor.index()),
                CursorKey::Activate => h.log.push(format!("activate {}", s.cursor.index())),
                CursorKey::Ignored => return,
                _ => {}
            }
            cx.stop_propagation();
            cx.notify();
        }))
        .child(list)
        .into_any_element()
}

fn center(cx: &mut VisualTestContext, selector: &'static str) -> Point<Pixels> {
    cx.debug_bounds(selector).unwrap_or_else(|| panic!("no {selector} on screen")).center()
}

fn rows_state() -> Rows {
    let mut cursor = CursorModel::new([("rows", 9)]);
    cursor.set("rows", 1);
    Rows { cursor, scroll: ScrollHandle::new(), focus: None }
}

/// The rows follow their CursorModel: the pointer moves the cursor only when it really moves,
/// and the keys move it from there; a still pointer doesn't pull it back (the lists section's
/// Rune test, lists_cursor_follows_pointer_and_keys).
#[gpui::test]
fn cursor_follows_pointer_and_keys(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, rows_state(), rows);
    let at = |cx: &mut VisualTestContext, i| view.read_with(cx, |h, _| h.state.cursor.is_at("rows", i));
    assert!(at(cx, 1));
    let p = center(cx, "row-2");
    cx.simulate_mouse_move(p, None, Modifiers::none());
    assert!(at(cx, 1), "the first sample only primes the gate");
    cx.simulate_mouse_move(p + point(px(0.), px(3.)), None, Modifiers::none());
    assert!(at(cx, 2));
    cx.simulate_click(p + point(px(0.), px(3.)), Modifiers::none());
    assert_eq!(take_log(&view, cx), ["row 2"]);
    focus_first(cx);
    cx.simulate_keystrokes("down j");
    assert!(at(cx, 4), "the still pointer doesn't pull the cursor back");
    cx.simulate_mouse_move(p + point(px(0.), px(3.5)), None, Modifiers::none());
    assert!(at(cx, 4), "sub-point jitter is not a move");
    cx.simulate_keystrokes("enter");
    assert_eq!(take_log(&view, cx), ["activate 4"]);
}

/// Keys past the cap scroll the cursor's row into view.
#[gpui::test]
fn keys_reveal_the_cursor_row(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, rows_state(), rows);
    focus_first(cx);
    cx.simulate_keystrokes("end");
    assert!(view.read_with(cx, |h, _| h.state.cursor.is_at("rows", 8)));
    let offset = view.read_with(cx, |h, _| h.state.scroll.offset().y);
    assert!(offset < px(0.), "scrolled down to the last row");
    let list = cx.debug_bounds("row-8").unwrap();
    let viewport = view.read_with(cx, |h, _| h.state.scroll.bounds());
    assert!(list.bottom() <= viewport.bottom() + px(0.5), "the last row is in view");
    cx.simulate_keystrokes("home");
    assert_eq!(view.read_with(cx, |h, _| h.state.scroll.offset().y), px(0.));
}

/// A row's action runs its own handler, not the row's, and a disabled row runs nothing.
#[gpui::test]
fn action_and_disabled_rows(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, (), |_, _, cx| {
        div()
            .w(px(320.))
            .child(
                ListRow::new("Office")
                    .id("office")
                    .action("\u{F0159}", true)
                    .cursor(true)
                    .on_click(cx.listener(|h: &mut common::Host<()>, _, _, _| h.log.push("row".into())))
                    .on_action(cx.listener(|h: &mut common::Host<()>, _, _, _| h.log.push("forget".into()))),
            )
            .child(
                ListRow::new("Neighbor")
                    .id("neighbor")
                    .disabled(true)
                    .on_click(cx.listener(|h: &mut common::Host<()>, _, _, _| h.log.push("disabled".into()))),
            )
            .into_any_element()
    });
    let action = center(cx, "office-action");
    cx.simulate_click(action, Modifiers::none());
    assert_eq!(take_log(&view, cx), ["forget"]);
    let row = cx.debug_bounds("office").unwrap();
    cx.simulate_click(row.origin + point(px(20.), px(10.)), Modifiers::none());
    assert_eq!(take_log(&view, cx), ["row"]);
    let off = center(cx, "neighbor");
    cx.simulate_click(off, Modifiers::none());
    assert!(take_log(&view, cx).is_empty());
}

/// A TileGrid's tiles report the pointer and clicks with their index, and the grid's cursor walks
/// by columns (tst_cursormodel's grid case).
#[gpui::test]
fn tile_grid_reports_tiles(cx: &mut TestAppContext) {
    let mut cursor = CursorModel::list(6).columns(4);
    cursor.set("", 1);
    let (view, cx) = host(cx, cursor, |h, _, cx| {
        TileGrid::new((0..6).map(|i| if i == 5 { Tile::new("Photos").avatar(Avatar::new("Photos")) } else { Tile::new(format!("Tile {i}")).glyph("\u{F120}") }))
            .id("grid")
            .columns(4)
            .cursor(h.state.cursor_in(""))
            .on_pointer_move(cx.listener(|h: &mut common::Host<CursorModel>, (i, p): &(usize, Point<Pixels>), _, cx| {
                if h.state.pointer_moved(p.x.into(), p.y.into(), "", *i) {
                    cx.notify();
                }
            }))
            .on_activate(cx.listener(|h: &mut common::Host<CursorModel>, i: &usize, _, _| h.log.push(format!("tile {i}"))))
            .into_any_element()
    });
    let p = center(cx, "grid-5");
    cx.simulate_mouse_move(p, None, Modifiers::none());
    cx.simulate_mouse_move(p + point(px(4.), px(0.)), None, Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.index()), 5);
    let two = center(cx, "grid-2");
    cx.simulate_click(two, Modifiers::none());
    assert_eq!(take_log(&view, cx), ["tile 2"]);
    let grid = cx.debug_bounds("grid-0").unwrap();
    let five = cx.debug_bounds("grid-5").unwrap();
    assert_eq!(five.origin.x, grid.origin.x + grid.size.width, "tile 5 is on the second line, second column");
    assert_eq!(five.origin.y, grid.origin.y + grid.size.height);
}

/// The composed section's SearchList: four items, filtered by the caller, with handlers that
/// record what they were asked.
#[derive(Default)]
struct Search {
    state: Option<Entity<SearchState>>,
    picked: Option<SharedString>,
    deleted: Option<SharedString>,
    closes: usize,
}

fn items() -> Vec<SearchItem> {
    vec![
        SearchItem::new("Terminal").glyph("\u{F120}").subtitle("App").detail("Open a new terminal window."),
        SearchItem::new("Terminal font").glyph("\u{F031}").subtitle("Style").detail("Pick the monospace font every terminal uses."),
        SearchItem::new("Terminal theme").glyph("\u{F1FC}").subtitle("Style").detail("Match the terminal colors to the desktop theme."),
        SearchItem::new("Intermediate CA").glyph("\u{F0A3}").subtitle("Certificate").detail("Trust a certificate authority for this machine."),
    ]
}

fn search(h: &mut common::Host<Search>, _: &mut Window, cx: &mut Context<common::Host<Search>>) -> AnyElement {
    let state = h.state.state.get_or_insert_with(|| SearchState::new(cx, "term")).clone();
    let query = state.read(cx).query(cx);
    let shown: Vec<SearchItem> = items().into_iter().filter(|i| SearchList::matches(&i.title, &query)).collect();
    let titles: Vec<SharedString> = shown.iter().map(|i| i.title.clone()).collect();
    let titles2 = titles.clone();
    SearchList::new("search", &state)
        .items(shown)
        .detail(true)
        .size(560.0, 300.0)
        .on_activate(cx.listener(move |h: &mut common::Host<Search>, i: &usize, _, _| h.state.picked = Some(titles[*i].clone())))
        .on_delete(cx.listener(move |h: &mut common::Host<Search>, i: &usize, _, _| h.state.deleted = Some(titles2[*i].clone())))
        .on_close(cx.listener(|h: &mut common::Host<Search>, _, _, _| h.state.closes += 1))
        .into_any_element()
}

fn query(view: &Entity<common::Host<Search>>, cx: &mut VisualTestContext) -> SharedString {
    let state = view.read_with(cx, |h, _| h.state.state.clone().unwrap());
    state.read_with(cx, |s, cx| s.query(cx))
}

fn cursor(view: &Entity<common::Host<Search>>, cx: &mut VisualTestContext) -> usize {
    let state = view.read_with(cx, |h, _| h.state.state.clone().unwrap());
    state.read_with(cx, |s, _| s.cursor.index())
}

/// Typing filters, arrows move the cursor, Return activates the row under it, Escape clears the
/// query (search_list_filters_moves_and_activates).
#[gpui::test]
fn search_list_filters_moves_and_activates(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, Search::default(), search);
    focus_first(cx);
    cx.simulate_keystrokes("down down");
    assert_eq!(cursor(&view, cx), 2);
    cx.simulate_keystrokes("enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.picked.clone()).as_deref(), Some("Terminal theme"));
    cx.simulate_input("i");
    assert_eq!(query(&view, cx), "termi");
    assert_eq!(cursor(&view, cx), 0, "a new query starts at the first match");
    cx.simulate_input("x");
    assert!(cx.debug_bounds("search-row-0").is_none(), "nothing matches termix");
    cx.simulate_keystrokes("escape");
    assert_eq!(query(&view, cx), "");
    assert!(cx.debug_bounds("search-row-3").is_some(), "an empty query lists everything");
}

/// The rest of SearchList's keys, as tst_searchlist.qml drives Qt's: j, k, x and Space are text;
/// the paging keys move the cursor and stop at the ends; Delete asks to delete the cursor row;
/// Escape clears, then asks to close; Return with no rows does nothing.
#[gpui::test]
fn search_list_keys_match_qt(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, Search::default(), search);
    focus_first(cx);
    for (k, want) in [("down", 1), ("down", 2), ("down", 3), ("down", 3), ("up", 2), ("home", 0), ("end", 3), ("pageup", 0), ("pagedown", 3)] {
        cx.simulate_keystrokes(k);
        assert_eq!(cursor(&view, cx), want, "after {k}");
    }
    cx.simulate_keystrokes("up delete");
    assert_eq!(view.read_with(cx, |h, _| h.state.deleted.clone()).as_deref(), Some("Terminal theme"));
    cx.simulate_input("jk x");
    assert_eq!(query(&view, cx), "termjk x", "j, k, Space and x are text");
    assert_eq!(view.read_with(cx, |h, _| h.state.deleted.clone()).as_deref(), Some("Terminal theme"), "x didn't delete");
    assert!(cx.debug_bounds("search-row-0").is_none());
    cx.simulate_keystrokes("enter");
    assert_eq!(view.read_with(cx, |h, _| h.state.picked.clone()), None, "Return with no rows does nothing");
    cx.simulate_keystrokes("escape");
    assert_eq!(query(&view, cx), "");
    assert_eq!(view.read_with(cx, |h, _| h.state.closes), 0);
    cx.simulate_keystrokes("escape");
    assert_eq!(view.read_with(cx, |h, _| h.state.closes), 1, "Escape on an empty query asks to close");
}

/// A click on a row activates it and puts the cursor there.
#[gpui::test]
fn search_list_click_activates(cx: &mut TestAppContext) {
    let (view, cx) = host(cx, Search::default(), search);
    let one = center(cx, "search-row-1");
    cx.simulate_click(one, Modifiers::none());
    assert_eq!(view.read_with(cx, |h, _| h.state.picked.clone()).as_deref(), Some("Terminal font"));
    assert_eq!(cursor(&view, cx), 1);
}

/// The empty SearchList reads the catalog's no-matches line in the display language, with the
/// query in it (search_list_says_no_matches_in_the_display_language).
#[gpui::test]
fn search_list_says_no_matches_in_the_display_language(cx: &mut TestAppContext) {
    for language in ["en", "de"] {
        let (view, vcx) = host(cx, Search::default(), search);
        let mut tokens = common::fixture("tokyo-night");
        tokens.language = language.into();
        vcx.update(|_, cx| omarchy_gpui::set_tokens(cx, tokens));
        let state = view.read_with(vcx, |h, _| h.state.state.clone().unwrap());
        state.update_in(vcx, |s, _, cx| s.input.update(cx, |i, cx| i.set_text("zzz", cx)));
        vcx.run_until_parked();
        let want = omarchy_gpui::omarchy_theme::strings::lookup(language, "common.noMatchesFor").unwrap().replace("%1", "zzz");
        if language == "en" {
            assert_eq!(want, "No matches for “zzz”");
        }
        let selector: &'static str = Box::leak(format!("search-empty: {want}").into_boxed_str());
        assert!(vcx.debug_bounds(selector).is_some(), "{language}: no {want:?} on screen");
    }
}
