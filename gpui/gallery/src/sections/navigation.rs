//! The `navigation` section (spec/gallery.md).

use omarchy_gpui::Paint;
use omarchy_gpui::gpui::{prelude::*, *};
use omarchy_gpui::ui::{Button, Heading, IconButton, IconRail, Tabs, Toolbar, ToolbarSeparator, icon_rail_item_size, label};

use crate::Gallery;

/// Which tab and rail item are chosen.
pub struct State {
    pub tab: usize,
    pub panel: usize,
}

impl State {
    pub fn new(_cx: &mut App) -> State {
        State { tab: 1, panel: 1 }
    }
}

const PANELS: [(&str, &str); 5] =
    [("\u{f07b}", "Files"), ("\u{f002}", "Search"), ("\u{f126}", "Source control"), ("\u{f188}", "Debug"), ("\u{f013}", "Settings")];

pub fn build(g: &mut Gallery, _window: &mut Window, cx: &mut Context<Gallery>) -> Vec<AnyElement> {
    let t = Paint::of(cx);
    let tk = t.tokens();
    let n = &g.more.navigation;

    // Three groups split by separators, a centered title, a trailing button and the overflow.
    let toolbar = Toolbar::new()
        .leading(IconButton::new("\u{f032}"))
        .leading(IconButton::new("\u{f033}").cursor(true))
        .leading(IconButton::new("\u{f0cd}"))
        .leading(ToolbarSeparator)
        .leading(IconButton::new("\u{f0ca}"))
        .leading(IconButton::new("\u{f0cb}"))
        .leading(ToolbarSeparator)
        .leading(IconButton::new("\u{f03e}"))
        .leading(IconButton::new("\u{f0c1}"))
        .center(Heading::new("notes.md").centered(true))
        .trailing(Button::new("Share").bordered(true))
        .overflow(true);

    let tabs = Tabs::new(["General", "Display", "Audio", "Network"], n.tab).cursor(Some(2)).on_choose(cx.listener(
        |g: &mut Gallery, i: &usize, _, cx| {
            g.more.navigation.tab = *i;
            cx.notify();
        },
    ));

    let rail = IconRail::new(PANELS, n.panel).on_choose(cx.listener(|g: &mut Gallery, i: &usize, _, cx| {
        g.more.navigation.panel = *i;
        cx.notify();
    }));
    let sp = &tk.spacing;
    let rail_h = sp.sm as f32 * 2.0 + 5.0 * icon_rail_item_size(&t) + 4.0 * sp.sm as f32;
    // Centered on whole points, as Qt's anchors.centerIn places it.
    let name = PANELS[n.panel].1;
    let content = div()
        .flex()
        .flex_shrink_0()
        .w(px(t.space(320.0)))
        .h(px(rail_h))
        .items_center()
        .justify_center()
        .child(label(&t, name, tk.font.body, false, t.dim()));

    vec![
        toolbar.into_any_element(),
        tabs.into_any_element(),
        div().flex().flex_row().items_start().child(rail).child(content).into_any_element(),
    ]
}
