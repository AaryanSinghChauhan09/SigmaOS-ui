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
    pub data: sections::data::Data,
    pub layout: sections::layout::Layout,
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
        data: sections::data::init(),
        layout: sections::layout::init(),
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
    data: "Data",
    layout: "Layout",
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

    fn press(rt: &mut Runtime<State>, key: &str) {
        let (x, y) = rt.find(key).unwrap_or_else(|| panic!("no element keyed {key}"));
        rt.pointer_down(x, y);
        rt.pointer_up(x, y);
        rt.frame(1.0 / 60.0);
    }

    /// Pressing a table row selects it, a tree chevron folds its branch, a tree row selects it.
    #[test]
    fn data_selects_and_folds() {
        let mut s = init();
        s.theme = fixture("tokyo-night");
        s.section = Some("data");
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
        press(&mut rt, "files-4");
        assert_eq!(rt.state.data.row, 4);
        press(&mut rt, "files-col-0");
        assert!(!rt.state.data.sort_ascending);
        press(&mut rt, "tree-qt-toggle");
        assert!(!rt.state.data.expanded.iter().any(|e| e == "qt"));
        press(&mut rt, "tree-spec/tokens");
        assert_eq!(rt.state.data.node, "spec/tokens");
    }

    /// The sidebar picks, the split handle drags, the calendar picks a day and steps months.
    #[test]
    fn layout_picks_drags_and_steps() {
        let mut s = init();
        s.theme = fixture("tokyo-night");
        s.section = Some("layout");
        let mut rt = Runtime::new(s, view, (WIDTH, 900.0), Env { system_dark: true, scale: 2.0 });
        rt.frame(0.0);
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
}
