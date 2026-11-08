//! The Rune gallery: every Omarchy UI primitive in the live theme, section for section with the
//! Qt gallery (spec/gallery.md), following theme switches live.
//!
//!   omarchy-rune-gallery                                      a window
//!   omarchy-rune-gallery --snap out.png --scene buttons       one section, headless (Rune's tooling)
//!   omarchy-rune-gallery --dump-tokens                        canonical token JSON
//!   OMARCHY_UI_THEME_DIR=fixtures/themes/nord omarchy-rune-gallery --snap nord.png --scene all

use omarchy_rune::ui::{
    Button, ButtonGroup, Card, ConfirmDialog, Dropdown, IconButton, MenuRow, NumberField, Slider, TextField, Toggle,
    ToggleSwitch, Tooltip, section_header, separator,
};
use omarchy_rune::{Theme, border, color};
use rune::{AppSpec, Cx, El, TextEdit, col, el, row, text};

const WIDTH: f32 = 760.0;

struct State {
    theme: Theme,
    section: Option<&'static str>,
    switches: [bool; 4],
    wifi: bool,
    bluetooth: bool,
    search: TextEdit,
    name: TextEdit,
    hovered: TextEdit,
    gaps: i64,
    theme_choice: String,
    dropdown_open: bool,
    level: f32,
    stepped: f32,
    align: String,
    network: String,
}

fn init() -> State {
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
    }
}

fn section(s: &State, t: &Theme, name: &str, title: &str, body: Vec<El>) -> Option<El> {
    if s.section.is_some_and(|only| only != name) {
        return None;
    }
    let sp = &t.tokens().spacing;
    Some(
        col()
            .w_full()
            .gap(sp.lg as f32)
            .child(section_header(t, title))
            .child(col().w_full().gap(sp.xxl as f32).children(body)),
    )
}

fn buttons(s: &State, t: &Theme) -> Vec<El> {
    let gap = t.tokens().spacing.control_gap as f32;
    vec![
        row()
            .items_center()
            .gap(gap)
            .child(Button::new("Plain").build(t))
            .child(Button::new("Bordered").bordered(true).build(t))
            .child(Button::new("Hover").bordered(true).cursor(true).build(t))
            .child(Button::new("Selected").bordered(true).selected(true).build(t))
            .child(Button::new("Icon").icon("\u{f013}").bordered(true).build(t))
            .child(IconButton::new("\u{f011}").label("Power").bordered(true).build(t))
            .child(IconButton::new("\u{f021}").label("Refresh").bordered(true).cursor(true).build(t)),
        ButtonGroup::new([("left", "Left"), ("center", "Center"), ("right", "Right")], s.align.clone())
            .on_choose(|s: &mut State, v: &str| s.align = v.into())
            .build(t),
    ]
}

fn toggles(s: &State, t: &Theme) -> Vec<El> {
    let gap = t.tokens().spacing.xxl as f32;
    let mut switches = row().items_center().gap(gap);
    for i in 0..4 {
        switches = switches.child(
            ToggleSwitch::new(s.switches[i])
                .key(format!("switch-{i}"))
                .cursor(i >= 2)
                .on_toggle(move |s: &mut State| s.switches[i] ^= true)
                .build(t),
        );
    }
    vec![
        switches,
        Toggle::new("Wi-Fi")
            .description("Connected to Basecamp")
            .checked(s.wifi)
            .width(360.0)
            .on_toggle(|s: &mut State| s.wifi ^= true)
            .build(t),
        Toggle::new("Bluetooth")
            .description("Off")
            .checked(s.bluetooth)
            .cursor(true)
            .width(360.0)
            .on_toggle(|s: &mut State| s.bluetooth ^= true)
            .build(t),
    ]
}

fn inputs(s: &State, t: &Theme, layer: &mut Vec<El>) -> Vec<El> {
    let sp = &t.tokens().spacing;
    vec![
        row()
            .gap(sp.control_gap as f32)
            .child(TextField::new(&s.search).placeholder("Search…").width(220.0).on_edit(|s: &mut State| &mut s.search).build(t))
            .child(TextField::new(&s.name).width(220.0).on_edit(|s: &mut State| &mut s.name).build(t))
            .child(TextField::new(&s.hovered).cursor(true).width(220.0).on_edit(|s: &mut State| &mut s.hovered).build(t)),
        row()
            .items_start()
            .gap(sp.xxl as f32)
            .child(NumberField::new("Gaps", s.gaps).range(0, 40).on_change(|s: &mut State, v| s.gaps = v).build(t))
            .child(
                Dropdown::new("theme-dropdown", [("tokyo-night", "Tokyo Night"), ("nord", "Nord")], s.theme_choice.clone())
                    .label("Theme")
                    .open(s.dropdown_open)
                    .on_toggle(|s: &mut State| s.dropdown_open ^= true)
                    .on_choose(|s: &mut State, v: &str| {
                        s.theme_choice = v.into();
                        s.dropdown_open = false;
                    })
                    .build(t, layer),
            ),
    ]
}

fn slider(s: &State, t: &Theme) -> Vec<El> {
    vec![
        Slider::new(s.level).width(360.0).on_change(|s: &mut State, v| s.level = v).build(t),
        Slider::new(s.stepped).ticks(5).width(360.0).on_change(|s: &mut State, v| s.stepped = (v * 4.0).round() / 4.0).build(t),
    ]
}

fn surfaces(s: &State, t: &Theme) -> Vec<El> {
    let sp = &t.tokens().spacing;
    let network = |id: &'static str, label: &str| {
        MenuRow::new(label).checked(s.network == id).on_press(move |s: &mut State| s.network = id.into())
    };
    let card = Card::new()
        .title("Network")
        .width(300.0)
        .child(Toggle::new("Wi-Fi").description("Basecamp").checked(s.wifi).on_toggle(|s: &mut State| s.wifi ^= true).build(t))
        .child(separator(t))
        .child(
            col()
                .w_full()
                .child(network("basecamp", "Basecamp").build(t))
                .child(network("guest", "Basecamp Guest").highlighted(true).build(t))
                .child(network("coffee", "Coffee Shop").build(t)),
        )
        .build(t);
    let side = col()
        .gap(sp.huge as f32)
        .items_start()
        .child(Tooltip::new("Restart Wi-Fi").build(t))
        .child(el().size(340.0, 170.0).center().child(ConfirmDialog::new("Remove the Coffee Shop network?").width(308.0).build(t)));
    vec![row().items_start().gap(sp.huge as f32).child(card).child(side)]
}

fn tokens(t: &Theme) -> Vec<El> {
    let tk = t.tokens();
    let sp = &tk.spacing;
    let p = &tk.palette;
    let mut swatches = row().gap(sp.lg as f32);
    for (name, c) in [
        ("foreground", p.foreground),
        ("background", p.background),
        ("accent", p.accent),
        ("urgent", p.urgent),
        ("muted", p.muted),
    ] {
        swatches = swatches.child(
            col()
                .gap(sp.xs as f32)
                .child(
                    el().size(64.0, 32.0).radius(t.radius_small()).fill(color(c)).border(1.0, color(tk.controls.normal.border.color)),
                )
                .child(text(name).font(t.text_style(tk.font.caption)).color(t.fg())),
        );
    }
    let c = &tk.controls;
    let mut states = row().gap(sp.lg as f32);
    for (name, st) in [("normal", &c.normal), ("hover-cursor", &c.hover), ("focus", &c.focus), ("selected", &c.selected)] {
        let selected = name == "selected";
        let label = text(name)
            .font(if selected { t.text_style_bold(tk.font.body) } else { t.text_style(tk.font.body) })
            .color(color(st.text));
        let chip = el().size(120.0, sp.control_height as f32).center().radius(t.radius()).fill(color(st.fill)).child(label);
        states = states.child(border(chip, &st.border));
    }
    let f = &tk.font;
    let mut scale = col().gap(sp.xs as f32);
    for (name, size) in [
        ("displayLarge", f.display_large),
        ("display", f.display),
        ("heading", f.heading),
        ("title", f.title),
        ("subtitle", f.subtitle),
        ("body", f.body),
        ("bodySmall", f.body_small),
        ("caption", f.caption),
    ] {
        scale = scale.child(text(format!("{name} {size}")).font(t.text_style(size)).color(t.fg()));
    }
    vec![swatches, states, scale]
}

fn view(s: &State, cx: &mut Cx<State>) -> El {
    let t = &s.theme;
    t.apply(cx);
    let sp = &t.tokens().spacing;
    let mut layer = Vec::new();
    let inputs = inputs(s, t, &mut layer);
    let sections = [
        section(s, t, "buttons", "Buttons", buttons(s, t)),
        section(s, t, "toggles", "Toggles", toggles(s, t)),
        section(s, t, "inputs", "Inputs", inputs),
        section(s, t, "slider", "Slider", slider(s, t)),
        section(s, t, "surfaces", "Surfaces", surfaces(s, t)),
        section(s, t, "tokens", "Tokens", tokens(t)),
    ];
    let page = col()
        .w_full()
        .padding(sp.panel_padding as f32)
        .gap(sp.huge as f32 * 2.0)
        .children(sections.into_iter().flatten());
    let body = col().w_full().h_full().fill(t.bg()).child(page.scroll(rune::ScrollSpec { stick_bottom: false }).grow(1.0).min_h(0.0));
    rune_kit::root(body, layer)
}

const SECTIONS: &[&str] = &["buttons", "toggles", "inputs", "slider", "surfaces", "tokens"];

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
        scenes: &[
            ("all", |s: &mut State| s.section = None),
            ("buttons", |s: &mut State| s.section = Some("buttons")),
            ("toggles", |s: &mut State| s.section = Some("toggles")),
            ("inputs", |s: &mut State| s.section = Some("inputs")),
            ("slider", |s: &mut State| s.section = Some("slider")),
            ("surfaces", |s: &mut State| s.section = Some("surfaces")),
            ("tokens", |s: &mut State| s.section = Some("tokens")),
        ],
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
}
