//! The two theme files, read the way the shell reads them (`Color.qml` `loadColors` and
//! `parseShell`). shell.toml is not strictly TOML (bare width lists such as `1 0 1 0` are allowed),
//! so this is a line walker, not a TOML parser.

use std::collections::BTreeMap;

use crate::color::{Rgba, parse_color};

/// What `colors.toml` contributes.
#[derive(Clone, Debug, PartialEq)]
pub struct ColorsFile {
    pub foreground: Rgba,
    pub background: Rgba,
    pub accent: Rgba,
    pub urgent: Rgba,
    pub muted: Rgba,
    pub success: Rgba,
    pub warning: Rgba,
    /// `dark_foreground` (or legacy `dark_fg`): the theme's own secondary-text stop.
    pub dark_foreground: Option<Rgba>,
    /// `muted` or `color8` when the theme names one (`muted` above falls back to the foreground).
    pub muted_named: Option<Rgba>,
    pub dark: bool,
    /// Every `key = "#rrggbb"` in the file, by key.
    pub named: BTreeMap<String, Rgba>,
}

impl Default for ColorsFile {
    fn default() -> Self {
        Self {
            foreground: Rgba::rgb(0xca, 0xcc, 0xcc),
            background: Rgba::rgb(0x10, 0x13, 0x15),
            accent: Rgba::rgb(0xca, 0xcc, 0xcc),
            urgent: Rgba::rgb(0xa5, 0x55, 0x55),
            muted: Rgba::rgb(0x70, 0x78, 0x80),
            success: Rgba::rgb(0x7f, 0x9f, 0x6f),
            warning: Rgba::rgb(0xc0, 0xa0, 0x60),
            dark_foreground: None,
            muted_named: None,
            dark: true,
            named: BTreeMap::new(),
        }
    }
}

fn split_kv(line: &str) -> Option<(&str, &str)> {
    let (k, v) = line.split_once('=')?;
    let k = k.trim();
    if k.is_empty() || !k.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_' || b == b'-') {
        return None;
    }
    Some((k, v.trim()))
}

pub fn parse_colors(raw: &str) -> ColorsFile {
    let mut out = ColorsFile::default();
    let (mut fg, mut bg, mut accent, mut muted) = (None, None, None, None);
    let (mut urgent, mut success, mut warning) = (None, None, None);
    for line in raw.lines() {
        let Some((key, value)) = split_kv(line) else { continue };
        let unquoted = value.trim_matches(|c| c == '"' || c == '\'');
        if key == "mode" {
            out.dark = !unquoted.eq_ignore_ascii_case("light");
            continue;
        }
        // The shell only takes `#rrggbb` here (the first seven characters of the value).
        let v = value.trim_start_matches(['"', '\'']);
        if v.len() < 7 || !v.starts_with('#') {
            continue;
        }
        let Some(c) = parse_color(&v[..7], 1.0) else { continue };
        out.named.insert(key.to_string(), c);
        match key {
            "foreground" => fg = Some(c),
            "background" => bg = Some(c),
            "accent" => accent = Some(c),
            "muted" => muted = Some(c),
            "red" | "color1" => urgent = Some(c),
            "green" | "color2" => success = Some(c),
            "yellow" | "color3" => warning = Some(c),
            _ => {}
        }
    }
    let n = |k: &str| out.named.get(k).copied();
    out.background = bg.or(n("color0")).unwrap_or(out.background);
    out.foreground = fg.or(n("color7")).unwrap_or(out.foreground);
    out.accent = accent.or(n("color4")).unwrap_or(out.accent);
    out.muted_named = muted.or(n("color8"));
    out.muted = out.muted_named.unwrap_or(out.foreground);
    // Canonical names win over the legacy short ones, wherever they sit in the file.
    out.dark_foreground = n("dark_foreground").or(n("dark_fg"));
    out.urgent = urgent.unwrap_or(out.urgent);
    out.success = success.unwrap_or(out.success);
    out.warning = warning.unwrap_or(out.warning);
    out
}

/// A flat `"section.key" -> raw string` map, as the shell's `shellValues`.
pub type ShellValues = BTreeMap<String, String>;

pub fn parse_shell(raw: &str) -> ShellValues {
    let mut out = ShellValues::new();
    let mut section = String::new();
    for line in raw.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        if let Some(rest) = line.strip_prefix('[') {
            if let Some((name, tail)) = rest.split_once(']') {
                let tail = tail.trim();
                if (tail.is_empty() || tail.starts_with('#'))
                    && !name.is_empty()
                    && name.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_' || b == b'-')
                {
                    section = name.to_string();
                }
            }
            continue;
        }
        let Some((key, value)) = split_kv(line) else { continue };
        if section.is_empty() {
            continue;
        }
        if let Some(v) = shell_value(value) {
            out.insert(format!("{section}.{key}"), v);
        }
    }
    out
}

/// One value: a quoted string, a number, a bare width list (`1 0 1 0`), or a bare word, with an
/// optional trailing comment.
fn shell_value(v: &str) -> Option<String> {
    let v = v.trim();
    if let Some(q) = v.chars().next().filter(|c| *c == '"' || *c == '\'') {
        let rest = &v[1..];
        let end = rest.find(['"', '\''])?;
        let inner = &rest[..end];
        let tail = rest[end + 1..].trim();
        let _ = q;
        if inner.is_empty() || !(tail.is_empty() || tail.starts_with('#')) {
            return None;
        }
        return Some(inner.to_string());
    }
    let body = match v.find('#') {
        Some(i) => v[..i].trim(),
        None => v,
    };
    if body.is_empty() {
        return None;
    }
    let is_num = |s: &str| {
        let s = s.strip_prefix('-').unwrap_or(s);
        let mut parts = s.splitn(2, '.');
        let int = parts.next().unwrap_or("");
        !int.is_empty()
            && int.bytes().all(|b| b.is_ascii_digit())
            && parts.next().is_none_or(|f| !f.is_empty() && f.bytes().all(|b| b.is_ascii_digit()))
    };
    let words: Vec<&str> = body.split_whitespace().collect();
    if words.len() == 1 && is_num(words[0]) {
        return Some(words[0].to_string());
    }
    if (2..=4).contains(&words.len()) && words.iter().all(|w| is_num(w)) {
        return Some(words.join(" "));
    }
    let bare = body.starts_with(|c: char| c.is_ascii_alphabetic())
        && body.bytes().all(|b| b.is_ascii_alphanumeric() || b == b'_' || b == b'-');
    bare.then(|| body.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn shell_values() {
        let v = parse_shell(
            "[bar]\nbackground = \"#1a1b26\" # c\nscale-with-font = true\nsize = 26\n[popups]\nborder-width = 1 0 1 0\nx = \"a\" junk\n",
        );
        assert_eq!(v["bar.background"], "#1a1b26");
        assert_eq!(v["bar.scale-with-font"], "true");
        assert_eq!(v["bar.size"], "26");
        assert_eq!(v["popups.border-width"], "1 0 1 0");
        assert!(!v.contains_key("popups.x"));
    }

    #[test]
    fn colors_fallbacks() {
        let c = parse_colors("color0 = \"#000000\"\ncolor7 = \"#eeeeee\"\ncolor4 = \"#0000ff\"\ncolor1 = \"#ff0000\"\n");
        assert_eq!(c.background, Rgba::rgb(0, 0, 0));
        assert_eq!(c.foreground, Rgba::rgb(0xee, 0xee, 0xee));
        assert_eq!(c.accent, Rgba::rgb(0, 0, 0xff));
        assert_eq!(c.urgent, Rgba::rgb(0xff, 0, 0));
        assert_eq!(c.muted, c.foreground);
    }
}
