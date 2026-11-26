//! Raw theme sources to resolved tokens. A pure function, ported from the Omarchy shell's
//! `Color.qml` (surfaces), `Style.qml` (states, spacing, type) and `Border.qml` (border specs).
//! See `spec/tokens.md`.

use crate::color::{Rgba, clamp_alpha, parse_color};
use crate::parse::{ColorsFile, ShellValues};
use crate::tokens::*;

/// Values only Hyprland knows.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct HyprValues {
    /// `decoration:rounding`.
    pub rounding: f64,
    /// `general:gaps_out`, raw (not yet halved).
    pub gaps_out: f64,
    /// `animations:enabled`.
    pub animations: bool,
    /// `general:border_size`.
    pub border_size: f64,
}

impl Default for HyprValues {
    /// What the shell assumes before Hyprland answers: sharp corners and a 10px outer gap.
    fn default() -> Self {
        Self {
            rounding: 0.0,
            gaps_out: 10.0,
            animations: true,
            border_size: 2.0,
        }
    }
}

/// Everything resolution reads, already loaded.
#[derive(Clone, Debug, Default)]
pub struct Sources {
    pub colors: ColorsFile,
    /// Theme shell.toml merged with the user's override (user keys win).
    pub shell: ShellValues,
    pub hypr: HyprValues,
    pub font_family: String,
    pub resolved_font_family: String,
    pub font_file: Option<std::path::PathBuf>,
    pub theme_name: String,
    /// The icon theme the theme names (`icons.theme`), see [`crate::icons::theme_name`].
    pub icon_theme: String,
    /// The display language code, see [`crate::strings::language`] (empty is English).
    pub language: String,
}

struct Ctx<'a> {
    c: &'a ColorsFile,
    v: &'a ShellValues,
}

const ROLES: &[&str] = &[
    "foreground",
    "text",
    "accent",
    "urgent",
    "muted",
    "background",
    "transparent",
];

impl Ctx<'_> {
    fn get(&self, key: &str) -> Option<&str> {
        self.v
            .get(key)
            .map(String::as_str)
            .filter(|s| !s.is_empty())
    }

    fn role(&self, name: &str) -> Option<Rgba> {
        Some(match name {
            "foreground" | "text" => self.c.foreground,
            "accent" => self.c.accent,
            "urgent" => self.c.urgent,
            "muted" => self.c.muted,
            "background" => self.c.background,
            "transparent" => Rgba::TRANSPARENT,
            _ => return None,
        })
    }

    fn alpha_key(&self, key: &str, fallback: f64) -> f64 {
        match self.get(key).and_then(|s| s.trim().parse::<f64>().ok()) {
            Some(n) if n.is_finite() => clamp_alpha(n),
            _ => fallback,
        }
    }

    /// `Color.flatColor`: the first non-angle word, followed through `section.key` references,
    /// then a role or a literal.
    fn flat_color(&self, value: &str, fallback: Rgba) -> Rgba {
        let first = |s: &str| -> String {
            s.split_whitespace()
                .find(|w| !is_angle(w))
                .unwrap_or(s)
                .trim()
                .to_string()
        };
        let mut token = first(value);
        let mut seen = std::collections::HashSet::new();
        loop {
            let key = token.to_ascii_lowercase();
            match self.v.get(&key) {
                Some(next) if *next != token => {
                    if !seen.insert(key) {
                        return fallback;
                    }
                    token = first(next);
                }
                _ => break,
            }
        }
        let role = token.to_ascii_lowercase();
        if let Some(c) = self.role(&role) {
            return c;
        }
        parse_color(&token, 1.0).unwrap_or(fallback)
    }

    /// `Color.composed`: a color key with its `-alpha` companion, which replaces the alpha.
    fn composed(
        &self,
        color_key: &str,
        alpha_key: &str,
        color_fallback: Rgba,
        alpha_fallback: f64,
    ) -> Rgba {
        let base = match self.get(color_key) {
            Some(raw) => self.flat_color(raw, color_fallback),
            None => color_fallback,
        };
        base.with_alpha(self.alpha_key(alpha_key, alpha_fallback))
    }

    fn text(&self, key: &str, fallback: Rgba) -> Rgba {
        self.get(key)
            .map_or(fallback, |raw| self.flat_color(raw, fallback))
    }

    /// `Border.resolveValueRef`: follow `section.key` references to their value.
    fn resolve_ref(&self, raw: &str) -> String {
        let mut s = raw.trim().to_string();
        let mut seen = std::collections::HashSet::new();
        while is_ref(&s) && seen.insert(s.clone()) {
            match self.get(&s) {
                Some(next) => s = next.trim().to_string(),
                None => break,
            }
        }
        s
    }

    /// `Border.cssColor`: a role or literal at `opacity`, multiplied into its own alpha.
    fn css_color(&self, s: &str, opacity: f64, fallback: Rgba) -> Rgba {
        let role = s.trim().to_ascii_lowercase();
        if role == "transparent" {
            return Rgba::TRANSPARENT;
        }
        if ROLES.contains(&role.as_str()) && role != "muted" {
            return self.role(&role).unwrap().times_alpha(opacity);
        }
        parse_color(s, opacity).unwrap_or(fallback)
    }

    /// `Border.borderValue`: a solid color or a gradient, falling back to `fallback` × opacity.
    fn border_value(
        &self,
        raw: &str,
        fallback: Rgba,
        opacity: f64,
        legacy_gradient: &str,
    ) -> (Rgba, Option<Gradient>) {
        let fb = fallback.times_alpha(opacity);
        let raw = raw.trim();
        let primary = if raw.is_empty() {
            gradient_from(&[], fb)
        } else {
            self.gradient(raw, fb, opacity)
        };
        let color = primary.0.first().copied().unwrap_or(fb);
        let mut gradient = (primary.0.len() > 1).then(|| Gradient {
            colors: primary.0.clone(),
            angle: primary.1,
        });
        if gradient.is_none() && !legacy_gradient.trim().is_empty() {
            let legacy = self.gradient(legacy_gradient, color, opacity);
            if legacy.0.len() > 1 {
                gradient = Some(Gradient {
                    colors: legacy.0,
                    angle: legacy.1,
                });
            }
        }
        (color, gradient)
    }

    fn gradient(&self, raw: &str, fallback: Rgba, opacity: f64) -> (Vec<Rgba>, f64) {
        let mut colors = Vec::new();
        let mut angle = 0.0;
        for part in raw.split_whitespace() {
            if is_angle(part) {
                angle = part.trim_end_matches("deg").parse().unwrap_or(0.0);
            } else {
                colors.push(self.css_color(part, opacity, fallback));
            }
        }
        if colors.is_empty() {
            colors.push(fallback);
        }
        (colors, angle)
    }

    fn value_or(&self, section: &str, keys: &[String]) -> Option<&str> {
        keys.iter()
            .find_map(|k| self.get(&format!("{section}.{k}")))
    }

    /// `Border.surfaceSpec`.
    fn surface_border(
        &self,
        section: &str,
        token: &str,
        fallback: Rgba,
        fallback_width: f64,
        alpha_key: Option<&str>,
    ) -> BorderSpec {
        let alpha_key = alpha_key
            .map(str::to_string)
            .unwrap_or_else(|| format!("{token}-alpha"));
        let opacity = self.alpha_key(&format!("{section}.{alpha_key}"), 1.0);
        let keys = |suffix: &str| -> Vec<String> {
            if token == "border" {
                vec![format!("border{suffix}")]
            } else {
                vec![format!("{token}{suffix}"), format!("border{suffix}")]
            }
        };
        let legacy = self.value_or(section, &keys("-gradient")).unwrap_or("");
        let raw = self.resolve_ref(self.get(&format!("{section}.{token}")).unwrap_or(""));
        let (color, gradient) = self.border_value(&raw, fallback, opacity, legacy);
        let mut widths = parse_widths(self.value_or(section, &keys("-width")), fallback_width);
        for (i, side) in ["top", "right", "bottom", "left"].iter().enumerate() {
            if let Some(v) = self.value_or(section, &keys(&format!("-width-{side}"))) {
                widths[i] = side_override(v);
            }
        }
        BorderSpec {
            color,
            widths,
            gradient,
        }
    }
}

fn gradient_from(colors: &[Rgba], fallback: Rgba) -> (Vec<Rgba>, f64) {
    if colors.is_empty() {
        (vec![fallback], 0.0)
    } else {
        (colors.to_vec(), 0.0)
    }
}

fn is_angle(w: &str) -> bool {
    w.strip_suffix("deg")
        .is_some_and(|n| n.parse::<f64>().is_ok())
}

fn is_ref(s: &str) -> bool {
    let ok = |p: &str| {
        !p.is_empty()
            && p.bytes()
                .all(|b| b.is_ascii_alphanumeric() || b == b'_' || b == b'-')
    };
    s.split_once('.').is_some_and(|(a, b)| ok(a) && ok(b))
}

fn side_override(v: &str) -> f64 {
    v.trim().parse::<f64>().map(|n| n.max(0.0)).unwrap_or(0.0)
}

/// `BorderGeometry.parseWidthSpec`: `N`, `Y X`, `T X B` or `T R B L`.
pub(crate) fn parse_widths(value: Option<&str>, fallback: f64) -> [f64; 4] {
    let fb = if fallback.is_finite() && fallback >= 0.0 {
        fallback
    } else {
        0.0
    };
    let Some(value) = value.filter(|v| !v.is_empty()) else {
        return [fb; 4];
    };
    let mut nums: Vec<f64> = Vec::new();
    for tok in value.split(|c: char| !(c.is_ascii_digit() || c == '.' || c == '-')) {
        if nums.len() == 4 {
            break;
        }
        if let Ok(n) = tok.parse::<f64>() {
            nums.push(if n.is_finite() && n > 0.0 { n } else { 0.0 });
        }
    }
    match nums.as_slice() {
        [] => [fb; 4],
        [a] => [*a; 4],
        [y, x] => [*y, *x, *y, *x],
        [t, x, b] => [*t, *x, *b, *x],
        [t, r, b, l, ..] => [*t, *r, *b, *l],
    }
}

fn int(raw: &str) -> Option<i64> {
    // JS parseInt: leading integer digits, ignoring what follows.
    let s = raw.trim();
    let (neg, digits) = match s.strip_prefix('-') {
        Some(rest) => (true, rest),
        None => (false, s),
    };
    let end = digits
        .find(|c: char| !c.is_ascii_digit())
        .unwrap_or(digits.len());
    let n: i64 = digits[..end].parse().ok()?;
    Some(if neg { -n } else { n })
}

fn float(raw: &str) -> Option<f64> {
    raw.trim().parse::<f64>().ok().filter(|n| n.is_finite())
}

fn bool_token(raw: Option<&str>, fallback: bool) -> bool {
    match raw.map(|s| s.trim().to_ascii_lowercase()) {
        Some(s) if ["true", "1", "yes", "on"].contains(&s.as_str()) => true,
        Some(s) if ["false", "0", "no", "off"].contains(&s.as_str()) => false,
        _ => fallback,
    }
}

fn round(n: f64) -> i64 {
    // JS Math.round: halves round toward +infinity.
    (n + 0.5).floor() as i64
}

pub fn resolve(src: &Sources) -> Tokens {
    let c = &src.colors;
    let v = &src.shell;
    let x = Ctx { c, v };

    // ---- [controls] (or the legacy [style]); [controls] wins when both set a key.
    let style = |key: &str| -> Option<&str> {
        x.get(&format!("controls.{key}"))
            .or_else(|| x.get(&format!("style.{key}")))
    };
    let style_num = |key: &str, fb: f64| style(key).and_then(float).unwrap_or(fb);
    let style_alpha = |key: &str, fb: f64| clamp_alpha(style_num(key, fb));
    let style_str = |key: &str, fb: &str| -> String {
        style(key)
            .map(str::trim)
            .filter(|s| !s.is_empty())
            .unwrap_or(fb)
            .to_string()
    };

    let normal_tok = style_str("normal-color", "foreground");
    let hover_tok = style_str("hover-cursor-color", "foreground");
    let selected_tok = style_str("selected-color", "foreground");
    let pressed_tok = style_str("pressed-color", &hover_tok);
    let focus_tok = style_str("focus-color", &hover_tok);
    let selection_tok = style_str("selection-color", "foreground");

    let state_color = |token: &str, fallback: Rgba| -> Rgba {
        let role = token.trim().to_ascii_lowercase();
        match role.as_str() {
            "foreground" | "text" => c.foreground,
            "accent" => c.accent,
            "urgent" => c.urgent,
            "background" => c.background,
            "transparent" => Rgba::TRANSPARENT,
            _ => {
                let t = token.trim();
                let hexish = t.starts_with('#') && matches!(t.len(), 4 | 7 | 9);
                if hexish {
                    parse_color(t, 1.0).unwrap_or(fallback)
                } else {
                    fallback
                }
            }
        }
    };
    let normal_c = state_color(&normal_tok, c.foreground);
    let hover_c = state_color(&hover_tok, c.foreground);
    let selected_c = state_color(&selected_tok, c.foreground);
    let pressed_c = state_color(&pressed_tok, hover_c);
    let focus_c = match focus_tok.trim().to_ascii_lowercase().as_str() {
        "hover" | "hover-cursor" | "inherit" => hover_c,
        _ => state_color(&focus_tok, hover_c),
    };
    let selection_c = state_color(&selection_tok, c.foreground);

    let normal_w = style_num("normal-border-width", 1.0).round().max(0.0);
    let hover_w = style_num("hover-cursor-border-width", normal_w)
        .round()
        .max(0.0);
    let selected_w = style_num("selected-border-width", 0.0).round().max(0.0);
    let focus_w = style_num("focus-border-width", hover_w).round().max(0.0);

    let control_border = |prefix: &str, color: Rgba, alpha: f64, fallback_w: f64| -> BorderSpec {
        let raw = style(&format!("{prefix}-border")).unwrap_or("");
        let legacy = style(&format!("{prefix}-border-gradient")).unwrap_or("");
        let (col, gradient) = x.border_value(raw, color, alpha, legacy);
        let mut widths = parse_widths(style(&format!("{prefix}-border-width")), fallback_w);
        for (i, side) in ["top", "right", "bottom", "left"].iter().enumerate() {
            if let Some(v) = style(&format!("{prefix}-border-width-{side}")) {
                widths[i] = side_override(v);
            }
        }
        BorderSpec {
            color: col,
            widths,
            gradient,
        }
    };

    let hover_border_alpha = style_alpha("hover-cursor-border-alpha", 0.25);
    let hover_fill_alpha = style_alpha("hover-cursor-fill-alpha", 0.08);
    let controls = Controls {
        normal: StateTokens {
            fill: normal_c.with_alpha(style_alpha("normal-fill-alpha", 0.04)),
            border: control_border(
                "normal",
                normal_c,
                style_alpha("normal-border-alpha", 0.4),
                normal_w,
            ),
            text: c.foreground,
        },
        hover: StateTokens {
            fill: hover_c.with_alpha(hover_fill_alpha),
            border: control_border("hover-cursor", hover_c, hover_border_alpha, hover_w),
            text: c.foreground,
        },
        focus: StateTokens {
            fill: focus_c.with_alpha(style_alpha("focus-fill-alpha", hover_fill_alpha)),
            border: control_border(
                "focus",
                focus_c,
                style_alpha("focus-border-alpha", hover_border_alpha),
                focus_w,
            ),
            text: c.foreground,
        },
        selected: StateTokens {
            fill: selected_c.with_alpha(style_alpha("selected-fill-alpha", 0.18)),
            border: control_border(
                "selected",
                selected_c,
                style_alpha("selected-border-alpha", 1.0),
                selected_w,
            ),
            text: selected_c,
        },
        pressed: FillToken {
            fill: pressed_c.with_alpha(style_alpha("pressed-fill-alpha", 0.22)),
        },
        selection: FillToken {
            fill: selection_c.with_alpha(style_alpha("selection-fill-alpha", 0.35)),
        },
    };

    // ---- [font] and [spacing]
    let base = x.get("font.base-size").and_then(int).unwrap_or(12).max(1);
    let font_scale = (base as f64 / 12.0).max(1.0 / 12.0);
    let spacing_scale = x
        .get("spacing.scale")
        .and_then(float)
        .filter(|n| *n >= 0.0)
        .unwrap_or(1.0);
    let with_font = bool_token(x.get("spacing.scale-with-font"), true);
    let eff = spacing_scale * if with_font { font_scale } else { 1.0 };
    let space = |px: f64| -> i64 {
        let n = px * eff;
        if n <= 0.0 { 0 } else { round(n).max(1) }
    };
    let sp = |key: &str, px: f64| -> i64 {
        match x.get(&format!("spacing.{key}")).and_then(float) {
            Some(n) if n >= 0.0 => round(n),
            _ => space(px),
        }
    };
    let font_px = |mult: f64| round(base as f64 * mult).max(1);
    let ft = |key: &str, fb: i64| -> i64 {
        match x.get(&format!("font.{key}")).and_then(int) {
            Some(n) if n > 0 => n,
            _ => fb,
        }
    };
    let body_small = ft("body-small", font_px(0.917));
    let title = ft("title", font_px(1.167));
    let font = Font {
        family: src.font_family.clone(),
        resolved_family: src.resolved_font_family.clone(),
        file: src.font_file.clone(),
        line_height: src
            .font_file
            .as_deref()
            .and_then(crate::fontmetrics::line_height_ratio)
            .unwrap_or(1.3),
        base_size: base,
        caption: ft("caption", font_px(0.833)),
        body_small,
        body: ft("body", font_px(1.0)),
        subtitle: ft("subtitle", font_px(1.083)),
        title,
        heading: ft("heading", font_px(1.333)),
        display: ft("display", font_px(2.0)),
        display_large: ft("display-large", font_px(2.333)),
        icon_small: ft("icon-small", body_small),
        icon: ft("icon", title),
        icon_large: ft("icon-large", font_px(1.5)),
    };
    let spacing = Spacing {
        scale: eff,
        hairline: space(1.0),
        xxs: sp("xxs", 2.0),
        xs: sp("xs", 3.0),
        sm: sp("sm", 4.0),
        md: sp("md", 6.0),
        lg: sp("lg", 8.0),
        xl: sp("xl", 10.0),
        xxl: sp("xxl", 12.0),
        xxxl: sp("xxxl", 14.0),
        huge: sp("huge", 18.0),
        control_gap: sp("control-gap", 8.0),
        control_padding_x: sp("control-padding-x", 10.0),
        control_padding_y: sp("control-padding-y", 6.0),
        input_padding_y: sp("input-padding-y", 7.0),
        control_height: sp("control-height", 28.0),
        popup_row_height: sp("popup-row-height", 28.0),
        row_gap: sp("row-gap", 8.0),
        row_padding_x: sp("row-padding-x", 12.0),
        label_gap: sp("label-gap", 4.0),
        panel_gap: sp("panel-gap", 14.0),
        panel_padding: sp("panel-padding", 18.0),
        popup_padding: sp("popup-padding", 14.0),
        dropdown_width: sp("dropdown-width", 240.0),
        searchable_dropdown_width: sp("searchable-dropdown-width", 260.0),
        number_field_width: sp("number-field-width", 120.0),
        searchable_popup_min_height: sp("searchable-popup-min-height", 220.0),
        meter_thickness: sp("meter-thickness", 6.0),
        scrollbar_thickness: sp("scrollbar-thickness", 6.0),
    };

    // ---- surfaces
    let card_w = space(2.0).max(1) as f64;
    let line_w = normal_w.max(1.0);
    let surface = |name: &str, bg: Rgba, bg_alpha: f64, border_fb: Rgba, width: f64| -> Surface {
        let border_color = x.composed(
            &format!("{name}.border"),
            &format!("{name}.border-alpha"),
            border_fb,
            1.0,
        );
        Surface {
            background: x.composed(
                &format!("{name}.background"),
                &format!("{name}.background-alpha"),
                bg,
                bg_alpha,
            ),
            text: x.text(&format!("{name}.text"), c.foreground),
            border: x.surface_border(name, "border", border_color, width, None),
        }
    };
    let menu_selected_border = x.composed(
        "menu.selected-border",
        "menu.selected-border-alpha",
        c.foreground,
        0.0,
    );
    let polkit_border_error =
        x.composed("polkit.border-error", "polkit.border-alpha", c.urgent, 1.0);
    let surfaces = Surfaces {
        popups: surface("popups", c.background, 1.0, c.accent, card_w),
        tooltip: surface("tooltip", c.background, 1.0, c.foreground, line_w),
        menu: MenuSurface {
            base: surface("menu", c.background, 1.0, c.foreground, card_w),
            scrim: x.composed("menu.scrim", "menu.scrim-alpha", c.background, 0.5),
            selected_background: x.composed(
                "menu.selected-background",
                "menu.selected-background-alpha",
                c.foreground,
                0.08,
            ),
            selected_text: x.text("menu.selected-text", c.accent),
            selected_border: x.surface_border(
                "menu",
                "selected-border",
                menu_selected_border,
                0.0,
                None,
            ),
        },
        notifications: NotificationSurface {
            base: surface("notifications", c.background, 1.0, c.accent, card_w),
            countdown: x.text("notifications.countdown", c.accent),
        },
        polkit: PolkitSurface {
            base: Surface {
                background: x.composed(
                    "polkit.background",
                    "polkit.background-alpha",
                    c.background,
                    1.0,
                ),
                text: x.text("polkit.text", c.foreground),
                border: x.surface_border(
                    "polkit",
                    "border",
                    x.composed("polkit.border", "polkit.border-alpha", c.accent, 1.0),
                    card_w,
                    Some("border-alpha"),
                ),
            },
            text_error: x.text("polkit.text-error", c.urgent),
            border_error: x.surface_border(
                "polkit",
                "border-error",
                polkit_border_error,
                card_w,
                Some("border-alpha"),
            ),
            accent: x.text("polkit.accent", c.accent),
            scrim: x.composed("polkit.scrim", "polkit.scrim-alpha", c.background, 0.5),
        },
    };

    let mut named = c.named.clone();
    for (k, val) in [
        ("foreground", c.foreground),
        ("background", c.background),
        ("accent", c.accent),
        ("muted", c.muted),
    ] {
        named.entry(k.to_string()).or_insert(val);
    }

    Tokens {
        schema: 1,
        theme: src.theme_name.clone(),
        icon_theme: src.icon_theme.clone(),
        language: crate::strings::language_code(&src.language),
        mode: if c.dark { Mode::Dark } else { Mode::Light },
        palette: Palette {
            foreground: c.foreground,
            background: c.background,
            accent: c.accent,
            urgent: c.urgent,
            muted: c.muted,
            success: c.success,
            warning: c.warning,
            named,
        },
        roles: Roles {
            // The theme's own Base24 stops when it has them (dark_foreground is base04, muted
            // base03). Otherwise derived toward the background: Qt's darker in a dark theme (the
            // shell's own factors), a mix in a light one, where darker would mean stronger.
            text_dim: c.dark_foreground.unwrap_or_else(|| {
                if c.dark {
                    c.foreground.darker(1.5)
                } else {
                    c.foreground.mix(c.background, 1.0 / 3.0)
                }
            }),
            text_faint: c.muted_named.unwrap_or_else(|| {
                if c.dark {
                    c.foreground.darker(2.0)
                } else {
                    c.foreground.mix(c.background, 0.5)
                }
            }),
            track: c.foreground.with_alpha(0.12),
            scrim: surfaces.menu.scrim,
        },
        radius: src.hypr.rounding.max(0.0).round() as i64,
        gaps_out: round(src.hypr.gaps_out.max(0.0) / 2.0).max(0),
        window_border: src.hypr.border_size.max(0.0).round() as i64,
        reduce_motion: !src.hypr.animations,
        font,
        spacing,
        controls,
        surfaces,
        motion: Motion::default(),
    }
}
