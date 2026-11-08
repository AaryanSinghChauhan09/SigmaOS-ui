use super::*;
use crate::parse::{parse_colors, parse_shell};

fn fixture(name: &str) -> Tokens {
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/themes").join(name);
    let read = |f: &str| std::fs::read_to_string(dir.join(f)).unwrap_or_default();
    resolve(&Sources {
        colors: parse_colors(&read("colors.toml")),
        shell: parse_shell(&read("shell.toml")),
        hypr: HyprValues::default(),
        font_family: "monospace".into(),
        resolved_font_family: "monospace".into(),
        font_file: None,
        theme_name: name.into(),
        icon_theme: "Yaru-blue".into(),
    })
}

#[test]
fn tokyo_night_matches_the_shell() {
    let t = fixture("tokyo-night");
    assert_eq!(t.palette.foreground.hex(), "#a9b1d6ff");
    assert_eq!(t.palette.accent.hex(), "#7aa2f7ff");
    assert_eq!(t.palette.urgent.hex(), "#f7768eff");
    // Foreground-tinted glass: 0.04 fill, 0.4 border at rest; 0.08 / 0.25 under the cursor.
    assert_eq!(t.controls.normal.fill.hex(), "#a9b1d60a");
    assert_eq!(t.controls.normal.border.color.hex(), "#a9b1d666");
    assert_eq!(t.controls.normal.border.widths, [1.0; 4]);
    assert_eq!(t.controls.hover.fill.hex(), "#a9b1d614");
    assert_eq!(t.controls.hover.border.color.hex(), "#a9b1d640");
    assert_eq!(t.controls.selected.fill.hex(), "#a9b1d62e");
    assert_eq!(t.controls.selected.border.widths, [0.0; 4]);
    // Popups take the Hyprland active border through `hyprland.active-border`.
    assert_eq!(t.surfaces.popups.border.color.hex(), "#7aa2f7ff");
    assert_eq!(t.surfaces.popups.border.widths, [2.0; 4]);
    assert_eq!(t.surfaces.tooltip.background.hex(), "#1a1b26f7");
    assert_eq!(t.surfaces.menu.selected_text.hex(), "#7aa2f7ff");
    assert_eq!(t.surfaces.menu.selected_border.color.hex(), "#a9b1d640");
    assert_eq!(t.font.body, 12);
    assert_eq!(t.font.heading, 16);
    assert_eq!(t.spacing.control_height, 28);
    assert_eq!(t.gaps_out, 5);
}

#[test]
fn darker_rounds_halves_as_qt_does() {
    use crate::Rgba;
    // tokyo-night's foreground: 0xa9 / 2 = 84.5, which Qt's 16-bit HSV math lands on 84.
    assert_eq!(Rgba::rgb(0xa9, 0xb1, 0xd6).darker(2.0).hex(), "#54596bff");
    assert_eq!(Rgba::rgb(0xa9, 0xb1, 0xd6).darker(1.5).hex(), "#71768fff");
}

#[test]
fn every_fixture_resolves() {
    let dir = std::path::Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/themes");
    let mut n = 0;
    for entry in std::fs::read_dir(dir).unwrap().flatten() {
        if entry.path().is_dir() {
            let name = entry.file_name().to_string_lossy().into_owned();
            let t = fixture(&name);
            assert!(t.palette.foreground != t.palette.background, "{name}");
            n += 1;
        }
    }
    assert!(n >= 20);
}

#[test]
fn light_themes_say_so() {
    assert!(!fixture("catppuccin-latte").is_dark());
    assert!(fixture("nord").is_dark());
}

#[test]
fn base_size_scales_spacing_and_type() {
    let mut shell = parse_shell("[font]\nbase-size = 18\n");
    shell.insert("controls.hover-cursor-border".into(), "rgba(33ccffee) rgba(00ff99ee) 45deg".into());
    let t = resolve(&Sources { shell, ..Default::default() });
    assert_eq!(t.font.body, 18);
    assert_eq!(t.spacing.control_height, 42);
    let g = t.controls.hover.border.gradient.expect("gradient");
    assert_eq!(g.colors.len(), 2);
    assert_eq!(g.angle, 45.0);
}

#[test]
fn width_specs() {
    use crate::resolve::parse_widths;
    assert_eq!(parse_widths(Some("2"), 0.0), [2.0; 4]);
    assert_eq!(parse_widths(Some("1 2"), 0.0), [1.0, 2.0, 1.0, 2.0]);
    assert_eq!(parse_widths(Some("1 2 3"), 0.0), [1.0, 2.0, 3.0, 2.0]);
    assert_eq!(parse_widths(Some("1 2 3 4"), 0.0), [1.0, 2.0, 3.0, 4.0]);
    assert_eq!(parse_widths(None, 3.0), [3.0; 4]);
}

#[test]
fn dim_and_faint_text_are_the_themes_base24_stops() {
    // tokyo-night: dark_foreground and muted.
    let t = fixture("tokyo-night");
    assert_eq!(t.roles.text_dim.hex(), "#565f89ff");
    assert_eq!(t.roles.text_faint.hex(), "#414868ff");
    // catppuccin-latte is light: its own stops, not anything darker than the text.
    let t = fixture("catppuccin-latte");
    assert_eq!(t.roles.text_dim.hex(), "#9ca0b0ff");
    assert_eq!(t.roles.text_faint.hex(), "#acb0beff");
}

#[test]
fn dim_and_faint_text_fall_back_to_the_foreground() {
    // Legacy short names count; canonical ones win wherever they sit.
    let c = parse_colors("dark_fg = \"#111111\"\ndark_foreground = \"#222222\"\n");
    assert_eq!(c.dark_foreground.unwrap().hex(), "#222222ff");
    assert_eq!(parse_colors("dark_fg = \"#111111\"\n").dark_foreground.unwrap().hex(), "#111111ff");
    // color8 stands in for muted.
    assert_eq!(parse_colors("color8 = \"#333333\"\n").muted_named.unwrap().hex(), "#333333ff");
    // Neither: derived from the foreground, as before.
    let c = parse_colors("foreground = \"#a9b1d6\"\nbackground = \"#1a1b26\"\n");
    assert_eq!((c.dark_foreground, c.muted_named), (None, None));
}
