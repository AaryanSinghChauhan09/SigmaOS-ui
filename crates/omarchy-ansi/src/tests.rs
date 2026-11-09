use super::*;

const FG: [u8; 3] = [0xa9, 0xb1, 0xd6];
const BG: [u8; 3] = [0x1a, 0x1b, 0x26];

fn grid(text: &str) -> Grid {
    parse(text).unwrap_or_else(|e| panic!("{e}: {text:?}"))
}

#[test]
fn every_sgr_form_parses_and_round_trips() {
    let text = concat!(
        "\x1b[0;1;38;2;1;2;3;48;2;4;5;6mAb\x1b[0m  \x1b[2;3;4;7;9mx\x1b[22;23;24;27;29my\x1b[m\n",
        "\x1b[31;42mr\x1b[91;102mR\x1b[39;49md\x1b[38;5;196;48;5;240mi\x1b[0mzz\n",
    );
    let g = grid(text);
    assert_eq!((g.cols, g.rows), (6, 2));
    let a = g.get(0, 0).unwrap();
    assert_eq!((a.text.as_str(), a.fg, a.bg, a.attrs.bold), ("A", Color::Rgb(1, 2, 3), Color::Rgb(4, 5, 6), true));
    let x = g.get(4, 0).unwrap();
    assert!(x.attrs.faint && x.attrs.italic && x.attrs.underline && x.attrs.reverse && x.attrs.crossed);
    assert_eq!(g.get(5, 0).unwrap().attrs, Attrs::default());
    assert_eq!(g.get(0, 1).unwrap().fg, Color::Indexed(1));
    assert_eq!(g.get(0, 1).unwrap().bg, Color::Indexed(2));
    assert_eq!(g.get(1, 1).unwrap().fg, Color::Indexed(9));
    assert_eq!(g.get(1, 1).unwrap().bg, Color::Indexed(10));
    assert_eq!(g.get(3, 1).unwrap().fg, Color::Indexed(196));
    // The canonical form parses back to the same grid, and is a fixed point.
    let canon = write_canonical(&g);
    assert_eq!(grid(&canon), g);
    assert_eq!(write_canonical(&grid(&canon)), canon);
    assert!(canon.starts_with("\x1b[0;1;38;2;1;2;3;48;2;4;5;6mAb\x1b[0m  \x1b[0;2;3;4;7;9mx\x1b[0my\n"), "{canon:?}");
}

#[test]
fn canonical_spaces_drop_their_ink() {
    let g = grid("\x1b[1;38;2;9;9;9;48;2;1;1;1m \x1b[0m \n");
    assert_eq!(write_canonical(&g), "\x1b[0;48;2;1;1;1m \x1b[0m \n");
}

#[test]
fn widths() {
    // A wide character takes two columns; a Nerd Font glyph one; a combining mark none.
    let g = grid("漢a\u{f0928}e\u{301}\n");
    assert_eq!(g.cols, 5);
    assert_eq!(g.get(0, 0).unwrap().width, 2);
    assert_eq!(g.get(1, 0).unwrap().width, 0);
    assert_eq!(g.get(3, 0).unwrap().text, "\u{f0928}");
    assert_eq!(g.get(4, 0).unwrap().text, "e\u{301}");
    assert_eq!(write_canonical(&g), "漢a\u{f0928}e\u{301}\n");
}

#[test]
fn strict_errors() {
    assert!(parse("ab\nabc\n").unwrap_err().contains("line 2"));
    assert!(parse("ab").is_err());
    assert!(parse("a\x1b[2Jb\n").is_err());
    assert!(parse("a\x1b[5mb\n").is_err());
    assert!(parse("a\tb\n").is_err());
}

#[test]
fn diff_counts_cells() {
    let a = grid("abc\ndef\n");
    assert_eq!(diff(&a, &a, FG, BG).differing, 0);
    let d = diff(&a, &grid("abc\ndxf\n"), FG, BG);
    assert_eq!((d.differing, d.total, d.percent), (1, 6, 16.67));
    let d = diff(&a, &grid("abc\nd\x1b[38;2;1;1;1me\x1b[0mf\n"), FG, BG);
    assert_eq!(d.differing, 1);
    // A space's fg shows nowhere; explicit colors equal to the defaults are the defaults.
    let spaces = grid("\x1b[38;2;1;1;1m \x1b[0mb\n");
    assert_eq!(diff(&grid("  b\n").clone(), &grid("  b\n"), FG, BG).differing, 0);
    assert_eq!(diff(&grid(" b\n"), &spaces, FG, BG).differing, 0);
    assert_eq!(diff(&grid("\x1b[38;2;169;177;214;48;2;26;27;38mx\x1b[0m\n"), &grid("x\n"), FG, BG).differing, 0);
    // Reverse is resolved before comparing.
    assert_eq!(diff(&grid("\x1b[7mx\x1b[0m\n"), &grid("\x1b[38;2;26;27;38;48;2;169;177;214mx\x1b[0m\n"), FG, BG).differing, 0);
    let d = diff(&a, &grid("abcd\n"), FG, BG);
    assert!(!d.size_matches);
    assert_eq!((d.cols, d.rows, d.total, d.differing), (4, 2, 8, 5));
}

fn options() -> Option<RenderOptions> {
    let tokens = omarchy_theme::load();
    RenderOptions::from_tokens(&tokens).ok()
}

#[test]
fn render_is_deterministic_and_sized() {
    let Some(opts) = options() else { return };
    let g = grid("╭─ Wi-Fi ─╮ \x1b[1;38;2;122;162;247m\u{f0928} Basecamp\x1b[0m\n│\x1b[48;2;40;40;60m ▕Ok▏ ⠿ \x1b[0m│ ━━━●────   \n");
    let a = render(&g, &opts).unwrap();
    let b = render(&g, &opts).unwrap();
    assert_eq!(a, b);
    assert_eq!((a.width, a.height), (g.cols as u32 * a.cell_width, g.rows as u32 * a.cell_height));
    assert_eq!(a.rgba.len() as u32, a.width * a.height * 4);
}

#[test]
fn box_lines_join_across_cells() {
    let Some(opts) = options() else { return };
    let img = render(&grid("\x1b[38;2;255;255;255m─┼─\x1b[0m\n"), &opts).unwrap();
    let rows_lit = |col: u32| -> Vec<u32> {
        (0..img.cell_height)
            .filter(|&y| {
                let x = col * img.cell_width + if col == 1 { 0 } else { img.cell_width / 2 };
                let i = ((y * img.width + x) * 4) as usize;
                img.rgba[i] == 255
            })
            .collect()
    };
    // The ─ on either side and the ┼'s arms light the same rows of pixels.
    assert!(!rows_lit(0).is_empty());
    assert_eq!(rows_lit(0), rows_lit(1));
    assert_eq!(rows_lit(0), rows_lit(2));
}
