//! gpui-component's theme from Omarchy's tokens (the `gpui-component` feature), so its widgets
//! read as Omarchy next to the kit's: foreground glass for controls, accent only for what is
//! chosen, urgent for destructive actions, Hyprland's rounding, the `monospace` font, no shadows.
//!
//! Applied by [`crate::init`] and on every change and fade frame, once `gpui_component::init(cx)`
//! has installed its theme. Call `gpui_component::init(cx)` first, then `omarchy_gpui::init(cx)`;
//! an app that calls them the other way round can call [`apply_current`] after.

use gpui::{App, px};
use gpui_component::{Theme as GcTheme, ThemeMode};
use omarchy_theme::{Rgba, Tokens};

use crate::theme::{color, family_of};

/// Map the theme on screen now onto gpui-component's (a no-op before `gpui_component::init`).
pub fn apply_current(cx: &mut App) {
    if let Some(p) = cx.try_global::<crate::Paint>() {
        let t = p.tokens().clone();
        apply(cx, &t);
    }
}

/// Map `t` onto gpui-component's theme (a no-op before `gpui_component::init`).
pub fn apply(cx: &mut App, t: &Tokens) {
    if !cx.has_global::<GcTheme>() {
        return;
    }
    let mode = if t.is_dark() { ThemeMode::Dark } else { ThemeMode::Light };
    if GcTheme::global(cx).mode != mode {
        // Switching mode reloads the mode's registered colors; ours go on top in the next edit.
        GcTheme::change(mode, None, cx);
    }
    GcTheme::update(cx, |g| map(g, t));
}

/// `over` composited onto `under` (straight alpha, sRGB), for the fills gpui-component expects
/// to be opaque.
fn over(under: Rgba, over: Rgba) -> Rgba {
    let a = over.a as f64 / 255.0;
    let m = |u: u8, o: u8| (u as f64 + (o as f64 - u as f64) * a).round() as u8;
    Rgba { r: m(under.r, over.r), g: m(under.g, over.g), b: m(under.b, over.b), a: 255 }
}

fn map(g: &mut GcTheme, t: &Tokens) {
    let p = &t.palette;
    let c = &t.controls;
    let s = &t.surfaces;
    let bg = p.background;
    let solid = |x: Rgba| color(over(bg, x));
    let fg = color(p.foreground);
    let on_urgent = color(p.background);

    g.font_family = family_of(t).into();
    g.mono_font_family = family_of(t).into();
    g.font_size = px(t.font.body as f32);
    g.mono_font_size = px(t.font.body as f32);
    g.radius = px(t.radius as f32);
    g.radius_lg = px(t.radius as f32);
    g.shadow = false;

    let x = &mut g.colors;
    x.background = color(bg);
    x.foreground = fg;
    x.border = color(c.normal.border.color);
    x.input = color(c.normal.border.color);
    x.ring = color(c.focus.border.color);
    x.caret = fg;
    x.selection = color(c.selection.fill);

    // Primary is the chosen state (selected glass, its own ink), not an accent fill.
    x.primary = solid(c.selected.fill);
    x.primary_hover = solid(over(over(bg, c.selected.fill), c.hover.fill));
    x.primary_active = solid(over(over(bg, c.selected.fill), c.pressed.fill));
    x.primary_foreground = color(c.selected.text);
    // Secondary, buttons, muted: the normal glass, stronger under the cursor.
    for (base, hover, active, ink) in [
        (&mut x.secondary, &mut x.secondary_hover, &mut x.secondary_active, &mut x.secondary_foreground),
        (&mut x.button, &mut x.button_hover, &mut x.button_active, &mut x.button_foreground),
        (&mut x.button_secondary, &mut x.button_secondary_hover, &mut x.button_secondary_active, &mut x.button_secondary_foreground),
    ] {
        *base = solid(c.normal.fill);
        *hover = solid(c.hover.fill);
        *active = solid(c.pressed.fill);
        *ink = fg;
    }
    x.button_primary = x.primary;
    x.button_primary_hover = x.primary_hover;
    x.button_primary_active = x.primary_active;
    x.button_primary_foreground = x.primary_foreground;
    x.muted = solid(c.normal.fill);
    x.muted_foreground = color(t.roles.text_dim);
    x.accent = solid(c.hover.fill);
    x.accent_foreground = fg;

    // Destructive, success, warning, info.
    let tone = |base: Rgba| (color(base), solid(base.times_alpha(0.85)), solid(base.times_alpha(0.7)));
    (x.danger, x.danger_hover, x.danger_active) = tone(p.urgent);
    x.danger_foreground = on_urgent;
    (x.button_danger, x.button_danger_hover, x.button_danger_active) = tone(p.urgent);
    x.button_danger_foreground = on_urgent;
    (x.success, x.success_hover, x.success_active) = tone(p.success);
    x.success_foreground = on_urgent;
    (x.button_success, x.button_success_hover, x.button_success_active) = tone(p.success);
    x.button_success_foreground = on_urgent;
    (x.warning, x.warning_hover, x.warning_active) = tone(p.warning);
    x.warning_foreground = on_urgent;
    (x.button_warning, x.button_warning_hover, x.button_warning_active) = tone(p.warning);
    x.button_warning_foreground = on_urgent;
    (x.info, x.info_hover, x.info_active) = tone(p.accent);
    x.info_foreground = on_urgent;
    (x.button_info, x.button_info_hover, x.button_info_active) = tone(p.accent);
    x.button_info_foreground = on_urgent;
    x.link = color(p.accent);
    x.link_hover = color(p.accent);
    x.link_active = color(p.accent);

    // Surfaces.
    x.popover = color(s.popups.background);
    x.popover_foreground = color(s.popups.text);
    x.overlay = color(t.roles.scrim);
    x.window_border = color(s.popups.border.color);
    x.title_bar = color(bg);
    x.title_bar_border = color(t.roles.track);
    x.status_bar = color(bg);
    x.status_bar_border = color(t.roles.track);
    x.sidebar = color(bg);
    x.sidebar_foreground = fg;
    x.sidebar_border = color(t.roles.track);
    x.sidebar_accent = solid(c.hover.fill);
    x.sidebar_accent_foreground = fg;
    x.sidebar_primary = solid(s.menu.selected_background);
    x.sidebar_primary_foreground = color(s.menu.selected_text);
    x.group_box = solid(c.normal.fill);
    x.group_box_foreground = fg;
    x.accordion = color(bg);
    x.description_list_label = solid(c.normal.fill);
    x.description_list_label_foreground = color(t.roles.text_dim);

    // Lists, tables, tabs: the menu surface's selection.
    x.list = color(bg);
    x.list_even = color(bg);
    x.list_head = color(bg);
    x.list_hover = solid(c.hover.fill);
    x.list_active = solid(s.menu.selected_background);
    x.list_active_border = color(s.menu.selected_border.color);
    x.table = color(bg);
    x.table_even = color(bg);
    x.table_head = color(bg);
    x.table_head_foreground = color(t.roles.text_dim);
    x.table_foot = color(bg);
    x.table_foot_foreground = color(t.roles.text_dim);
    x.table_hover = solid(c.hover.fill);
    x.table_active = solid(s.menu.selected_background);
    x.table_active_border = color(s.menu.selected_border.color);
    x.table_row_border = color(t.roles.track);
    x.tab_bar = color(bg);
    x.tab_bar_segmented = solid(c.normal.fill);
    x.tab = color(bg);
    x.tab_foreground = color(t.roles.text_dim);
    x.tab_active = solid(c.selected.fill);
    x.tab_active_foreground = color(c.selected.text);

    // Switches, sliders, progress, scrollbars: track and foreground.
    x.switch = solid(c.normal.fill);
    x.switch_thumb = color(p.foreground.darker(1.25));
    x.slider_bar = fg;
    x.slider_thumb = fg;
    x.progress_bar = fg;
    x.scrollbar = color(Rgba::TRANSPARENT);
    x.scrollbar_thumb = color(t.roles.track);
    x.scrollbar_thumb_hover = color(p.foreground.times_alpha(0.3));
    x.skeleton = color(t.roles.track);
    x.drag_border = color(p.accent);
    x.drop_target = color(c.hover.fill);
    x.chart_1 = color(p.accent);
    x.chart_2 = color(p.success);
    x.chart_3 = color(p.warning);
    x.chart_4 = color(p.urgent);
    x.chart_5 = color(p.muted);
    x.chart_bullish = color(p.success);
    x.chart_bearish = color(p.urgent);
    x.chart_grid = color(t.roles.track);
    let named = |k: &str, d: Rgba| color(p.named_or(k, d));
    x.red = named("red", p.urgent);
    x.red_light = named("bright_red", p.urgent);
    x.green = named("green", p.success);
    x.green_light = named("bright_green", p.success);
    x.yellow = named("yellow", p.warning);
    x.yellow_light = named("bright_yellow", p.warning);
    x.blue = named("blue", p.accent);
    x.blue_light = named("bright_blue", p.accent);
    x.magenta = named("magenta", p.accent);
    x.magenta_light = named("bright_magenta", p.accent);
    x.cyan = named("cyan", p.accent);
    x.cyan_light = named("bright_cyan", p.accent);
}
