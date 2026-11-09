# Omarchy UI tokens

This is the contract every framework binding implements. A binding reads the same sources, resolves them by the same rules, and can print the result as the canonical JSON below (`--dump-tokens`). `bin/review` diffs those dumps across bindings for every fixture theme, so drift shows up as a failing parity check rather than as an app that looks slightly off.

The rules are lifted from the Omarchy shell (`shell/Commons/Color.qml`, `Style.qml`, `Border.qml`, `BorderGeometry.js` on `quattro`), which is the reference implementation. When the shell changes, this file and both bindings change with it.

## Sources

| Source | Default location | Provides |
|---|---|---|
| Theme palette | `~/.local/state/omarchy/current/theme/colors.toml` | `foreground`, `background`, `accent`, `muted`, `red` (urgent), `green` (success), `yellow` (warning), every named color, `mode` |
| Theme shell tokens | `~/.local/state/omarchy/current/theme/shell.toml` | `[controls]` state tokens, `[spacing]`, `[font]`, per-surface roles (`[popups]`, `[tooltip]`, `[menu]`, …), `[hyprland]` border refs |
| User shell overrides | `~/.config/omarchy/shell.toml` | Same keys as above; user keys win over the theme's (this is where `omarchy display text size` writes `[font] base-size`) |
| Hyprland | `j/getoption` on the instance socket | `decoration:rounding` → `radius`, `general:gaps_out` → `gapsOut` (halved), `animations:enabled` → `reduceMotion`, `general:border_size` → `windowBorder` |
| Fontconfig | `fc-match monospace` | The concrete family (and file) the `monospace` alias resolves to |
| Icon theme | `~/.local/state/omarchy/current/theme/icons.theme` | The freedesktop icon theme Avatar's icons by name come from: its first non-empty line, else `Yaru-blue` (as `omarchy-theme-set-gnome` applies it). Rust `Tokens::icon_theme`, Qt `ThemeSource.iconTheme`; not in the canonical JSON. Lookup rules: `components/lists.md`, Avatar |

A theme switch replaces the whole `current/theme` directory and rewrites `current/theme.name`, so bindings watch the `current/` directory, not files inside the theme. They also watch the theme directory itself, re-armed after every change (a swap leaves a watch on the old directory), so an edit inside it, such as to `icons.theme`, is followed too (Qt watches `icons.theme` as a file as well: `QFileSystemWatcher`'s directory watch misses a file rewritten in place). Hyprland announces `configreloaded` on `.socket2.sock` once new rounding and gaps are in effect. When `HYPRLAND_INSTANCE_SIGNATURE` is unset, bindings use the most recently modified instance under `$XDG_RUNTIME_DIR/hypr/`.

How a binding follows these while an app runs (coalescing, the public changed hook, the theme-switch color fade, the live-switch test every binding has) is [`following.md`](following.md).

### Overrides for review and tests

Every binding honors these, so a gallery can render any fixture without touching the live desktop:

| Variable | Effect |
|---|---|
| `OMARCHY_UI_THEME_DIR` | Read `colors.toml` / `shell.toml` from this directory instead. The user's `~/.config/omarchy/shell.toml` is then ignored, so renders are deterministic. |
| `OMARCHY_UI_ROUNDING` | Use this instead of `decoration:rounding`. |
| `OMARCHY_UI_GAPS_OUT` | Use this instead of `general:gaps_out` (raw Hyprland value; still halved). |
| `OMARCHY_UI_REDUCE_MOTION` | `1` forces reduced motion, `0` forces motion on. |
| `OMARCHY_UI_FONT_FAMILY` | Use this family instead of resolving `monospace`. |
| `OMARCHY_UI_ICON_THEME` | Use this icon theme instead of the one `icons.theme` names. |
| `OMARCHY_UI_ICON_DIRS` | Search these roots for icons (colon-separated) instead of the XDG icon directories, `~/.icons` and `/usr/share/pixmaps`. `bin/review` points both at `fixtures/icons/`, so icons by name render the same on every machine. |

## Palette

`colors.toml` lines matching `key = "#rrggbb"` populate the palette.

- `foreground`, `background`, `accent`, `muted` come from the keys of the same name.
- Missing `background` falls back to `color0`, missing `foreground` to `color7`, missing `accent` to `color4`, missing `muted` to `color8`, then to `foreground`.
- `urgent` is `red`, else `color1`. There is no `urgent` key.
- `success` is `green`, else `color2`; `warning` is `yellow`, else `color3`. (The kit's own; the shell has neither.)
- `mode` is `"light"` or `"dark"` (default dark).
- Defaults when nothing is found: foreground `#cacccc`, background `#101315`, accent `#cacccc`, urgent `#a55555`, muted `#707880`, success `#7f9f6f`, warning `#c0a060`.

### Roles

Colors derived from the palette, so components share them instead of each re-deriving its own (the shell writes `Qt.darker(foreground, 1.5)` by hand 48 times).

| Role | Value | For |
|---|---|---|
| `textDim` | `dark_foreground` (legacy `dark_fg`; Base24 base04). Without one: foreground darker 1.5 in a dark theme, mixed ⅓ toward the background in a light one | Secondary text: subtitles, values, hints, status lines |
| `textFaint` | `muted`, else `color8` (Base24 base03). Without either: foreground darker 2.0 in a dark theme, mixed ½ toward the background in a light one | Disabled and placeholder text |
| `track` | foreground at 0.12 | The unfilled part of meters, progress bars and scrollbars; also the separator color |
| `scrim` | `surfaces.menu.scrim` | Behind modal dialogs |

`textDim` and `textFaint` follow the theme's own neutral ramp (`docs/theming.md` in Omarchy: `muted` is for "comments, placeholders, dividers"), so they read the same as the theme's terminal and editor. The fallback, for themes predating those keys, fades toward the background: in a dark theme exactly the shell's `Qt.darker`; in a light theme `darker` would make secondary text stronger than the foreground, so it mixes instead (sRGB, per 8-bit channel, rounded).

`darker(f)` is Qt's `QColor::darker(round(f × 100))`, exactly: HSV value divided in 16-bit channels with float math, so halves round as Qt rounds them.

## Colors in shell.toml

A color token is one of: a palette role (`foreground` or `text`, `accent`, `urgent`, `muted`, `background`, `transparent`), a reference to another key (`hyprland.active-border`), or a literal in any Hyprland-compatible form: `#rgb`, `#rrggbb`, `#rrggbbaa`, `rgb(rrggbb)`, `rgba(rrggbbaa)`, `rgb(r,g,b)`, `rgba(r,g,b,a)`, `0xaarrggbb`. A border token may be a gradient: several colors and an optional `<n>deg` angle (`"rgba(33ccffee) rgba(00ff99ee) 45deg"`). Color-only consumers use the first stop.

Alpha companions (`background-alpha`, `border-alpha`, …) multiply the color's own alpha and clamp to 0–1.

## Radius, gaps, borders

- `radius` is Hyprland's `decoration:rounding` (Omarchy's default is 0: sharp). Every panel, card, dialog, control and popup uses it. Small elements cap it: `min(4, radius)`. Only true circles and pill switches keep their own radius, and switches are pills only when `radius > 0`.
- `gapsOut` is half of Hyprland's `general:gaps_out`: the distance from a floating surface to the screen edge.
- A border spec is `{ color, widths: [top, right, bottom, left], gradient: { colors, angle } | null }`. Width strings are CSS-style: `N`, `"Y X"`, `"T X B"`, `"T R B L"`; `*-border-width-top|right|bottom|left` keys override one side.

## Interactive states (`[controls]`)

Five states, applied in this priority: **pressed > focus > hover-cursor > selected > normal**.

| State | Meaning | Fill | Border |
|---|---|---|---|
| `normal` | idle chrome | `normal-color` × `normal-fill-alpha` (0.04) | `normal-border` × `normal-border-alpha` (0.4), width 1 |
| `hover` | mouse hover **or** keyboard cursor | `hover-cursor-color` × 0.08 | × 0.25, width 1 |
| `focus` | real keyboard focus | mirrors hover by default | mirrors hover |
| `selected` | persistent chosen state | `selected-color` × 0.18 | × 1.0, width 0 |
| `pressed` | mouse down | `pressed-color` (→ hover) × 0.22 | — |
| `selection` | text selection | `selection-color` × 0.35 | — |

State colors default to `foreground`, so Omarchy controls are foreground-tinted glass over the background, not accent-filled. Accent marks the *selected* text and the *on* state only where the theme says so.

Plain buttons are borderless at rest and show the hover border under the pointer or cursor. `bordered` buttons, inputs and dropdowns show the normal border at rest. Components reserve the widest border any state can paint, so hovering never relayouts.

## Surfaces

Each surface has `background`, `text`, `border` (spec), and some extras. Defaults when `shell.toml` omits them:

| Surface | background | text | border | extras |
|---|---|---|---|---|
| `popups` (dropdowns, flyouts, cards) | background | foreground | accent (`hyprland.active-border`) | |
| `tooltip` | background × 0.97 | foreground | foreground | |
| `menu` (lists, launcher-style cards) | background | foreground | foreground | `scrim` background × 0.5, `selectedBackground` fg × 0.08, `selectedText` accent, `selectedBorder` |
| `notifications` | background | foreground | accent | `countdown` accent |
| `polkit` (modal prompts) | background | foreground | accent | `textError`, `borderError` urgent, `scrim`, `accent` |

## Spacing

`effectiveScale = [spacing] scale × (scale-with-font ? baseSize / 12 : 1)`. `space(px) = max(1, round(px × effectiveScale))`. A `[spacing]` key pins one token in px.

| Token | px at scale 1 | Token | px at scale 1 |
|---|---|---|---|
| `hairline` | 1 | `controlGap` | 8 |
| `xxs` | 2 | `controlPaddingX` | 10 |
| `xs` | 3 | `controlPaddingY` | 6 |
| `sm` | 4 | `inputPaddingY` | 7 |
| `md` | 6 | `controlHeight` | 28 |
| `lg` | 8 | `popupRowHeight` | 28 |
| `xl` | 10 | `rowGap` | 8 |
| `xxl` | 12 | `rowPaddingX` | 12 |
| `xxxl` | 14 | `labelGap` | 4 |
| `huge` | 18 | `panelGap` | 14 |
| | | `panelPadding` | 18 |
| | | `popupPadding` | 14 |
| | | `dropdownWidth` | 240 |
| | | `searchableDropdownWidth` | 260 |
| | | `numberFieldWidth` | 120 |
| | | `searchablePopupMinHeight` | 220 |
| | | `meterThickness` | 6 |
| | | `scrollbarThickness` | 6 |

## Typography

One family everywhere: the fontconfig `monospace` alias, which `omarchy font set` points at the user's chosen font. `baseSize` (`[font] base-size`, default 12) is the rem root; each token is `max(1, round(baseSize × mult))` unless `[font] <token>` pins it.

| Token | mult | px at 12 |
|---|---|---|
| `caption` | 0.833 | 10 |
| `bodySmall` | 0.917 | 11 |
| `body` | 1.0 | 12 |
| `subtitle` | 1.083 | 13 |
| `title` | 1.167 | 14 |
| `heading` | 1.333 | 16 |
| `display` | 2.0 | 24 |
| `displayLarge` | 2.333 | 28 |
| `iconSmall` | = bodySmall | 11 |
| `icon` | = title | 14 |
| `iconLarge` | 1.5 | 18 |

Selected labels are bold. Nothing else changes weight.

## Motion

Short and quiet. `fast` 120 ms for color and state changes, `press` 160 ms for the press give (scale 0.97) on the `easeOut` curve `cubic-bezier(0.23, 1, 0.32, 1)`. With `reduceMotion`, every duration is 0.

## Canonical JSON

`--dump-tokens` prints this shape. Colors are lowercase `#rrggbbaa`, alpha rounded as `round(a × 255)`. Numbers are integers except `spacing.scale`.

```json
{
  "schema": 1,
  "mode": "dark",
  "palette": { "foreground": "#a9b1d6ff", "background": "#1a1b26ff", "accent": "#7aa2f7ff", "urgent": "#f7768eff", "muted": "#414868ff", "success": "#9ece6aff", "warning": "#e0af68ff" },
  "roles": { "textDim": "#565f89ff", "textFaint": "#414868ff", "track": "#a9b1d61f", "scrim": "#1a1b2680" },
  "radius": 0,
  "gapsOut": 5,
  "reduceMotion": false,
  "font": { "family": "monospace", "baseSize": 12, "caption": 10, "bodySmall": 11, "body": 12, "subtitle": 13, "title": 14, "heading": 16, "display": 24, "displayLarge": 28, "iconSmall": 11, "icon": 14, "iconLarge": 18 },
  "spacing": { "scale": 1.0, "hairline": 1, "xxs": 2, "...": 0 },
  "controls": {
    "normal":   { "fill": "#a9b1d60a", "border": { "color": "#a9b1d666", "widths": [1, 1, 1, 1], "gradient": null } },
    "hover":    { "fill": "...", "border": { "...": 0 } },
    "focus":    { "fill": "...", "border": { "...": 0 } },
    "selected": { "fill": "...", "border": { "...": 0 }, "text": "#a9b1d6ff" },
    "pressed":  { "fill": "..." },
    "selection":{ "fill": "..." }
  },
  "surfaces": {
    "popups":  { "background": "...", "text": "...", "border": { "...": 0 } },
    "tooltip": { "...": 0 },
    "menu":    { "...": 0, "scrim": "...", "selectedBackground": "...", "selectedText": "...", "selectedBorder": { "...": 0 } },
    "notifications": { "...": 0 },
    "polkit":  { "...": 0 }
  },
  "motion": { "fast": 120, "press": 160, "easeOut": [0.23, 1.0, 0.32, 1.0] }
}
```
