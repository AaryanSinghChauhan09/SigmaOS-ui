//! The Rune gallery: every Omarchy UI primitive in the live theme, section for section with the
//! Qt gallery (spec/gallery.md), following theme switches live.
//!
//!   omarchy-rune-gallery                                      a window
//!   omarchy-rune-gallery --snap out.png --scene buttons       one section, headless (Rune's tooling)
//!   omarchy-rune-gallery --dump-tokens                        canonical token JSON
//!   omarchy-rune-gallery --list-sections                      the sections, in order
//!   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-rune-gallery --snap nord.png --scene all
//!
//! Each section is `sections/<name>.rs` with `pub fn build(s, t, layer) -> Vec<El>`; `layer`
//! collects overlays (open dropdowns, menus) mounted above the page. Adding a section: write the
//! module, list it in `gallery!` below (same name as the Qt gallery's `sections/<name>.qml`), and
//! keep any state it needs in its own struct, as a field of `State`.

use omarchy_rune::Theme;
use omarchy_rune::ui::section_header;
use rune::{AppSpec, Cx, El, TextEdit, col};

mod sections;

const WIDTH: f32 = 760.0;

pub(crate) struct State {
    pub theme: Theme,
    pub section: Option<&'static str>,
    pub switches: [bool; 4],
    pub wifi: bool,
    pub bluetooth: bool,
    pub search: TextEdit,
    pub name: TextEdit,
    pub hovered: TextEdit,
    pub gaps: i64,
    pub theme_choice: String,
    pub dropdown_open: bool,
    pub level: f32,
    pub stepped: f32,
    pub align: String,
    pub network: String,
    pub night_light: bool,
    pub night_strength: f32,
    pub lists: sections::lists::Lists,
    pub tiles: sections::tiles::Tiles,
    pub menus: sections::menus::Menus,
    pub navigation: sections::navigation::Navigation,
    pub forms: sections::forms::Forms,
    pub pickers: sections::pickers::Pickers,
    pub data: sections::data::Data,
    pub layout: sections::layout::Layout,
    pub timeline: sections::timeline::Editing,
    pub color: sections::color::Picker,
    pub composed: sections::composed::Composed,
}

pub(crate) fn init() -> State {
    let s = State {
        theme: Theme::load(),
        section: None,
        switches: [false, true, false, true],
        wifi: true,
        bluetooth: false,
        search: TextEdit::new(""),
        name: TextEdit::new("Omarchy"),
        hovered: TextEdit::new("Hovered"),
        gaps: 10,
        theme_choice: "tokyo-night".into(),
        dropdown_open: false,
        level: 0.35,
        stepped: 0.75,
        align: "center".into(),
        network: "basecamp".into(),
        night_light: true,
        night_strength: 0.6,
        lists: sections::lists::Lists::init(),
        tiles: sections::tiles::Tiles::init(),
        menus: sections::menus::Menus::init(),
        navigation: sections::navigation::Navigation::init(),
        forms: sections::forms::init(),
        pickers: sections::pickers::init(),
        data: sections::data::init(),
        layout: sections::layout::init(),
        timeline: sections::timeline::init(),
        color: sections::color::init(),
        composed: sections::composed::init(),
    };
    sections::tiles::preload(&s.theme);
    s
}

/// The sections, in spec/gallery.md order: `name: "Title"`, one per line.
macro_rules! gallery {
    ($($name:ident: $title:literal),* $(,)?) => {
        const SECTIONS: &[&str] = &[$(stringify!($name)),*];
        const SCENES: &[(&str, fn(&mut State))] = &[
            ("all", |s: &mut State| s.section = None),
            $((stringify!($name), |s: &mut State| s.section = Some(stringify!($name)))),*
        ];
        fn sections(s: &State, t: &Theme, layer: &mut Vec<El>) -> Vec<El> {
            let mut out = Vec::new();
            $(
                if s.section.is_none_or(|only| only == stringify!($name)) {
                    out.push(section(t, $title, sections::$name::build(s, t, layer)));
                }
            )*
            out
        }
    };
}

gallery! {
    buttons: "Buttons",
    toggles: "Toggles",
    inputs: "Inputs",
    slider: "Slider",
    surfaces: "Surfaces",
    lists: "Lists",
    tiles: "Tiles",
    progress: "Progress",
    messages: "Messages",
    menus: "Menus",
    navigation: "Navigation",
    forms: "Forms",
    pickers: "Pickers",
    data: "Data",
    layout: "Layout",
    timeline: "Timeline",
    color: "Color",
    composed: "Composed",
    tokens: "Tokens",
}

fn section(t: &Theme, title: &str, body: Vec<El>) -> El {
    let sp = &t.tokens().spacing;
    col().w_full().gap(sp.lg as f32).child(section_header(t, title)).child(col().w_full().gap(sp.xxl as f32).children(body))
}

fn view(s: &State, cx: &mut Cx<State>) -> El {
    let t = &s.theme;
    t.apply(cx);
    let sp = &t.tokens().spacing;
    let mut layer = Vec::new();
    let page = col().w_full().padding(sp.panel_padding as f32).gap(sp.huge as f32 * 2.0).children(sections(s, t, &mut layer));
    let body = col().w_full().h_full().fill(t.bg()).child(page.scroll(rune::ScrollSpec { stick_bottom: false }).grow(1.0).min_h(0.0));
    rune_kit::root(body, layer)
}

fn main() {
    if std::env::args().any(|a| a == "--dump-tokens") {
        println!("{}", omarchy_theme::load().to_canonical_json());
        return;
    }
    if std::env::args().any(|a| a == "--list-sections") {
        println!("{}", SECTIONS.join("\n"));
        return;
    }
    let families = Theme::load().families();
    rune::run(AppSpec {
        scenes: SCENES,
        families,
        setup: Some(|s, wake| s.theme.follow(wake)),
        poll: Some(|s, _, _| s.theme.poll()),
        ..AppSpec::new("Omarchy UI · Rune", (WIDTH, 900.0), init, view)
    });
}

#[cfg(test)]
mod tests {
    use super::*;
    use rune::{Env, Runtime};

    fn fixture(name: &str) -> Theme {
        let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/themes").join(name);
        let read = |f: &str| std::fs::read_to_string(dir.join(f)).unwrap_or_default();
        Theme::from_tokens(omarchy_theme::resolve(&omarchy_theme::Sources {
            colors: omarchy_theme::parse::parse_colors(&read("colors.toml")),
            shell: omarchy_theme::parse::parse_shell(&read("shell.toml")),
            font_family: "monospace".into(),
            resolved_font_family: "monospace".into(),
            ..Default::default()
        }))
    }

    /// Every section lays out, for every fixture theme, without a GPU.
    #[test]
    fn every_section_builds_for_every_theme() {
        let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/themes");
        for entry in std::fs::read_dir(dir).unwrap().flatten() {
            let name = entry.file_name().to_string_lossy().into_owned();
            for section in SECTIONS {
                let mut s = init();
                s.theme = fixture(&name);
                s.section = Some(section);
                let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
                rt.frame(0.0);
            }
        }
    }

    /// Pressing a switch flips it through the Omarchy primitive's handler.
    #[test]
    fn toggles_toggle() {
        let mut s = init();
        s.theme = fixture("tokyo-night");
        s.section = Some("toggles");
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
        let before = rt.state.switches[0];
        let (x, y) = rt.find("switch-0").expect("the first switch");
        rt.pointer_down(x, y);
        rt.pointer_up(x, y);
        rt.frame(1.0 / 60.0);
        assert_ne!(before, rt.state.switches[0]);
    }

    /// The lists section's rows follow its CursorModel: the pointer moves the cursor (only when
    /// it really moves) and the keys move it from there.
    #[test]
    fn lists_cursor_follows_pointer_and_keys() {
        let mut rt = runtime("lists");
        assert!(rt.state.lists.cursor.is_at("rows", 1));
        let (x, y) = rt.find("lists-row-4").expect("row 4");
        rt.pointer_move(x, y);
        rt.frame(1.0 / 60.0);
        rt.pointer_move(x, y + 3.0);
        rt.frame(2.0 / 60.0);
        assert!(rt.state.lists.cursor.is_at("rows", 4));
        rt.pointer_down(x, y + 3.0);
        rt.pointer_up(x, y + 3.0);
        rt.frame(3.0 / 60.0);
        rt.key(rune::KeyEvent { key: rune::Key::Down, mods: Default::default() });
        rt.frame(4.0 / 60.0);
        rt.key(rune::KeyEvent { key: rune::Key::Text("j".into()), mods: Default::default() });
        rt.frame(5.0 / 60.0);
        assert!(rt.state.lists.cursor.is_at("rows", 6), "the still pointer doesn't pull the cursor back");
    }

    /// The composed SearchList: typing filters, arrows move the cursor, Return activates the
    /// row under it, Escape clears the query.
    #[test]
    fn search_list_filters_moves_and_activates() {
        use rune::{Key, KeyEvent};
        let key = |rt: &mut Runtime<State>, k: Key, at: f64| {
            rt.key(KeyEvent { key: k, mods: Default::default() });
            rt.frame(at);
        };
        let mut rt = runtime("composed");
        press(&mut rt, "composed-search-filter");
        key(&mut rt, Key::Down, 2.0 / 60.0);
        assert_eq!(rt.state.composed.cursor.index(), 2);
        key(&mut rt, Key::Enter, 3.0 / 60.0);
        assert_eq!(rt.state.composed.picked.as_deref(), Some("Terminal theme"));
        key(&mut rt, Key::Text("i".into()), 4.0 / 60.0);
        assert_eq!(rt.state.composed.query.text, "termi");
        assert_eq!(rt.state.composed.cursor.index(), 0, "a new query starts at the first match");
        key(&mut rt, Key::Text("x".into()), 5.0 / 60.0);
        assert!(rt.find("composed-search-row-0").is_none(), "nothing matches termix");
        key(&mut rt, Key::Escape, 6.0 / 60.0);
        assert_eq!(rt.state.composed.query.text, "");
        assert!(rt.find("composed-search-row-3").is_some(), "an empty query lists everything");
    }

    /// The rest of SearchList's keys, as `qt/tests/tst_searchlist.qml` drives Qt's: j, k, x and
    /// Space are text; the paging keys move the cursor and stop at the ends; Delete asks to
    /// delete the cursor row; Escape clears, then asks to close; Return with no rows does nothing.
    #[test]
    fn search_list_keys_match_qt() {
        use rune::{Key, KeyEvent};
        let mut at = 1.0 / 60.0;
        let mut key = |rt: &mut Runtime<State>, k: Key| {
            at += 1.0 / 60.0;
            rt.key(KeyEvent { key: k, mods: Default::default() });
            rt.frame(at);
        };
        let text = |t: &str| Key::Text(t.into());
        let mut rt = runtime("composed");
        press(&mut rt, "composed-search-filter");
        let c = |rt: &Runtime<State>| rt.state.composed.cursor.index();

        for (k, want) in [(Key::Down, 2), (Key::Down, 3), (Key::Down, 3), (Key::Up, 2), (Key::Home, 0), (Key::End, 3), (Key::PageUp, 0), (Key::PageDown, 3)] {
            let name = format!("{k:?}");
            key(&mut rt, k);
            assert_eq!(c(&rt), want, "after {name}");
        }
        key(&mut rt, Key::Up);
        key(&mut rt, Key::Delete);
        assert_eq!(rt.state.composed.deleted.as_deref(), Some("Terminal theme"));

        for t in ["j", "k", " ", "x"] {
            key(&mut rt, text(t));
        }
        assert_eq!(rt.state.composed.query.text, "termjk x", "j, k, Space and x are text");
        assert_eq!(rt.state.composed.deleted.as_deref(), Some("Terminal theme"), "x didn't delete");
        assert!(rt.find("composed-search-row-0").is_none());
        key(&mut rt, Key::Enter);
        assert_eq!(rt.state.composed.picked, None, "Return with no rows does nothing");

        key(&mut rt, Key::Escape);
        assert_eq!(rt.state.composed.query.text, "");
        assert_eq!(rt.state.composed.closes, 0);
        key(&mut rt, Key::Escape);
        assert_eq!(rt.state.composed.closes, 1, "Escape on an empty query asks to close");
    }

    /// The empty SearchList reads the catalog's no-matches line (the one Qt's I18n reads, from
    /// the harvested Strings.js) in the display language, with the query in it.
    #[test]
    fn search_list_says_no_matches_in_the_display_language() {
        for (language, line) in [("en", "No matches for “zzz”"), ("de", "")] {
            let mut s = init();
            let mut tokens = fixture("tokyo-night").tokens().clone();
            tokens.language = language.into();
            s.theme = Theme::from_tokens(tokens);
            s.section = Some("composed");
            let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
            rt.frame(0.0);
            let want = omarchy_theme::strings::lookup(language, "common.noMatchesFor").unwrap().replace("%1", "zzz");
            if !line.is_empty() {
                assert_eq!(want, line);
            }
            assert!(want.contains("zzz"));
            assert!(rt.elements().iter().any(|e| e.text.as_deref() == Some(want.as_str())), "{language}: no {want:?} on screen");
        }
    }

    /// A gallery section, laid out once, in tokyo-night.
    fn runtime(section: &'static str) -> Runtime<State> {
        let mut s = init();
        s.theme = fixture("tokyo-night");
        s.section = Some(section);
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
        rt
    }

    /// Click the element keyed `key`.
    fn press(rt: &mut Runtime<State>, key: &str) {
        let (x, y) = rt.find(key).unwrap_or_else(|| panic!("no element keyed {key}"));
        rt.pointer_down(x, y);
        rt.pointer_up(x, y);
        rt.frame(1.0 / 60.0);
    }

    /// A fresh `section` with the element keyed `key` clicked.
    fn pressed(section: &'static str, key: &str) -> Runtime<State> {
        let mut rt = runtime(section);
        press(&mut rt, key);
        rt
    }

    /// Pressing a menu row runs it: a check row flips, a plain row reports itself.
    #[test]
    fn menu_rows_run() {
        assert!(!pressed("menus", "menu-sidebar").state.menus.sidebar);
        assert_eq!(pressed("menus", "menu-new").state.menus.ran, Some("New"));
    }

    /// A menu with no `on_close` still walks with j and k as with Down and Up (Qt's Menu takes
    /// both), skipping its disabled row and wrapping at the ends, and Enter runs the row the walk
    /// lands on (`qt/tests/tst_navigation.qml` drives Qt's the same way).
    #[test]
    fn menu_walks_with_vim_keys_and_arrows() {
        use rune::{Key, KeyEvent};
        let mut rt = pressed("menus", "menu-new");
        assert_eq!(rt.state.menus.ran, Some("New"));
        let mut at = 1.0 / 60.0;
        let mut key = |rt: &mut Runtime<State>, k: &str| {
            let key = match k {
                "Down" => Key::Down,
                "Up" => Key::Up,
                "Enter" => Key::Enter,
                "Home" => Key::Home,
                "End" => Key::End,
                c => Key::Text(c.into()),
            };
            rt.key(KeyEvent { key, mods: Default::default() });
            // j and k are re-sent as Down and Up, which land on the next frame.
            for _ in 0..2 {
                at += 1.0 / 60.0;
                rt.frame(at);
            }
        };
        for (keys, ran) in [
            (&["j", "Enter"][..], "Open…"),
            (&["Down", "Enter"], "Save"),
            (&["k", "Enter"], "Open…"),
            (&["j", "j", "Enter"], "Sidebar"),
            (&["End", "Down", "Enter"], "New"),
            (&["Home", "Up", "Up", "Enter"], "Word wrap"),
        ] {
            for k in keys {
                key(&mut rt, k);
            }
            if ran == "Sidebar" {
                assert!(!rt.state.menus.sidebar, "j skips the disabled row to Sidebar and Enter flips it");
            } else if ran == "Word wrap" {
                assert!(rt.state.menus.wrap, "Up from the first row wraps to the last");
            } else {
                assert_eq!(rt.state.menus.ran, Some(ran), "after {keys:?}");
            }
        }
    }

    /// A Pane's header actions are live controls at the end of its Heading: the Night light
    /// switch sits at the bordered pane's right edge and flips its setting.
    #[test]
    fn pane_actions_run() {
        let mut rt = runtime("surfaces");
        let (x, _) = rt.find("night-light").expect("the pane's header switch");
        let sp = &rt.state.theme.tokens().spacing;
        // The page padding, the 240-wide Layers pane and the gap, then the 240-wide bordered pane.
        let right = sp.panel_padding as f32 + 240.0 + sp.huge as f32 + 240.0 - sp.panel_padding as f32;
        assert!(x > right - 40.0 && x < right, "the action ends the header, not the title ({x} vs {right})");
        assert!(rt.state.night_light);
        press(&mut rt, "night-light");
        assert!(!rt.state.night_light);
    }

    /// Pressing a tab or a rail item chooses it.
    #[test]
    fn tabs_and_rail_choose() {
        assert_eq!(pressed("navigation", "tab-3").state.navigation.tab, 3);
        assert_eq!(pressed("navigation", "rail-4").state.navigation.panel, 4);
    }

    /// Keys on the choosers, as `qt/tests/tst_navigation.qml` and `tst_forms.qml` drive Qt's:
    /// Tabs take Left/Right and h/l, the IconRail Up/Down and k/j, a RadioGroup the arrows and
    /// j/k; each selects as it moves, stops at the ends, and Home/End jump (tabs and rail).
    #[test]
    fn choosers_follow_keys_as_in_qt() {
        use rune::{Key, KeyEvent};
        fn walk(rt: &mut Runtime<State>, keys: &[&str], at: &mut f64) {
            for k in keys {
                let key = match *k {
                    "Left" => Key::Left,
                    "Right" => Key::Right,
                    "Up" => Key::Up,
                    "Down" => Key::Down,
                    "Home" => Key::Home,
                    "End" => Key::End,
                    c => Key::Text(c.into()),
                };
                rt.key(KeyEvent { key, mods: Default::default() });
                // A key a handler re-sends (an alias) lands on the next frame.
                for _ in 0..2 {
                    *at += 1.0 / 60.0;
                    rt.frame(*at);
                }
            }
        }
        let mut at = 1.0 / 60.0;
        let mut rt = pressed("navigation", "tab-1");
        assert_eq!(rt.state.navigation.tab, 1);
        for (keys, want) in [(&["Right"][..], 2), (&["l"], 3), (&["Right"], 3), (&["h", "Left"], 1), (&["Home"], 0), (&["Left"], 0), (&["End"], 3), (&["Up", "Down"], 3)] {
            walk(&mut rt, keys, &mut at);
            assert_eq!(rt.state.navigation.tab, want, "tabs after {keys:?}");
        }

        let mut rt = pressed("navigation", "rail-0");
        let last = 4;
        for (keys, want) in [(&["Down"][..], 1), (&["j"], 2), (&["k", "Up"], 0), (&["Up"], 0), (&["End"], last), (&["Down"], last), (&["Home"], 0), (&["Right", "Left"], 0)] {
            walk(&mut rt, keys, &mut at);
            assert_eq!(rt.state.navigation.panel, want, "rail after {keys:?}");
        }

        let mut rt = pressed("forms", "radio-top");
        assert_eq!(rt.state.forms.position, "top");
        for (keys, want) in [(&["Down"][..], "bottom"), (&["j"], "hidden"), (&["Down"], "hidden"), (&["k"], "bottom"), (&["Up", "Up"], "top"), (&["Right"], "bottom"), (&["Left"], "top")] {
            walk(&mut rt, keys, &mut at);
            assert_eq!(rt.state.forms.position, want, "radios after {keys:?}");
        }
    }

    /// A checkbox flips, a disabled one doesn't, and a radio takes the group's value.
    #[test]
    fn forms_check_and_choose() {
        let mut rt = runtime("forms");
        press(&mut rt, "check-0");
        assert!(rt.state.forms.checks[0]);
        press(&mut rt, "check-3");
        assert!(!rt.state.forms.checks[3]);
        press(&mut rt, "radio-hidden");
        assert_eq!(rt.state.forms.position, "hidden");
    }

    /// A MultiSelect row adds or removes its value.
    #[test]
    fn multiselect_toggles_values() {
        let mut rt = runtime("pickers");
        press(&mut rt, "ms-notify-Calendar");
        assert!(rt.state.pickers.notify.iter().any(|v| v == "Calendar"));
        press(&mut rt, "ms-notify-Mail");
        assert!(!rt.state.pickers.notify.iter().any(|v| v == "Mail"));
    }

    /// The MultiSelect's keys, as `qt/tests/tst_forms.qml`'s test_multiselect_keys drives Qt's:
    /// j opens it from the trigger, typing filters and Return toggles the first match, Down goes
    /// into the list, Space toggles, j walks, Return toggles, k off the first row goes back to the
    /// search, Escape closes.
    #[test]
    fn multiselect_keys_as_in_qt() {
        use rune::{Key, KeyEvent};
        let mut at = 1.0 / 60.0;
        let mut walk = |rt: &mut Runtime<State>, keys: &[Key]| {
            for k in keys {
                rt.key(KeyEvent { key: k.clone(), mods: Default::default() });
                for _ in 0..2 {
                    at += 1.0 / 60.0;
                    rt.frame(at);
                }
            }
        };
        let text = |t: &str| Key::Text(t.into());
        // The gallery holds it open; a press on the trigger closes it and leaves the trigger focused.
        let mut rt = pressed("pickers", "ms-notify");
        assert!(!rt.state.pickers.notify_open);
        walk(&mut rt, &[text("j")]);
        assert!(rt.state.pickers.notify_open, "j opens it from the trigger");
        walk(&mut rt, &[text("m"), text("e"), text("s")]);
        assert_eq!(rt.state.pickers.search.text, "mes");
        walk(&mut rt, &[Key::Enter]);
        assert_eq!(rt.state.pickers.notify, ["Mail", "Updates", "Messages"], "Return toggles the first match");
        walk(&mut rt, &[Key::Backspace, Key::Backspace, Key::Backspace, Key::Down, text(" ")]);
        assert_eq!(rt.state.pickers.notify, ["Mail", "Updates", "Messages", "Calendar"], "Down into the list, Space toggles");
        walk(&mut rt, &[text("j"), Key::Enter]);
        assert_eq!(rt.state.pickers.notify, ["Updates", "Messages", "Calendar"], "j walks, Return toggles");
        walk(&mut rt, &[Key::End, text("j"), text("j"), text("j"), text("j")]);
        assert_eq!(rt.state.pickers.notify_row, Some(4), "stops at the last row");
        walk(&mut rt, &[text("k"), text("k"), text("k"), text("k"), Key::Up]);
        assert_eq!(rt.state.pickers.notify_row, None, "Up off the first row goes back to the search");
        walk(&mut rt, &[Key::Escape]);
        assert!(!rt.state.pickers.notify_open, "Escape closes it");
        walk(&mut rt, &[Key::Down]);
        assert!(rt.state.pickers.notify_open, "Down opens it too");
        assert_eq!(rt.state.pickers.search.text, "", "with the search cleared");
    }

    /// Pressing a table row selects it, a tree chevron folds its branch, a tree row selects it.
    #[test]
    fn data_selects_and_folds() {
        let mut rt = runtime("data");
        press(&mut rt, "files-4");
        assert_eq!(rt.state.data.row, 4);
        press(&mut rt, "files-col-0");
        assert!(!rt.state.data.sort_ascending);
        press(&mut rt, "tree-qt-toggle");
        assert!(!rt.state.data.expanded.iter().any(|e| e == "qt"));
        press(&mut rt, "tree-spec/tokens");
        assert_eq!(rt.state.data.node, "spec/tokens");
    }

    /// Keys on the table and the tree, as `qt/tests/tst_data.qml` drives Qt's: Up/Down/Home/End
    /// walk the rows and stop at the ends, Return and Space choose; in the tree Left folds an
    /// open branch and Right unfolds a folded one.
    #[test]
    fn data_keys_as_in_qt() {
        use rune::{Key, KeyEvent};
        fn walk(rt: &mut Runtime<State>, keys: &[Key], at: &mut f64) {
            for k in keys {
                rt.key(KeyEvent { key: k.clone(), mods: Default::default() });
                for _ in 0..2 {
                    *at += 1.0 / 60.0;
                    rt.frame(*at);
                }
            }
        }
        let mut at = 1.0 / 60.0;
        let mut rt = pressed("data", "files-0");
        assert_eq!(rt.state.data.row, 0);
        walk(&mut rt, &[Key::Down, Key::Enter], &mut at);
        assert_eq!(rt.state.data.row, 1);
        walk(&mut rt, &[Key::End, Key::Down, Key::Text(" ".into())], &mut at);
        let last = rt.state.data.row;
        assert!(last > 1, "End then Space chooses the last row");
        walk(&mut rt, &[Key::Home, Key::Up, Key::Enter], &mut at);
        assert_eq!(rt.state.data.row, 0, "Up stops at the first row");

        let mut rt = pressed("data", "tree-qt");
        assert_eq!(rt.state.data.node, "qt");
        let open = |rt: &Runtime<State>, id: &str| rt.state.data.expanded.iter().any(|e| e == id);
        assert!(open(&rt, "qt"));
        walk(&mut rt, &[Key::Left], &mut at);
        assert!(!open(&rt, "qt"), "Left folds an open branch");
        walk(&mut rt, &[Key::Left], &mut at);
        assert!(!open(&rt, "qt"), "and leaves a folded one");
        walk(&mut rt, &[Key::Right], &mut at);
        assert!(open(&rt, "qt"), "Right unfolds it");
        walk(&mut rt, &[Key::Right], &mut at);
        assert!(open(&rt, "qt"), "and leaves an open one");
        walk(&mut rt, &[Key::Down, Key::Enter], &mut at);
        assert_ne!(rt.state.data.node, "qt", "Down then Return chooses the next row");
    }

    /// The sidebar picks, the split handle drags, the calendar picks a day and steps months.
    #[test]
    fn layout_picks_drags_and_steps() {
        let mut rt = runtime("layout");
        press(&mut rt, "places-downloads");
        assert_eq!(rt.state.layout.place, "downloads");

        let (x, y) = rt.find("split-handle-0").expect("the split handle");
        rt.pointer_down(x, y);
        rt.pointer_move(x + 40.0, y);
        rt.frame(1.0 / 60.0);
        rt.pointer_up(x + 40.0, y);
        rt.frame(2.0 / 60.0);
        assert_eq!(rt.state.layout.sidebar, 240.0);

        press(&mut rt, "cal-2026-11-20");
        assert_eq!(rt.state.layout.day, omarchy_rune::ui::Date::new(2026, 11, 20));
        press(&mut rt, "cal-next");
        assert_eq!(rt.state.layout.month, (2026, 12));
    }

    /// The progress section's Spinners, stepped on the render clock: a turning one turns its
    /// glyph's outline once every 900 ms about its square's middle, a pulse breathes once every
    /// 840 ms (420 ms each way), and under reduced motion both stand still as text.
    #[test]
    fn spinners_turn_and_pulse_on_the_clock() {
        use omarchy_rune::ui::Spinner;
        let mut rt = runtime("progress");
        rt.frame(0.225);
        let els = rt.elements();
        let spinners: Vec<_> = els.iter().filter(|e| e.label.as_deref() == Some("Busy")).collect();
        assert_eq!(spinners.len(), 4, "the section's four spinners");
        let (turning, pulsing) = spinners.split_at(2);
        for s in turning {
            let glyph = els.iter().find(|e| e.role == "icon" && e.parent.is_some_and(|p| std::ptr::eq(&els[p], *s)))
                .expect("a turning spinner draws its glyph as an icon");
            assert_eq!(glyph.rect, s.rect, "the glyph turns about the spinner square's middle");
            let phase = glyph.loop_phase.expect("the glyph runs a loop");
            assert!((phase - 0.225 / Spinner::TURN).abs() < 1e-6, "a quarter turn at 225 ms, not {phase}");
            assert_eq!(s.rect[2], s.rect[3], "a square");
        }
        for s in pulsing {
            let phase = s.loop_phase.expect("a pulse runs a loop");
            assert!((phase - 0.225 / Spinner::PULSE).abs() < 1e-6, "{phase}");
        }

        let mut s = init();
        s.theme = Theme::from_tokens(omarchy_theme::Tokens { reduce_motion: true, ..fixture("tokyo-night").tokens().clone() });
        s.section = Some("progress");
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
        rt.frame(0.3);
        let els = rt.elements();
        assert!(!els.iter().any(|e| e.role == "icon"), "still spinners are text");
        assert!(els.iter().all(|e| e.loop_phase.is_none_or(|p| p == 0.0)), "nothing loops");
    }

    /// Pressing a clip selects it; pressing a swatch picks its color.
    #[test]
    fn editing_selects_and_picks() {
        let mut rt = runtime("timeline");
        press(&mut rt, "clip-1-0");
        assert_eq!(rt.state.timeline.selected, Some((1, 0)));

        let mut rt = runtime("color");
        press(&mut rt, "swatch-red");
        assert_eq!(rt.state.color.value.hex(), "#f7768e");
        assert_eq!(rt.state.color.hex.text, "#f7768e");
    }
}
