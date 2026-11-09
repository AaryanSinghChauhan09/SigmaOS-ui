//! `omarchy-ansi`: check, measure, rasterize and diff terminal gallery snapshots
//! (spec/terminal.md). Colors and the font come from the core's tokens under the caller's
//! environment (`OMARCHY_UI_THEME_DIR`, `OMARCHY_UI_FONT_FAMILY`, ...).
//!
//!   omarchy-ansi size IN.ans                       COLSxROWS
//!   omarchy-ansi check IN.ans [--size COLSxROWS]   ok COLSxROWS, or exit 1 with the reason
//!   omarchy-ansi png IN.ans OUT.png [--scale 2] [--font-size 12]
//!   omarchy-ansi diff A.ans B.ans [--png DIFF.png] [--scale 2] [--font-size 12]
//!   omarchy-ansi canon IN.ans                      the canonical form, to stdout

use std::path::Path;
use std::process::ExitCode;

use omarchy_ansi::{Grid, RenderOptions, diff, parse, render, write_canonical};

const USAGE: &str = "usage:
  omarchy-ansi size IN.ans
  omarchy-ansi check IN.ans [--size COLSxROWS]
  omarchy-ansi png IN.ans OUT.png [--scale 2] [--font-size 12]
  omarchy-ansi diff A.ans B.ans [--png DIFF.png] [--scale 2] [--font-size 12]
  omarchy-ansi canon IN.ans";

enum Fail {
    Usage(String),
    Error(String),
}

fn read(path: &str) -> Result<Grid, Fail> {
    let text = std::fs::read_to_string(path).map_err(|e| Fail::Error(format!("{path}: {e}")))?;
    parse(&text).map_err(|e| Fail::Error(format!("{path}: {e}")))
}

/// Positional arguments and `--flag value` options.
type Opts = Vec<(String, String)>;

fn split(args: &[String]) -> Result<(Vec<String>, Opts), Fail> {
    let (mut pos, mut opts) = (Vec::new(), Vec::new());
    let mut it = args.iter();
    while let Some(a) = it.next() {
        if let Some(name) = a.strip_prefix("--") {
            let (k, v) = match name.split_once('=') {
                Some((k, v)) => (k.to_string(), v.to_string()),
                None => (name.to_string(), it.next().ok_or_else(|| Fail::Usage(format!("--{name} needs a value")))?.clone()),
            };
            opts.push((k, v));
        } else {
            pos.push(a.clone());
        }
    }
    Ok((pos, opts))
}

fn opt<'a>(opts: &'a [(String, String)], key: &str) -> Option<&'a str> {
    opts.iter().rev().find(|(k, _)| k == key).map(|(_, v)| v.as_str())
}

fn number(opts: &[(String, String)], key: &str, default: f32) -> Result<f32, Fail> {
    match opt(opts, key) {
        None => Ok(default),
        Some(v) => v.parse::<f32>().ok().filter(|n| *n > 0.0).ok_or_else(|| Fail::Usage(format!("--{key} {v}: not a positive number"))),
    }
}

fn render_options(opts: &[(String, String)]) -> Result<RenderOptions, Fail> {
    let tokens = omarchy_theme::load();
    let mut r = RenderOptions::from_tokens(&tokens).map_err(Fail::Error)?;
    r.scale = number(opts, "scale", 2.0)?;
    r.font_size_px = number(opts, "font-size", 12.0)?;
    Ok(r)
}

fn known(opts: &[(String, String)], allowed: &[&str]) -> Result<(), Fail> {
    match opts.iter().find(|(k, _)| !allowed.contains(&k.as_str())) {
        Some((k, _)) => Err(Fail::Usage(format!("unknown option --{k}"))),
        None => Ok(()),
    }
}

fn run(args: &[String]) -> Result<(), Fail> {
    let Some(cmd) = args.first() else { return Err(Fail::Usage(String::new())) };
    let (pos, opts) = split(&args[1..])?;
    let want = |n: usize| if pos.len() == n { Ok(()) } else { Err(Fail::Usage(format!("{cmd} takes {n} file argument(s)"))) };
    match cmd.as_str() {
        "size" => {
            want(1)?;
            known(&opts, &[])?;
            let g = read(&pos[0])?;
            println!("{}x{}", g.cols, g.rows);
        }
        "check" => {
            want(1)?;
            known(&opts, &["size"])?;
            let g = read(&pos[0])?;
            if let Some(size) = opt(&opts, "size") {
                let (c, r) = size.split_once('x').ok_or_else(|| Fail::Usage(format!("--size {size}: not COLSxROWS")))?;
                let (c, r): (usize, usize) = (
                    c.parse().map_err(|_| Fail::Usage(format!("--size {size}: not COLSxROWS")))?,
                    r.parse().map_err(|_| Fail::Usage(format!("--size {size}: not COLSxROWS")))?,
                );
                if (c, r) != (g.cols, g.rows) {
                    return Err(Fail::Error(format!("{}: {}x{}, want {c}x{r}", pos[0], g.cols, g.rows)));
                }
            }
            println!("ok {}x{}", g.cols, g.rows);
        }
        "canon" => {
            want(1)?;
            known(&opts, &[])?;
            print!("{}", write_canonical(&read(&pos[0])?));
        }
        "png" => {
            want(2)?;
            known(&opts, &["scale", "font-size"])?;
            let g = read(&pos[0])?;
            let r = render_options(&opts)?;
            let img = render(&g, &r).map_err(Fail::Error)?;
            img.write_png(Path::new(&pos[1])).map_err(Fail::Error)?;
        }
        "diff" => {
            want(2)?;
            known(&opts, &["png", "scale", "font-size"])?;
            let (a, b) = (read(&pos[0])?, read(&pos[1])?);
            let tokens = omarchy_theme::load();
            let p = &tokens.palette;
            let (fg, bg) = ([p.foreground.r, p.foreground.g, p.foreground.b], [p.background.r, p.background.g, p.background.b]);
            let d = diff(&a, &b, fg, bg);
            if let Some(out) = opt(&opts, "png") {
                // The cell size the png command renders at, over the union of the two grids.
                let r = render_options(&opts)?;
                let probe = render(&Grid::new(1, 1), &r).map_err(Fail::Error)?;
                let (cw, ch) = (probe.cell_width, probe.cell_height);
                let (w, h) = (d.cols as u32 * cw, d.rows as u32 * ch);
                let mut rgba = vec![0u8; (w * h * 4) as usize];
                for row in 0..d.rows {
                    for col in 0..d.cols {
                        if !d.mask[row * d.cols + col] {
                            continue;
                        }
                        for y in row as u32 * ch..(row as u32 + 1) * ch {
                            for x in col as u32 * cw..(col as u32 + 1) * cw {
                                let i = ((y * w + x) * 4) as usize;
                                rgba[i..i + 4].copy_from_slice(&[0xff, 0x3b, 0x6b, 0x99]);
                            }
                        }
                    }
                }
                omarchy_ansi::write_png(Path::new(out), w, h, &rgba).map_err(Fail::Error)?;
            }
            println!("{}", d.to_json());
        }
        "-h" | "--help" | "help" => println!("{USAGE}"),
        other => return Err(Fail::Usage(format!("unknown command {other}"))),
    }
    Ok(())
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match run(&args) {
        Ok(()) => ExitCode::SUCCESS,
        Err(Fail::Usage(msg)) => {
            if !msg.is_empty() {
                eprintln!("omarchy-ansi: {msg}");
            }
            eprintln!("{USAGE}");
            ExitCode::from(2)
        }
        Err(Fail::Error(msg)) => {
            eprintln!("omarchy-ansi: {msg}");
            ExitCode::from(1)
        }
    }
}
