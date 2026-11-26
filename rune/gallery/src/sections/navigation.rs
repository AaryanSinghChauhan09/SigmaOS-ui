//! The `navigation` section (spec/gallery.md).

use omarchy_rune::Theme;
use omarchy_rune::ui::{Button, Heading, IconButton, IconRail, Tabs, Toolbar, toolbar_separator};
use rune::{El, el, row, text};

use crate::State;

/// Which tab and rail item are chosen.
pub struct Navigation {
    pub tab: usize,
    pub panel: usize,
}

impl Navigation {
    pub fn init() -> Navigation {
        Navigation { tab: 1, panel: 1 }
    }
}

const PANELS: [(&str, &str); 5] = [
    ("\u{f07b}", "Files"),
    ("\u{f002}", "Search"),
    ("\u{f126}", "Source control"),
    ("\u{f188}", "Debug"),
    ("\u{f013}", "Settings"),
];

pub fn build(s: &State, t: &Theme, _layer: &mut Vec<El>) -> Vec<El> {
    let tk = t.tokens();
    let n = &s.navigation;
    let icon = |g: &str| IconButton::new(g).build(t);

    // Three groups split by separators, a centered title, a trailing button and the overflow.
    let toolbar = Toolbar::new()
        .leading([
            icon("\u{f032}"),
            IconButton::new("\u{f033}").cursor(true).build(t),
            icon("\u{f0cd}"),
            toolbar_separator(t),
            icon("\u{f0ca}"),
            icon("\u{f0cb}"),
            toolbar_separator(t),
            icon("\u{f03e}"),
            icon("\u{f0c1}"),
        ])
        .center([Heading::new("notes.md").centered(true).build(t)])
        .trailing([Button::new("Share").bordered(true).build(t)])
        .overflow(true)
        .build(t);

    let tabs = Tabs::new(["General", "Display", "Audio", "Network"], n.tab)
        .cursor(Some(2))
        .on_choose(|s: &mut State, i| s.navigation.tab = i)
        .build(t);

    let rail = IconRail::new(PANELS, n.panel)
        .on_choose(|s: &mut State, i| s.navigation.panel = i)
        .build(t);
    let rail_h = {
        let sp = &tk.spacing;
        let size = (sp.control_height + sp.sm * 2) as f32;
        sp.sm as f32 * 2.0 + 5.0 * size + 4.0 * sp.sm as f32
    };
    // Centered on whole points, as Qt's anchors.centerIn places it.
    let name = PANELS[n.panel].1;
    let style = t.text_style(tk.font.body);
    let w = t.space(320.0);
    let (x, y) = (
        ((w - rune::text::advance(name, style)) / 2.0).round(),
        ((rail_h - style.line_height) / 2.0).round(),
    );
    let content = el().size(w, rail_h).child(
        text(name)
            .font(style)
            .color(t.dim())
            .single_line()
            .absolute(x, y),
    );

    vec![
        toolbar,
        tabs,
        row().items_start().child(rail).child(content),
    ]
}
