// The canonical token JSON (spec/tokens.md), as the omarchy-theme CLI resolves it, mapped onto CSS
// custom properties. Nothing here resolves a theme: every value comes from the core as it is, and
// this file only names it for CSS (`#rrggbbaa` is CSS as it is, numbers get their unit).
//
//   palette.accent                 --omarchy-accent
//   roles.textDim                  --omarchy-text-dim
//   controls.hover.fill            --omarchy-control-hover-fill (+ -fill-alpha, a number)
//   controls.normal.border         --omarchy-control-normal-border (color), -border-width
//                                  ("1px 1px 1px 1px"), -border-top|right|bottom|left, -border-image
//                                  (a gradient or none), -border-solid (transparent under a gradient)
//   surfaces.menu.selectedText     --omarchy-menu-selected-text
//   radius                         --omarchy-radius, --omarchy-radius-small (min(4, radius))
//   spacing.controlPaddingX        --omarchy-space-control-padding-x; spacing.scale --omarchy-space-scale
//   font.caption                   --omarchy-font-caption; font.resolvedFamily --omarchy-font-family
//   motion.theme                   --omarchy-motion-theme (0ms under reduceMotion), motion.ease --omarchy-ease

const kebab = (s) => s.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase());

const isBorder = (v) => v && typeof v === "object" && Array.isArray(v.widths) && "color" in v;

/** A border spec's gradient as CSS (`linear-gradient(...)`), or `none`. The angle is drawn as the Qt
 *  kit draws it (BorderGeometry.js gradientEndpoints): 0deg runs left to right, 90deg top to bottom,
 *  which is CSS's angle plus 90. */
export function borderGradient(spec) {
  const g = spec && spec.gradient;
  if (!g || !Array.isArray(g.colors) || g.colors.length < 2) return "none";
  return `linear-gradient(${90 + (Number(g.angle) || 0)}deg, ${g.colors.join(", ")})`;
}

function border(out, name, spec, colors) {
  out[name] = spec.color;
  colors.add(name);
  const [t, r, b, l] = spec.widths;
  out[`${name}-width`] = `${t}px ${r}px ${b}px ${l}px`;
  out[`${name}-top`] = `${t}px`;
  out[`${name}-right`] = `${r}px`;
  out[`${name}-bottom`] = `${b}px`;
  out[`${name}-left`] = `${l}px`;
  out[`${name}-image`] = borderGradient(spec);
  // The color to draw as a plain border: the color, or transparent when a gradient ring draws it.
  out[`${name}-solid`] = out[`${name}-image`] === "none" ? spec.color : "#00000000";
  colors.add(`${name}-solid`);
}

const alphaOf = (hex) => (typeof hex === "string" && hex.length === 9 ? parseInt(hex.slice(7), 16) / 255 : 1);

function fontStack(family) {
  const f = String(family || "").trim();
  if (!f || f === "monospace") return "monospace";
  return `"${f.replace(/["\\]/g, "")}", monospace`;
}

/**
 * Every custom property for one token set, as `{ "--omarchy-…": "value" }`, plus the names that hold
 * colors (`colors`), which are the ones that fade on a theme switch.
 */
export function tokensToCss(tokens) {
  const out = {};
  const colors = new Set();
  const p = (k) => `--omarchy-${k}`;
  const color = (k, v) => {
    out[p(k)] = v;
    colors.add(p(k));
  };

  for (const [k, v] of Object.entries(tokens.palette || {})) color(kebab(k), v);
  for (const [k, v] of Object.entries(tokens.roles || {})) color(kebab(k), v);

  for (const [state, s] of Object.entries(tokens.controls || {})) {
    for (const [k, v] of Object.entries(s)) {
      const name = `control-${kebab(state)}-${kebab(k)}`;
      if (isBorder(v)) {
        border(out, p(name), v, colors);
        out[p(`${name}-alpha`)] = String(+alphaOf(v.color).toFixed(4));
      } else {
        color(name, v);
        if (k === "fill") out[p(`${name}-alpha`)] = String(+alphaOf(v).toFixed(4));
      }
    }
  }

  for (const [surface, s] of Object.entries(tokens.surfaces || {})) {
    for (const [k, v] of Object.entries(s)) {
      const name = `${kebab(surface)}-${kebab(k)}`;
      if (isBorder(v)) border(out, p(name), v, colors);
      else color(name, v);
    }
  }

  const radius = Number(tokens.radius) || 0;
  out[p("radius")] = `${radius}px`;
  out[p("radius-small")] = `${Math.min(4, radius)}px`;
  out[p("gaps-out")] = `${Number(tokens.gapsOut) || 0}px`;
  out[p("mode")] = tokens.mode === "light" ? "light" : "dark";

  const font = tokens.font || {};
  out[p("font-family")] = fontStack(font.resolvedFamily || font.family);
  for (const [k, v] of Object.entries(font)) {
    if (typeof v === "number") out[p(`font-${kebab(k)}`)] = `${v}px`;
  }

  for (const [k, v] of Object.entries(tokens.spacing || {})) {
    out[p(`space-${kebab(k)}`)] = k === "scale" ? String(v) : `${v}px`;
  }

  const m = tokens.motion || {};
  const still = !!tokens.reduceMotion;
  for (const k of ["fast", "press", "theme"]) out[p(`motion-${k}`)] = `${still ? 0 : Number(m[k]) || 0}ms`;
  if (Array.isArray(m.ease)) out[p("ease")] = `cubic-bezier(${m.ease.join(", ")})`;
  if (Array.isArray(m.easeOut)) out[p("ease-out")] = `cubic-bezier(${m.easeOut.join(", ")})`;

  return { props: out, colors: [...colors] };
}

/** The same as a CSS rule, for a static snapshot (`:root { --omarchy-…: …; }`). */
export function tokensToCssText(tokens, selector = ":root") {
  const { props } = tokensToCss(tokens);
  const lines = Object.entries(props).map(([k, v]) => `  ${k}: ${v};`);
  const scheme = tokens.mode === "light" ? "light" : "dark";
  return `${selector} {\n  color-scheme: ${scheme};\n${lines.join("\n")}\n}\n`;
}
