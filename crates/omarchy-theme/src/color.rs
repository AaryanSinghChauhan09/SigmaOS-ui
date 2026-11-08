//! Colors as the Omarchy shell reads them: hex, Hyprland's `rgb()/rgba()` forms and `0xaarrggbb`,
//! with an alpha multiplier. Ported from `shell/Commons/BorderGeometry.js` `canonicalColor`.

use std::fmt;

/// An 8-bit sRGB color with straight (not premultiplied) alpha.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash, Default)]
pub struct Rgba {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

impl Rgba {
    pub const TRANSPARENT: Rgba = Rgba { r: 0, g: 0, b: 0, a: 0 };

    pub const fn rgb(r: u8, g: u8, b: u8) -> Self {
        Self { r, g, b, a: 255 }
    }

    /// The same color at `alpha` (0–1), replacing its own alpha, as the shell's `Util.alpha` does.
    pub fn with_alpha(self, alpha: f64) -> Self {
        Self { a: alpha_byte(alpha), ..self }
    }

    /// The color's own alpha multiplied by `alpha` (0–1).
    pub fn times_alpha(self, alpha: f64) -> Self {
        Self { a: alpha_byte(self.a as f64 / 255.0 * alpha), ..self }
    }

    pub fn alpha_f(self) -> f32 {
        self.a as f32 / 255.0
    }

    /// Channels as 0–1 floats, for frameworks that take them that way.
    pub fn to_f32(self) -> [f32; 4] {
        [self.r as f32 / 255.0, self.g as f32 / 255.0, self.b as f32 / 255.0, self.a as f32 / 255.0]
    }

    /// `#rrggbbaa`, lowercase: the canonical form in token dumps.
    pub fn hex(self) -> String {
        format!("#{:02x}{:02x}{:02x}{:02x}", self.r, self.g, self.b, self.a)
    }

    /// Relative luminance (WCAG), for picking legible ink.
    pub fn luminance(self) -> f64 {
        let lin = |c: u8| {
            let c = c as f64 / 255.0;
            if c <= 0.04045 { c / 12.92 } else { ((c + 0.055) / 1.055).powf(2.4) }
        };
        0.2126 * lin(self.r) + 0.7152 * lin(self.g) + 0.0722 * lin(self.b)
    }

    /// Qt's `Qt.darker(color, factor)`, which the shell uses for secondary text (labels at 1.4,
    /// descriptions at 1.5, placeholders at 1.6): `QColor::darker(round(factor × 100))`, which
    /// divides the HSV value with Qt's 16-bit channels and float math. Ported step for step so
    /// halves round the way Qt rounds them (169 / 2 is 84 in Qt, not 85).
    pub fn darker(self, factor: f64) -> Rgba {
        let factor = (factor * 100.0).round() as i64;
        if factor <= 0 {
            return self;
        }
        if factor < 100 {
            return self; // lighter(); the kit never asks for it
        }
        const MAX: f32 = 65535.0;
        let q_round = |d: f32| if d >= 0.0 { (d + 0.5) as i64 } else { (d - 0.5) as i64 };
        let fuzzy_eq = |a: f32, b: f32| (a - b).abs() * 100000.0 <= a.abs().min(b.abs());
        // toHsv()
        let (r, g, b) = (self.r as f32 * 257.0 / MAX, self.g as f32 * 257.0 / MAX, self.b as f32 * 257.0 / MAX);
        let max = r.max(g).max(b);
        let min = r.min(g).min(b);
        let delta = max - min;
        let value = q_round(max * MAX) as u32;
        let (hue, sat) = if delta.abs() <= 0.00001 {
            (u16::MAX as i64, 0i64)
        } else {
            let sat = q_round((delta / max) * MAX);
            let mut h = if fuzzy_eq(r, max) {
                (g - b) / delta
            } else if fuzzy_eq(g, max) {
                2.0 + (b - r) / delta
            } else {
                4.0 + (r - g) / delta
            };
            h *= 60.0;
            if h < 0.0 {
                h += 360.0;
            }
            (q_round(h * 100.0), sat)
        };
        // value / factor, then toRgb()
        let v16 = (value * 100) / factor as u32;
        let h = if hue == u16::MAX as i64 { 0.0 } else { hue as f32 / 6000.0 };
        let s = sat as f32 / MAX;
        let v = v16 as f32 / MAX;
        let (rr, gg, bb) = if s == 0.0 {
            (v, v, v)
        } else {
            let i = h as i32;
            let f = h - i as f32;
            let p = v * (1.0 - s);
            if i & 1 == 1 {
                let q = v * (1.0 - s * f);
                match i {
                    1 => (q, v, p),
                    3 => (p, q, v),
                    _ => (v, p, q),
                }
            } else {
                let t = v * (1.0 - s * (1.0 - f));
                match i {
                    0 => (v, t, p),
                    2 => (p, v, t),
                    _ => (t, p, v),
                }
            }
        };
        // 16-bit channels, read back as QML does (channel / 65535 × 255, rounded).
        let byte = |c: f32| {
            let c16 = q_round(c * MAX).clamp(0, 65535) as f64;
            (c16 / 65535.0 * 255.0).round() as u8
        };
        Rgba { r: byte(rr), g: byte(gg), b: byte(bb), a: self.a }
    }

    /// Mix toward `other` by `t` (0 keeps self, 1 is other), in sRGB like the theme templates' `mix`.
    pub fn mix(self, other: Rgba, t: f64) -> Rgba {
        let t = t.clamp(0.0, 1.0);
        let m = |a: u8, b: u8| (a as f64 + (b as f64 - a as f64) * t).round() as u8;
        Rgba { r: m(self.r, other.r), g: m(self.g, other.g), b: m(self.b, other.b), a: m(self.a, other.a) }
    }
}

impl fmt::Display for Rgba {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(&self.hex())
    }
}

impl serde::Serialize for Rgba {
    fn serialize<S: serde::Serializer>(&self, s: S) -> Result<S::Ok, S::Error> {
        s.serialize_str(&self.hex())
    }
}

pub(crate) fn clamp_alpha(v: f64) -> f64 {
    if v.is_finite() { v.clamp(0.0, 1.0) } else { 0.0 }
}

fn alpha_byte(alpha: f64) -> u8 {
    (clamp_alpha(alpha) * 255.0).round() as u8
}

fn hex2(s: &str) -> Option<u8> {
    u8::from_str_radix(s, 16).ok()
}

fn hex6(s: &str) -> Option<(u8, u8, u8)> {
    if s.len() != 6 || !s.bytes().all(|b| b.is_ascii_hexdigit()) {
        return None;
    }
    Some((hex2(&s[0..2])?, hex2(&s[2..4])?, hex2(&s[4..6])?))
}

/// Parse a literal color and multiply its alpha by `alpha`. Returns None for anything that is not
/// a color literal (a role name or a key reference), which callers resolve themselves.
pub fn parse_color(value: &str, alpha: f64) -> Option<Rgba> {
    let s = value.trim();
    let a = clamp_alpha(alpha);
    let mk = |(r, g, b): (u8, u8, u8), own: f64| Rgba { r, g, b, a: alpha_byte(own * a) };

    if let Some(h) = s.strip_prefix('#') {
        if h.len() == 3 && h.bytes().all(|b| b.is_ascii_hexdigit()) {
            let d: Vec<u8> = h.chars().map(|c| hex2(&format!("{c}{c}")).unwrap()).collect();
            return Some(mk((d[0], d[1], d[2]), 1.0));
        }
        if h.len() == 6 {
            return hex6(h).map(|c| mk(c, 1.0));
        }
        if h.len() == 8 {
            let own = hex2(&h[6..8])? as f64 / 255.0;
            return hex6(&h[0..6]).map(|c| mk(c, own));
        }
        return None;
    }

    let lower = s.to_ascii_lowercase();
    if let Some(inner) = lower.strip_prefix("rgba(").and_then(|r| r.strip_suffix(')')) {
        if inner.len() == 8 && !inner.contains(',') {
            let own = hex2(&inner[6..8])? as f64 / 255.0;
            return hex6(&inner[0..6]).map(|c| mk(c, own));
        }
        let parts: Vec<&str> = inner.split(',').map(str::trim).collect();
        if parts.len() == 4 {
            let ch = |p: &str| p.parse::<u32>().ok().map(|n| n.min(255) as u8);
            let own: f64 = parts[3].parse().ok()?;
            return Some(mk((ch(parts[0])?, ch(parts[1])?, ch(parts[2])?), clamp_alpha(own)));
        }
        return None;
    }
    if let Some(inner) = lower.strip_prefix("rgb(").and_then(|r| r.strip_suffix(')')) {
        if inner.len() == 6 && !inner.contains(',') {
            return hex6(inner).map(|c| mk(c, 1.0));
        }
        let parts: Vec<&str> = inner.split(',').map(str::trim).collect();
        if parts.len() == 3 {
            let ch = |p: &str| p.parse::<u32>().ok().map(|n| n.min(255) as u8);
            return Some(mk((ch(parts[0])?, ch(parts[1])?, ch(parts[2])?), 1.0));
        }
        return None;
    }
    if let Some(h) = lower.strip_prefix("0x")
        && h.len() == 8
    {
        let own = hex2(&h[0..2])? as f64 / 255.0;
        return hex6(&h[2..8]).map(|c| mk(c, own));
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_every_hyprland_form() {
        assert_eq!(parse_color("#abc", 1.0), Some(Rgba::rgb(0xaa, 0xbb, 0xcc)));
        assert_eq!(parse_color("#7aa2f7", 1.0), Some(Rgba::rgb(0x7a, 0xa2, 0xf7)));
        assert_eq!(parse_color("#7aa2f780", 1.0).unwrap().a, 0x80);
        assert_eq!(parse_color("rgba(33ccffee)", 1.0), Some(Rgba { r: 0x33, g: 0xcc, b: 0xff, a: 0xee }));
        assert_eq!(parse_color("rgb(33ccff)", 1.0), Some(Rgba::rgb(0x33, 0xcc, 0xff)));
        assert_eq!(parse_color("rgb(1,2,3)", 1.0), Some(Rgba::rgb(1, 2, 3)));
        assert_eq!(parse_color("rgba(1,2,3,0.5)", 1.0).unwrap().a, 128);
        assert_eq!(parse_color("0x80ff0000", 1.0), Some(Rgba { r: 255, g: 0, b: 0, a: 0x80 }));
        assert_eq!(parse_color("accent", 1.0), None);
    }

    #[test]
    fn alpha_multiplies() {
        assert_eq!(parse_color("#ffffff", 0.25).unwrap().a, 64);
        assert_eq!(parse_color("#ffffff80", 0.5).unwrap().a, 64);
    }
}
