//! gpui-component's theme follows Omarchy's (the `gpui-component` feature): on install, and on a
//! change, mode and colors included.
#![cfg(feature = "gpui-component")]

mod common;

use common::fixture;
use gpui::TestAppContext;
use gpui_component::Theme as GcTheme;
use omarchy_gpui::color;

#[gpui::test]
fn gpui_component_theme_follows(cx: &mut TestAppContext) {
    cx.update(|cx| {
        gpui_component::init(cx);
        omarchy_gpui::install(cx, fixture("tokyo-night"));
        let t = fixture("tokyo-night");
        let g = GcTheme::global(cx);
        assert!(g.mode.is_dark());
        assert_eq!(g.background, color(t.palette.background));
        assert_eq!(g.foreground, color(t.palette.foreground));
        assert_eq!(g.radius, gpui::px(t.radius as f32));
        assert!(!g.shadow);

        let mut latte = fixture("catppuccin-latte");
        latte.reduce_motion = true; // land at once, so the check needs no clock
        omarchy_gpui::set_tokens(cx, latte.clone());
        let g = GcTheme::global(cx);
        assert!(!g.mode.is_dark());
        assert_eq!(g.background, color(latte.palette.background));
        assert_eq!(g.danger, color(latte.palette.urgent));
    });
}
