# Surfaces

The in-window surface and the title style that go with Card (`spec/components.md`): Pane, for an app's own panes, and Heading, for the titles of panes, toolbars, sections and pages. Both came out of the `composed` section ([`composed.md`](composed.md)), whose inspector had to borrow Card's popup chrome and an ad-hoc `Text`. Token names refer to `spec/tokens.md`; `space(n)` is `Style.space(n)` / `Theme::space(n)`.

| Component | Qt (`import Omarchy.Ui`) | Rune (`omarchy_rune::ui`) | What it is |
|---|---|---|---|
| Pane | `Pane` | `Pane` | A flat surface inside an app window (inspector, side panel, settings group): padding, an optional Heading with actions, children in a column. No fill or border by default; a one-side rule or the controls' normal glass on request. |
| Heading | `Heading` | `Heading`, `HeadingLevel` | A bold title at a font token's size, an optional `textDim` subtitle, an optional trailing slot, left-aligned or centered. |

`Pane` shadows Qt Quick Controls' `Pane` (as `Button` and `TextField` do): qualify the imports. In Rune, SplitView's own pane is `SplitPane`.

## Pane

Qt `Pane` (on `BorderSurface`) · Rune `Pane`

**Card or Pane.** Card is the `[popups]` surface: the popups background, the accent (Hyprland active) border, for the shell's panels, popups and flyouts that float over everything. A pane belongs to a window: an app's inspector, side panel or a group on a settings page. It sits on the window background and must not look like a popup, so it never takes the popups border, never a shadow, and is separated from its neighbour by at most a rule.

**Anatomy.** Top to bottom, `panelPadding` inside its edges (plus the border or rule on that side):

1. **Header** (when `title` is set): a Heading at the `title` level with the `subtitle` and, as its trailing slot, the `actions` (IconButtons, a Button, a ToggleSwitch). It fills the content width, so the actions sit at the end.
2. **Content**: the children in a column, `panelGap` apart, `panelGap` under the header.

**Looks.** Three, from what the Omarchy apps need:

| Look | Qt | Rune | Fill | Edge | Corners | For |
|---|---|---|---|---|---|---|
| Plain (default) | | | none (the window shows) | none | none | A region that only needs padding and a header |
| Divided | `divider: "left"` (`"top"`, `"right"`, `"bottom"`) | `.divider(Side::Left)` (`rune::Side`) | none | a `hairline` rule in `track` along that side, inside the pane's bounds, full length | square | A side panel or inspector against the canvas or editor beside it |
| Bordered | `bordered: true` | `.bordered(true)` | `controls.normal` fill | `controls.normal` border (width from the token, 1 by default) | `radius` | A group set off inside a page or a larger pane |

The divided rule is the same line as Separator, the Toolbar's rule and the SplitView handle. When a Pane sits in a SplitView, leave the divider off: the handle is the rule. `bordered` and `divider` can be combined but rarely should be.

**API.**

| Qt | Rune | |
|---|---|---|
| `title`, `subtitle` | `.title(t)`, `.subtitle(s)` | The header; none when `title` is empty |
| `actions: [..]` (Heading's trailing slot) | `.actions([El])` | |
| `divider: string` | `.divider(Side)` | `""` (default) or one side |
| `bordered: bool` | `.bordered(bool)` | |
| children (a column) | `.child(El)`, `.children(..)` | |
| `foreground` | | The header's ink (default foreground) |
| `contentPadding`, `spacing` | | Default `panelPadding`, `panelGap` |
| width, height (implicit from content) | `.width(w)`; height from content, or chain `.h(..)` / `.h_full()` on the El | |

**Metrics.** Padding `panelPadding` (18) on every side, plus the bordered look's border widths, plus `hairline` on the divided side. Header to content and between children `panelGap` (14). Rule `hairline` (1) in `track`. Bordered corners `radius`; the other looks draw nothing to round. No shadow, no gradient.

## Heading

Qt `Heading` · Rune `Heading`

**Anatomy.** A column of the title (bold) over an optional subtitle (`textDim`, regular), `xxs` apart; then, when given, the trailing items, `controlGap` after the lines, `xxs` apart from each other and centered against the lines. Both lines are single lines that elide at the end.

**Levels.** Each level maps to font tokens, so `omarchy display text size` scales them:

| Level | Qt `level` | Rune | Title | Subtitle | For |
|---|---|---|---|---|---|
| title (default) | `"title"` | `HeadingLevel::Title` | `title` (14) bold | `bodySmall` (11) | Panes, toolbars, inspector and dialog heads |
| heading | `"heading"` | `HeadingLevel::Heading` | `heading` (16) bold | `body` (12) | Sections of a page, a settings page's groups |
| display | `"display"` | `HeadingLevel::Display` | `display` (24) bold | `subtitle` (13) | A page's or an empty window's title |

**Alignment.** Left by default. `centered` centers both lines in the space before the trailing slot: a toolbar's title in its center slot, a dialog title. The Heading's own width is its widest line plus the trailing slot, so in a Toolbar's center slot it centers on the bar.

**Not SectionHeader, not Hero.**

- **SectionHeader** (`PanelSectionHeader` / `section_header`) is a *caption*: `caption` size, bold, `textDim`. It labels a group of rows inside a panel ("Networks", "Frame") and sits under a heading, never in place of one.
- **Hero** is a shell panel's status head: a `display`-size glyph, a `title` bold name, an uppercased, letter-spaced `caption` status line (urgent on error) and a control. It reports a device or service, not a place in an app.
- **Heading** names the surface itself (a pane, toolbar, section or page) in the foreground at `title` size or larger, with a plain-case subtitle.

**API.**

| Qt | Rune | |
|---|---|---|
| `text` | `new(text)` | |
| `subtitle` | `.subtitle(s)` | Hidden when empty |
| `level: "title" \| "heading" \| "display"` | `.level(HeadingLevel)` | |
| `centered: bool` | `.centered(bool)` | |
| `trailing: [..]` (default property: children go here) | `.trailing([El])` | |
| `foreground` | | Title ink (default foreground) |
| width (implicit: the widest line + trailing) | natural width; chain `.w_full()` to fill | Accessible role heading |

**Metrics.** Lines `xxs` (2) apart; trailing `controlGap` (8) after the lines, items `xxs` apart, vertically centered against the lines. Height is the taller of the lines and the trailing slot.

## Gallery

### `surfaces` (after the Card, Tooltip and ConfirmDialog row)

Two more rows, `xxl` apart, each `huge` apart within and top-aligned:

1. **Panes**:
   - A divided Pane 240 wide, `divider` left: title "Layers", subtitle "3 items", action an IconButton U+F067; content a column of MenuRows Background, Title (checked), Logo.
   - A bordered Pane 240 wide: title "Night light", subtitle "Sunset to sunrise", action a ToggleSwitch on; content a SettingRow "Strength" with a 100-wide Slider at 0.6.
2. **Headings**, two columns 240 wide, the headings `huge` apart:
   - "Settings" / "Omarchy" at display; "Appearance" / "Theme, font and rounding" at heading.
   - "Frame" at title with a trailing IconButton U+F141; "notes.md" / "~/Documents" at title, centered.
