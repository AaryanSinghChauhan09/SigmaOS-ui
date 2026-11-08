//! The resolved token set. Field names serialize to the canonical JSON in `spec/tokens.md`.

use std::collections::BTreeMap;
use std::path::PathBuf;

use serde::Serialize;

use crate::color::Rgba;

#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum Mode {
    Dark,
    Light,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct Gradient {
    pub colors: Vec<Rgba>,
    /// Degrees, Hyprland's convention.
    pub angle: f64,
}

/// A border: a color (the first stop when it is a gradient), per-side widths, and an optional gradient.
#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct BorderSpec {
    pub color: Rgba,
    /// Top, right, bottom, left.
    pub widths: [f64; 4],
    pub gradient: Option<Gradient>,
}

impl BorderSpec {
    pub fn none() -> Self {
        Self { color: Rgba::TRANSPARENT, widths: [0.0; 4], gradient: None }
    }
    pub fn max_width(&self) -> f64 {
        self.widths.iter().copied().fold(0.0, f64::max)
    }
    pub fn is_none(&self) -> bool {
        self.max_width() <= 0.0
    }
    pub fn is_uniform(&self) -> bool {
        self.widths.iter().all(|w| *w == self.widths[0])
    }
    /// The same border at a uniform `width`.
    pub fn with_width(&self, width: f64) -> Self {
        Self { widths: [width; 4], ..self.clone() }
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Palette {
    pub foreground: Rgba,
    pub background: Rgba,
    pub accent: Rgba,
    pub urgent: Rgba,
    pub muted: Rgba,
    /// Every color in `colors.toml` by its key (`red`, `bright_blue`, `lighter_background`, …).
    #[serde(skip)]
    pub named: BTreeMap<String, Rgba>,
}

impl Palette {
    /// A named color from `colors.toml`, else `fallback`.
    pub fn named_or(&self, key: &str, fallback: Rgba) -> Rgba {
        self.named.get(key).copied().unwrap_or(fallback)
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Font {
    /// The family name to ask for: the fontconfig alias, so `omarchy font set` keeps working.
    pub family: String,
    /// What the alias resolves to now, for display.
    pub resolved_family: String,
    /// The resolved family's regular face, for frameworks that load font files themselves.
    #[serde(skip)]
    pub file: Option<PathBuf>,
    pub base_size: i64,
    pub caption: i64,
    pub body_small: i64,
    pub body: i64,
    pub subtitle: i64,
    pub title: i64,
    pub heading: i64,
    pub display: i64,
    pub display_large: i64,
    pub icon_small: i64,
    pub icon: i64,
    pub icon_large: i64,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Spacing {
    pub scale: f64,
    pub hairline: i64,
    pub xxs: i64,
    pub xs: i64,
    pub sm: i64,
    pub md: i64,
    pub lg: i64,
    pub xl: i64,
    pub xxl: i64,
    pub xxxl: i64,
    pub huge: i64,
    pub control_gap: i64,
    pub control_padding_x: i64,
    pub control_padding_y: i64,
    pub input_padding_y: i64,
    pub control_height: i64,
    pub popup_row_height: i64,
    pub row_gap: i64,
    pub row_padding_x: i64,
    pub label_gap: i64,
    pub panel_gap: i64,
    pub panel_padding: i64,
    pub popup_padding: i64,
}

impl Spacing {
    /// `Style.space(px)`: a pixel value at the default scale, scaled like every other token.
    pub fn space(&self, px: f64) -> f64 {
        let n = px * self.scale;
        if n <= 0.0 { 0.0 } else { n.round().max(1.0) }
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct StateTokens {
    pub fill: Rgba,
    pub border: BorderSpec,
    /// Label ink in this state.
    pub text: Rgba,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct FillToken {
    pub fill: Rgba,
}

/// Interactive-state chrome, priority pressed > focus > hover > selected > normal.
#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct Controls {
    pub normal: StateTokens,
    pub hover: StateTokens,
    pub focus: StateTokens,
    pub selected: StateTokens,
    pub pressed: FillToken,
    pub selection: FillToken,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct Surface {
    pub background: Rgba,
    pub text: Rgba,
    pub border: BorderSpec,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct MenuSurface {
    #[serde(flatten)]
    pub base: Surface,
    pub scrim: Rgba,
    pub selected_background: Rgba,
    pub selected_text: Rgba,
    pub selected_border: BorderSpec,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct NotificationSurface {
    #[serde(flatten)]
    pub base: Surface,
    pub countdown: Rgba,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PolkitSurface {
    #[serde(flatten)]
    pub base: Surface,
    pub text_error: Rgba,
    pub border_error: BorderSpec,
    pub accent: Rgba,
    pub scrim: Rgba,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
pub struct Surfaces {
    /// Dropdown lists, flyouts, cards.
    pub popups: Surface,
    pub tooltip: Surface,
    /// Lists with a selected row, launcher-style cards.
    pub menu: MenuSurface,
    pub notifications: NotificationSurface,
    /// Modal prompts.
    pub polkit: PolkitSurface,
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Motion {
    /// Color and state changes, in ms.
    pub fast: u32,
    /// The press give (scale 0.97), in ms.
    pub press: u32,
    /// `cubic-bezier(x1, y1, x2, y2)`.
    pub ease_out: [f64; 4],
}

impl Default for Motion {
    fn default() -> Self {
        Self { fast: 120, press: 160, ease_out: [0.23, 1.0, 0.32, 1.0] }
    }
}

#[derive(Clone, Debug, PartialEq, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Tokens {
    pub schema: u32,
    #[serde(skip)]
    pub theme: String,
    pub mode: Mode,
    pub palette: Palette,
    /// Hyprland's `decoration:rounding`; 0 is sharp.
    pub radius: i64,
    /// Half of Hyprland's `general:gaps_out`.
    pub gaps_out: i64,
    /// Hyprland's `general:border_size`.
    #[serde(skip)]
    pub window_border: i64,
    pub reduce_motion: bool,
    pub font: Font,
    pub spacing: Spacing,
    pub controls: Controls,
    pub surfaces: Surfaces,
    pub motion: Motion,
}

impl Tokens {
    pub fn is_dark(&self) -> bool {
        self.mode == Mode::Dark
    }

    /// The radius for a small element (a checkbox, a chip): the window rounding, capped at 4.
    pub fn radius_small(&self) -> f64 {
        (self.radius as f64).min(4.0)
    }

    /// A duration in ms, or 0 under reduced motion.
    pub fn duration(&self, ms: u32) -> u32 {
        if self.reduce_motion { 0 } else { ms }
    }

    /// The canonical JSON dump (`spec/tokens.md`), with whole numbers printed as integers so
    /// every binding's dump compares byte for byte.
    pub fn to_canonical_json(&self) -> String {
        let mut v = serde_json::to_value(self).expect("tokens serialize");
        normalize(&mut v);
        serde_json::to_string_pretty(&v).expect("json")
    }
}

fn normalize(v: &mut serde_json::Value) {
    use serde_json::Value;
    match v {
        Value::Number(n) => {
            if let Some(f) = n.as_f64()
                && f.fract() == 0.0
                && f.abs() < 1e15
            {
                *v = Value::from(f as i64);
            } else if let Some(f) = n.as_f64() {
                // Four decimals is plenty and keeps float noise out of diffs.
                *v = serde_json::json!((f * 10000.0).round() / 10000.0);
            }
        }
        Value::Array(a) => a.iter_mut().for_each(normalize),
        Value::Object(o) => o.values_mut().for_each(normalize),
        _ => {}
    }
}
