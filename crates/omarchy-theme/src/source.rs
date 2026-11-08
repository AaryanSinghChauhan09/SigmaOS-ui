//! Loading the sources from the machine: theme files, the user's override, Hyprland's socket and
//! fontconfig. Every `OMARCHY_UI_*` override from `spec/tokens.md` is honored here.

use std::io::{Read, Write};
use std::os::unix::net::UnixStream;
use std::path::{Path, PathBuf};
use std::process::Command;
use std::time::Duration;

use crate::parse::{parse_colors, parse_shell};
use crate::resolve::{HyprValues, Sources};

fn home() -> PathBuf {
    std::env::var_os("HOME").map(PathBuf::from).unwrap_or_else(|| PathBuf::from("/"))
}

/// `~/.local/state/omarchy/current`, the directory a theme switch swaps the theme into.
pub fn current_dir() -> PathBuf {
    home().join(".local/state/omarchy/current")
}

/// The theme directory to read: `OMARCHY_UI_THEME_DIR`, else the live theme.
pub fn theme_dir() -> PathBuf {
    match std::env::var_os("OMARCHY_UI_THEME_DIR") {
        Some(d) if !d.is_empty() => PathBuf::from(d),
        _ => current_dir().join("theme"),
    }
}

/// True when rendering a fixed theme directory rather than the live desktop.
pub fn is_pinned() -> bool {
    std::env::var_os("OMARCHY_UI_THEME_DIR").is_some_and(|d| !d.is_empty())
}

/// `~/.config/omarchy/shell.toml`, the machine-level override.
pub fn user_shell_path() -> PathBuf {
    home().join(".config/omarchy/shell.toml")
}

fn read(path: &Path) -> String {
    std::fs::read_to_string(path).unwrap_or_default()
}

fn env_f64(name: &str) -> Option<f64> {
    std::env::var(name).ok().and_then(|v| v.trim().parse().ok())
}

/// The running Hyprland instance's directory under `$XDG_RUNTIME_DIR/hypr`: the one named by
/// `HYPRLAND_INSTANCE_SIGNATURE`, else the most recently started one (apps launched outside the
/// session environment don't inherit the signature).
pub fn hypr_dir() -> Option<PathBuf> {
    let runtime = std::env::var_os("XDG_RUNTIME_DIR").map(PathBuf::from)?;
    let base = runtime.join("hypr");
    if let Some(sig) = std::env::var_os("HYPRLAND_INSTANCE_SIGNATURE").filter(|s| !s.is_empty()) {
        let dir = base.join(sig);
        if dir.join(".socket.sock").exists() {
            return Some(dir);
        }
    }
    let mut best: Option<(std::time::SystemTime, PathBuf)> = None;
    for entry in std::fs::read_dir(&base).ok()?.flatten() {
        let dir = entry.path();
        if !dir.join(".socket.sock").exists() {
            continue;
        }
        let t = entry.metadata().and_then(|m| m.modified()).unwrap_or(std::time::UNIX_EPOCH);
        if best.as_ref().is_none_or(|(bt, _)| t > *bt) {
            best = Some((t, dir));
        }
    }
    best.map(|(_, d)| d)
}

/// One `j/getoption` request on Hyprland's command socket.
fn hypr_option(dir: &Path, name: &str) -> Option<serde_json::Value> {
    let mut s = UnixStream::connect(dir.join(".socket.sock")).ok()?;
    s.set_read_timeout(Some(Duration::from_millis(500))).ok()?;
    s.write_all(format!("j/getoption {name}").as_bytes()).ok()?;
    let mut out = String::new();
    s.read_to_string(&mut out).ok()?;
    serde_json::from_str(&out).ok()
}

fn first_number(css: &str) -> Option<f64> {
    css.split(|c: char| !(c.is_ascii_digit() || c == '.' || c == '-')).find_map(|t| t.parse().ok())
}

/// Hyprland's values, with `OMARCHY_UI_*` overrides on top. Falls back to the defaults when
/// Hyprland isn't running.
pub fn hypr_values() -> HyprValues {
    let mut h = HyprValues::default();
    let pinned = is_pinned();
    // A pinned theme render must not depend on the desktop it runs on.
    if !pinned && let Some(dir) = hypr_dir() {
        if let Some(n) = hypr_option(&dir, "decoration:rounding").and_then(|v| v["int"].as_f64()) {
            h.rounding = n;
        }
        if let Some(v) = hypr_option(&dir, "general:gaps_out") {
            let css = v["css"].as_str().and_then(first_number);
            if let Some(n) = css.or_else(|| v["int"].as_f64()) {
                h.gaps_out = n;
            }
        }
        if let Some(b) = hypr_option(&dir, "animations:enabled").and_then(|v| v["bool"].as_bool()) {
            h.animations = b;
        }
        if let Some(n) = hypr_option(&dir, "general:border_size").and_then(|v| v["int"].as_f64()) {
            h.border_size = n;
        }
    }
    if let Some(n) = env_f64("OMARCHY_UI_ROUNDING") {
        h.rounding = n;
    }
    if let Some(n) = env_f64("OMARCHY_UI_GAPS_OUT") {
        h.gaps_out = n;
    }
    if let Ok(v) = std::env::var("OMARCHY_UI_REDUCE_MOTION") {
        h.animations = v.trim() != "1";
    }
    h
}

/// The family `monospace` resolves to and its file, from `fc-match`.
pub fn resolve_font(family: &str) -> (String, Option<PathBuf>) {
    let out = Command::new("fc-match").args(["-f", "%{family[0]}\n%{file}", family]).output();
    match out {
        Ok(o) if o.status.success() => {
            let s = String::from_utf8_lossy(&o.stdout);
            let mut lines = s.lines();
            let name = lines.next().unwrap_or(family).trim().to_string();
            let file = lines.next().map(|f| PathBuf::from(f.trim())).filter(|p| p.exists());
            (if name.is_empty() { family.to_string() } else { name }, file)
        }
        _ => (family.to_string(), None),
    }
}

/// Everything resolution needs, read now.
pub fn load_sources() -> Sources {
    let dir = theme_dir();
    let colors = parse_colors(&read(&dir.join("colors.toml")));
    let mut shell = parse_shell(&read(&dir.join("shell.toml")));
    if !is_pinned() {
        shell.extend(parse_shell(&read(&user_shell_path())));
    }
    let family = std::env::var("OMARCHY_UI_FONT_FAMILY").ok().filter(|f| !f.trim().is_empty());
    let (font_family, (resolved, file)) = match family {
        Some(f) => (f.clone(), resolve_font(&f)),
        None => ("monospace".to_string(), resolve_font("monospace")),
    };
    let theme_name = if is_pinned() {
        dir.file_name().map(|n| n.to_string_lossy().into_owned()).unwrap_or_default()
    } else {
        read(&current_dir().join("theme.name")).trim().to_string()
    };
    Sources {
        colors,
        shell,
        hypr: hypr_values(),
        font_family,
        resolved_font_family: resolved,
        font_file: file,
        theme_name,
        icon_theme: crate::icons::theme_name(),
    }
}
