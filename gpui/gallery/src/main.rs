//! The GPUI gallery: every Omarchy UI component in the live theme, section for section with the
//! Qt gallery (spec/gallery.md), following theme switches live.
//!
//!   omarchy-gpui-gallery                                   a window, following the desktop
//!   omarchy-gpui-gallery --section buttons                 a window with one section
//!   omarchy-gpui-gallery --snap out.png --section buttons --size 760x400 --scale 2
//!                                                          one section to a PNG, offscreen
//!   omarchy-gpui-gallery --list-sections                   the sections, in order
//!   omarchy-gpui-gallery --dump-tokens                     the core's tokens as canonical JSON
//!
//! `--snap` renders with `omarchy_gpui::snap`: GPUI's headless app context, the test platform's windows (never on
//! screen), Linux's real text system and the wgpu headless renderer reading the frame back, so
//! stills need a GPU adapter (any Vulkan or GL driver, llvmpipe included) but no display.
//!
//! Each section is `sections/<name>.rs` with `pub fn build(g, window, cx) -> Vec<AnyElement>`.
//! Adding one: write the module and list it in `gallery!` below, under the Qt section's name.

use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{TextInput, section_header};
use omarchy_gpui::{Paint, ui};

mod sections;

pub const WIDTH: f32 = 760.0;

pub struct Gallery {
    pub section: Option<&'static str>,
    pub switches: [bool; 4],
    pub wifi: bool,
    pub bluetooth: bool,
    pub search: Entity<TextInput>,
    pub name: Entity<TextInput>,
    pub hovered: Entity<TextInput>,
    pub gaps: i64,
    pub theme_choice: SharedString,
    pub level: f32,
    pub stepped: f32,
    pub align: SharedString,
    pub network: SharedString,
    pub night_light: bool,
    pub night_strength: f32,
    pub scroll: ScrollHandle,
    pub more: sections::More,
}

impl Gallery {
    pub fn new(section: Option<&'static str>, cx: &mut Context<Gallery>) -> Gallery {
        Gallery {
            section,
            switches: [false, true, false, true],
            wifi: true,
            bluetooth: false,
            search: TextInput::new(cx, ""),
            name: TextInput::new(cx, "Omarchy"),
            hovered: TextInput::new(cx, "Hovered"),
            gaps: 10,
            theme_choice: "tokyo-night".into(),
            level: 0.35,
            stepped: 0.75,
            align: "center".into(),
            network: "basecamp".into(),
            night_light: true,
            night_strength: 0.6,
            scroll: ScrollHandle::new(),
            more: sections::More::new(cx),
        }
    }
}

/// The sections, in spec/gallery.md order: `name: "Title"`.
macro_rules! gallery {
    ($($name:ident: $title:literal),* $(,)?) => {
        pub const SECTIONS: &[&str] = &[$(stringify!($name)),*];
        fn sections(g: &mut Gallery, window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
            let mut out = Vec::new();
            $(
                if g.section.is_none_or(|only| only == stringify!($name)) {
                    let body = sections::$name::build(g, window, cx);
                    out.push(section(cx, $title, body));
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

fn section(cx: &App, title: &str, body: Vec<AnyElement>) -> AnyElement {
    let t = Paint::of(cx);
    let sp = &t.tokens().spacing;
    div()
        .flex()
        .flex_col()
        .w_full()
        .gap(px(sp.lg as f32))
        .child(section_header(&t, title.to_string()))
        .child(div().flex().flex_col().w_full().gap(px(sp.xxl as f32)).children(body))
        .into_any_element()
}

impl Render for Gallery {
    fn render(&mut self, window: &mut Window, cx: &mut Context<Self>) -> impl IntoElement {
        let t = Paint::of(cx);
        let sp = t.tokens().spacing.clone();
        let body = sections(self, window, cx);
        div()
            .size_full()
            .bg(t.bg())
            .font_family(t.family())
            .text_color(t.fg())
            .child(
                div()
                    .id("page")
                    .size_full()
                    .overflow_y_scroll()
                    .track_scroll(&self.scroll)
                    .child(div().flex().flex_col().w_full().p(px(sp.panel_padding as f32)).gap(px(sp.huge as f32 * 2.0)).children(body)),
            )
    }
}

fn arg(name: &str) -> Option<String> {
    let args: Vec<String> = std::env::args().collect();
    args.iter().position(|a| a == name).and_then(|i| args.get(i + 1).cloned())
}

fn section_arg() -> Option<&'static str> {
    let name = arg("--section").or_else(|| arg("--scene"))?;
    if name == "all" {
        return None;
    }
    match SECTIONS.iter().find(|s| **s == name) {
        Some(s) => Some(*s),
        None => {
            eprintln!("omarchy-gpui-gallery: no section {name:?} (--list-sections)");
            std::process::exit(2);
        }
    }
}

use omarchy_gpui::omarchy_theme;

fn main() {
    let args: Vec<String> = std::env::args().collect();
    if args.iter().any(|a| a == "--dump-tokens") {
        println!("{}", omarchy_theme::load().to_canonical_json());
        return;
    }
    if args.iter().any(|a| a == "--list-sections") {
        println!("{}", SECTIONS.join("\n"));
        return;
    }
    let section = section_arg();
    if let Some(out) = arg("--snap") {
        let (w, h) = arg("--size")
            .and_then(|s| s.split_once('x').and_then(|(w, h)| Some((w.parse().ok()?, h.parse().ok()?))))
            .unwrap_or((WIDTH, 900.0));
        if arg("--scale").is_some_and(|s| s != "2") {
            eprintln!("omarchy-gpui-gallery: --snap renders at 2x (GPUI's test windows); --scale is ignored");
        }
        if let Err(e) = omarchy_gpui::snap::render(std::path::Path::new(&out), (w, h), |_, cx| Gallery::new(section, cx)) {
            eprintln!("omarchy-gpui-gallery: {e:#}");
            std::process::exit(1);
        }
        return;
    }
    omarchy_gpui::gpui_platform::application().run(move |cx| {
        gpui_component::init(cx);
        omarchy_gpui::init(cx);
        let bounds = Bounds::centered(None, size(px(WIDTH), px(900.0)), cx);
        cx.open_window(
            WindowOptions { window_bounds: Some(WindowBounds::Windowed(bounds)), app_id: Some("omarchy-gpui-gallery".into()), ..Default::default() },
            |window, cx| {
                window.set_window_title("Omarchy UI · GPUI");
                cx.new(|cx| Gallery::new(section, cx))
            },
        )
        .expect("open the gallery window");
        cx.activate(true);
    });
    let _ = ui::Card::new;
}
