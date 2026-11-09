//! The theme-switch color fade (`spec/following.md`): every color eases from what is on screen to
//! the new tokens over `motion.theme` on `motion.ease`, interpolated per channel in sRGB, alpha
//! included. Everything that isn't a color takes the new value at once.

use omarchy_theme::{BorderSpec, Gradient, Rgba, Surface, Tokens};

/// `cubic-bezier(x1, y1, x2, y2)` at time `x` (0–1): the curve's y where its x is `x`, solved by
/// Newton steps with a bisection fallback, as browsers do.
pub fn cubic_bezier(c: [f64; 4], x: f64) -> f64 {
    let x = x.clamp(0.0, 1.0);
    if x == 0.0 || x == 1.0 {
        return x;
    }
    let [x1, y1, x2, y2] = c;
    let coord = |t: f64, a: f64, b: f64| {
        let u = 1.0 - t;
        3.0 * u * u * t * a + 3.0 * u * t * t * b + t * t * t
    };
    let slope = |t: f64, a: f64, b: f64| {
        let u = 1.0 - t;
        3.0 * u * u * a + 6.0 * u * t * (b - a) + 3.0 * t * t * (1.0 - b)
    };
    let mut t = x;
    for _ in 0..8 {
        let err = coord(t, x1, x2) - x;
        if err.abs() < 1e-7 {
            return coord(t, y1, y2);
        }
        let d = slope(t, x1, x2);
        if d.abs() < 1e-6 {
            break;
        }
        t -= err / d;
    }
    let (mut lo, mut hi) = (0.0, 1.0);
    t = x;
    for _ in 0..40 {
        let v = coord(t, x1, x2);
        if (v - x).abs() < 1e-7 {
            break;
        }
        if v < x {
            lo = t;
        } else {
            hi = t;
        }
        t = (lo + hi) / 2.0;
    }
    coord(t, y1, y2)
}

fn mix(a: Rgba, b: Rgba, p: f64) -> Rgba {
    a.mix(b, p)
}

fn mix_gradient(a: &Option<Gradient>, b: &Option<Gradient>, p: f64) -> Option<Gradient> {
    match (a, b) {
        (Some(a), Some(b)) if a.colors.len() == b.colors.len() => Some(Gradient {
            colors: a.colors.iter().zip(&b.colors).map(|(x, y)| mix(*x, *y, p)).collect(),
            angle: b.angle,
        }),
        _ => b.clone(),
    }
}

fn mix_border(a: &BorderSpec, b: &BorderSpec, p: f64) -> BorderSpec {
    BorderSpec { color: mix(a.color, b.color, p), widths: b.widths, gradient: mix_gradient(&a.gradient, &b.gradient, p) }
}

fn mix_surface(a: &Surface, b: &Surface, p: f64) -> Surface {
    Surface { background: mix(a.background, b.background, p), text: mix(a.text, b.text, p), border: mix_border(&a.border, &b.border, p) }
}

/// `to` with every color `p` of the way from `from`'s (0 is `from`'s colors, 1 is exactly `to`).
pub fn mix_tokens(from: &Tokens, to: &Tokens, p: f64) -> Tokens {
    if p >= 1.0 {
        return to.clone();
    }
    let mut t = to.clone();
    let (a, b) = (&from.palette, &mut t.palette);
    b.foreground = mix(a.foreground, b.foreground, p);
    b.background = mix(a.background, b.background, p);
    b.accent = mix(a.accent, b.accent, p);
    b.urgent = mix(a.urgent, b.urgent, p);
    b.muted = mix(a.muted, b.muted, p);
    b.success = mix(a.success, b.success, p);
    b.warning = mix(a.warning, b.warning, p);
    for (k, v) in b.named.iter_mut() {
        if let Some(old) = a.named.get(k) {
            *v = mix(*old, *v, p);
        }
    }
    let (a, b) = (&from.roles, &mut t.roles);
    b.text_dim = mix(a.text_dim, b.text_dim, p);
    b.text_faint = mix(a.text_faint, b.text_faint, p);
    b.track = mix(a.track, b.track, p);
    b.scrim = mix(a.scrim, b.scrim, p);

    let (a, b) = (&from.controls, &mut t.controls);
    for (sa, sb) in [(&a.normal, &mut b.normal), (&a.hover, &mut b.hover), (&a.focus, &mut b.focus), (&a.selected, &mut b.selected)] {
        sb.fill = mix(sa.fill, sb.fill, p);
        sb.text = mix(sa.text, sb.text, p);
        sb.border = mix_border(&sa.border, &sb.border, p);
    }
    b.pressed.fill = mix(a.pressed.fill, b.pressed.fill, p);
    b.selection.fill = mix(a.selection.fill, b.selection.fill, p);

    let (a, b) = (&from.surfaces, &mut t.surfaces);
    b.popups = mix_surface(&a.popups, &b.popups, p);
    b.tooltip = mix_surface(&a.tooltip, &b.tooltip, p);
    b.menu.base = mix_surface(&a.menu.base, &b.menu.base, p);
    b.menu.scrim = mix(a.menu.scrim, b.menu.scrim, p);
    b.menu.selected_background = mix(a.menu.selected_background, b.menu.selected_background, p);
    b.menu.selected_text = mix(a.menu.selected_text, b.menu.selected_text, p);
    b.menu.selected_border = mix_border(&a.menu.selected_border, &b.menu.selected_border, p);
    b.notifications.base = mix_surface(&a.notifications.base, &b.notifications.base, p);
    b.notifications.countdown = mix(a.notifications.countdown, b.notifications.countdown, p);
    b.polkit.base = mix_surface(&a.polkit.base, &b.polkit.base, p);
    b.polkit.text_error = mix(a.polkit.text_error, b.polkit.text_error, p);
    b.polkit.border_error = mix_border(&a.polkit.border_error, &b.polkit.border_error, p);
    b.polkit.accent = mix(a.polkit.accent, b.polkit.accent, p);
    b.polkit.scrim = mix(a.polkit.scrim, b.polkit.scrim, p);
    t
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn ease_matches_css() {
        let ease = [0.25, 0.1, 0.25, 1.0];
        assert_eq!(cubic_bezier(ease, 0.0), 0.0);
        assert_eq!(cubic_bezier(ease, 1.0), 1.0);
        // CSS `ease` at the halfway time is ~0.8024.
        assert!((cubic_bezier(ease, 0.5) - 0.8024).abs() < 1e-3);
        let linear = [0.0, 0.0, 1.0, 1.0];
        assert!((cubic_bezier(linear, 0.3) - 0.3).abs() < 1e-4);
    }
}
