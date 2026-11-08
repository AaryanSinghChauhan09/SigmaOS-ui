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
    pub lists: sections::lists::Lists,
    pub tiles: sections::tiles::Tiles,
    pub menus: sections::menus::Menus,
    pub navigation: sections::navigation::Navigation,
}

pub(crate) fn init() -> State {
    State {
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
        lists: sections::lists::Lists::init(),
        tiles: sections::tiles::Tiles::init(),
        menus: sections::menus::Menus::init(),
        navigation: sections::navigation::Navigation::init(),
    }
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
        let mut s = init();
        s.theme = fixture("tokyo-night");
        s.section = Some("lists");
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
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
    fn press(section: &'static str, key: &str) -> Runtime<State> {
        let mut s = init();
        s.theme = fixture("tokyo-night");
        s.section = Some(section);
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
        let (x, y) = rt.find(key).unwrap_or_else(|| panic!("no element keyed {key}"));
        rt.pointer_down(x, y);
        rt.pointer_up(x, y);
        rt.frame(1.0 / 60.0);
        rt
    }

    /// Pressing a menu row runs it: a check row flips, a plain row reports itself.
    #[test]
    fn menu_rows_run() {
        assert!(!press("menus", "menu-sidebar").state.menus.sidebar);
        assert_eq!(press("menus", "menu-new").state.menus.ran, Some("New"));
    }

    /// Pressing a tab or a rail item chooses it.
    #[test]
    fn tabs_and_rail_choose() {
        assert_eq!(press("navigation", "tab-3").state.navigation.tab, 3);
        assert_eq!(press("navigation", "rail-4").state.navigation.panel, 4);
    }
}
