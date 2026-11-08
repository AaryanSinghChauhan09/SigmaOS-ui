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
