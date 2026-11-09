# Terminal bindings

How the token contract ([`tokens.md`](tokens.md)) and every component ([`components.md`](components.md), [`components/`](components/)) map onto a terminal's cell grid, so Ratatui (Rust, the reference), Bubble Tea + Lip Gloss (Go) and Textual (Python) draw the same cells. The GUI specs say what a component is; this file says what it is in cells. Where the two disagree, this file wins for terminal bindings.

The reference implementation is `tui/ratatui` (`omarchy-ratatui`, the gallery `omarchy-ratatui-gallery`). When this file and Ratatui disagree, one of them is a bug: fix it here first, then in Ratatui, and `bin/review` shows every other terminal binding against it, cell by cell.

## The grid

One cell is one column × one row of the terminal's single font at its single size. Nothing a terminal binding draws depends on pixels. A cell holds one character (a wide character takes two cells), a foreground color, a background color and attributes (bold, faint, italic, underline, reverse, crossed out).

- **Widths** are display widths as `unicode-width` 0.2 computes them (Go: `github.com/mattn/go-runewidth` with East Asian ambiguous = 1; Python: `wcwidth`), and Nerd Font glyphs (private use area) are 1 cell. Combining marks join the cell before them.
- **Elision**: text cut to `w` cells keeps the longest prefix that fits in `w − 1` cells and appends `…` (U+2026). Text that fits is never cut.
- **Centering** a run of `n` cells in `w` puts `floor((w − n) / 2)` before it.
- **Wrapping** (word wrap) breaks at spaces; a word longer than the line stays whole on its own line and is then elided; hard newlines are kept.

## Color

### Depth

| Depth | When | Written as |
|---|---|---|
| 24-bit | `COLORTERM` is `truecolor` or `24bit` (every Omarchy terminal) | `38;2;r;g;b` / `48;2;r;g;b` |
| 256 | else `TERM` contains `256color` | `38;5;n` / `48;5;n`, n in 16–255 only |
| 16 | otherwise | `30–37`, `90–97`, `40–47`, `100–107`, `39`, `49`, and `2` (faint) |

`OMARCHY_UI_COLORS` = `24bit` (or `truecolor`), `256` or `16` overrides the detection. Snapshots are always 24-bit.

### Compositing

Components ask for token colors, many with alpha (the `[controls]` fills are the foreground at 0.04–0.22). A cell has no alpha, so every color is composited when it is painted, per 8-bit channel, in sRGB:

```
out = (top × a + under × (255 − a) + 127) / 255        (integers; a is top's alpha byte)
```

That is `round((top × a + under × (255 − a)) / 255)` exactly, the core's `Rgba::mix(under, top, a / 255)`. `under` is the background already painted in that cell, or `palette.background` where nothing is. Ink (the foreground) composites over the cell's background the same way. A scrim composites over both the background and the ink of every cell under it, so what is under a modal fades toward the scrim.

### The page is the terminal's

**The theme background is never painted.** After a frame is drawn, every cell whose background equals `palette.background` (as composited, so exactly the theme background) is written with the default background (`49`): the terminal shows its own, Omarchy-themed background, with its opacity. Surfaces whose background is the theme's (Card, Menu, Tooltip, Toast, Dialog, the popups) therefore clear their cells to the default background rather than painting them; they are visible by their border. What does paint: the controls' fills (normal, hover, focus, pressed, selected), tones (badges, trim handles, toned fills), the scrim's effect on what is under it, swatches and picker pixels, and any surface background that differs from `palette.background`.

### 256 colors

Each composited color becomes the xterm-256 index in 16–255 nearest by squared sRGB distance (`Δr² + Δg² + Δb²`; ties to the lower index). The cube's levels are 0, 95, 135, 175, 215, 255 (index `16 + 36r + 6g + b`); the grays are `8 + 10i` (index `232 + i`). The theme background is `49` and the theme foreground `39`.

### 16 colors

The terminal's own 16 colors are the theme's (Omarchy themes every terminal), so the 16-color mode names roles by ANSI index instead of mapping RGB. Each composited color is matched to the nearest entry of the role tables below, computed from the current tokens (composited over the theme background), exact matches first, ties to the earlier entry:

| Ink role (foreground) | Written | Fill role (background) | Written |
|---|---|---|---|
| foreground | `39` | background | `49` |
| textDim | `90` | controls.normal.fill | `49` |
| textFaint | `90` + faint (`2`) | controls.selected.fill | `49` |
| muted | `90` | controls.hover.fill | `100` |
| accent | `34` | controls.focus.fill | `100` |
| urgent | `31` | controls.pressed.fill | `100` |
| success | `32` | accent | `44` |
| warning | `33` | urgent | `41` |
| background | `30` | success | `42` |
| controls.normal.border | `90` | warning | `43` |
| controls.hover.border | `90` | foreground | `47` |
| track | `90` | | |

Ink that lands on the same index as its cell's background is written as `39`. The selected state is shown by its bold label at 16 colors (its fill maps to the default background).

### Fades

A theme switch fades every color over `motion.theme` (280 ms) on `motion.ease` (`cubic-bezier(0.25, 0.1, 0.25, 1)`), per channel in sRGB, by redrawing ([`following.md`](following.md)): the binding interpolates the tokens (as `omarchy-gpui`'s `mix_tokens` does) and draws every frame from the interpolated set, at least 60 frames a second while it runs, landing exactly on the new tokens. Only at 24-bit: at 256 and 16 colors, under reduced motion and on the first frame, the new colors land at once. Layout never fades (there is nothing in cells that could).

## Metrics

Font sizes and the `[spacing]` scale don't apply: the terminal owns the font and its size (`omarchy display text size` changes the GUI, not the grid). Spacing tokens map to fixed cells:

| Tokens (px at scale 1) | Columns | Rows |
|---|---|---|
| `hairline`, `xxs`, `xs`, `sm` (1–4) | 0 | 0 |
| `md`, `lg`, `xl`, `controlGap`, `labelGap`, `rowGap`, `controlPaddingX`, `rowPaddingX` (6–12) | 1 | 0 |
| `xxl`, `xxxl`, `panelGap` (12–14) | 1 (2 between side-by-side groups) | 1 |
| `huge`, `panelPadding`, `popupPadding` (14–18) | 2 | 1 |
| `huge × 2` | 4, or 2 where the row is full | 1 |

- A **control** (Button, TextField, Dropdown trigger, NumberField, Keycap, Banner, ListRow, ...) is **one row** tall.
- **Pixel widths** in `gallery.md` and the component specs become `floor(px / 8 + 0.5)` columns (220 → 28, 240 → 30, 300 → 38, 360 → 45, 420 → 53, 480 → 60, 720 → 90). Heights are given per component and specimen below; rows of content are one row each, so pixel heights don't convert.
- **Surface padding** (Card, Pane, Dialog, SearchList): 2 columns × 1 row inside the border. Children of a surface: 1 row apart.

## Corners and borders

- Boxes (Card, Pane bordered, Menu, Tooltip, Toast, Dialog, popups, SearchList, the SplitView frame) are drawn with **light** box-drawing lines whatever the border width: `─ │` and corners `┌ ┐ └ ┘` at radius 0, `╭ ╮ ╰ ╯` at radius > 0. A cell can't show 1 px against 2 px, and heavy lines have no rounded corners. Width 0 draws nothing (and reserves nothing).
- The border's color is the spec's color; a gradient border uses its first stop (`BorderSpec.color`).
- Heavy lines (`━ ┃ ┏ ┓ ┗ ┛`) are reserved for what is *chosen or grabbed*: the selected tab's underline, crop handles, the countdown, meters and sliders' played part, the drop indicator.
- A box's title (Card) sits in its top edge: `┌ Title ───┐`, starting at the third column, a space each side, bold `textDim` (a SectionHeader).
- Small radii (`min(4, radius)`), pills and circles have no cell form: switches, knobs, chips and badges are square at every radius.

## States

The `[controls]` ladder, **pressed > focus > hover-cursor > selected > normal**, in cells:

- **The control frame.** A one-row control spans `[edge][pad] content [pad][edge]`. The **fill** covers all its cells (edges included). The **edges** are its first and last cell: `▏` (U+258F, LEFT ONE EIGHTH BLOCK) and `▕` (U+2595, RIGHT ONE EIGHTH BLOCK) in the state's border color, so the border is a thin line on the control's outer sides. A control reserves its edge cells whenever any of its states shows a border (so a state change never relayouts); when the current state shows none, the edge cells are just fill.
- **normal**: `controls.normal.fill`, edges in `controls.normal.border` when the control is bordered at rest (bordered buttons, inputs, dropdowns, keycaps).
- **hover-cursor and focus**: `controls.hover.fill` (`controls.focus.fill` for real focus), edges in their border. The keyboard cursor and focus look like hover: keyboard first. Pointer hover where the terminal reports motion (every Omarchy terminal does, with mouse reporting on).
- **selected**: `controls.selected.fill`, its border (width 0 by default: no edges), the label **bold** in `controls.selected.text`.
- **pressed**: `controls.pressed.fill` while the mouse button is down.
- **Accent marks only what is chosen**: checks, the chosen radio, a menu's cursor text, the selected tab's rule, the selected rail item, today, handles and the playhead. **Urgent** only what destroys.
- **Bold** only for selection and titles (Heading, Hero title, bold ListRow titles, section headers).
- **Disabled**: everything in `textFaint`, no hits.

## Type

The terminal has one font at one size. The type scale collapses into weight and ink:

| Tokens | In cells |
|---|---|
| `displayLarge`, `display`, `heading`, `title` (titles) | bold, foreground |
| `subtitle`, `body`, `bodySmall` | regular, foreground (or the role the component spec gives) |
| `caption` | regular; as the spec says: section headers bold `textDim`, hints `textDim` |

Nothing is uppercased except where the GUI spec uppercases (Hero's status line, the Calendar's month and weekdays); letter spacing is dropped. Headings of different levels differ only in what they title.

## Icons

The kit's glyphs are Nerd Font codepoints and work as they are (Omarchy's monospace is JetBrainsMono Nerd Font): a glyph is one cell, followed by a space (two spaces before a ListRow's title). Freedesktop icons by name don't exist in a terminal: an Avatar given only an icon name draws its glyph if it has one, else its initial, exactly as when the GUI lookup finds nothing. Terminal bindings register `"icons": "none"`.

Images (Avatar pictures) are drawn in half-block pixels: each cell is `▀` with the upper pixel as ink and the lower as background, so a `w` × `h` cell area shows `w` × `2h` pixels. The pixel `(i, j)` of a `W` × `H` picture scaled to `w` × `2h` is the box `x ∈ [floor(i·W/w), max(floor((i+1)·W/w), that + 1))`, likewise for y, averaged with alpha weighting: color `round(Σ c·a / Σ a)`, alpha `round(Σ a / n)` (integer halves round up), then composited over the theme background. `round` avatars clear the pixels whose center lies outside the circle inscribed in the pixel grid.

## Motion

- The theme fade (above): 24-bit only, none under reduced motion.
- **Spinner**: turning is the braille frames `⠋ ⠙ ⠹ ⠸ ⠼ ⠴ ⠦ ⠧ ⠇ ⠏`, one every 90 ms (a turn in 900 ms, `frame = (ms mod 900) / 90`); pulse is the glyph (`󰦖` U+F0996) at opacity `0.5 + 0.5 × ease((ms mod 840) / 420, mirrored)` on `cubic-bezier(0.42, 0, 0.58, 1)`. Still (reduced motion, snapshots): frame 0, and the pulse at 0.5.
- **Indeterminate ProgressBar**: a segment `round(w / 4)` wide whose start is `round(−seg + (w + seg) × ease(p))`, `p = (ms mod 1400) / 1400`, same curve, clipped. Still: `[round(3w/8), round(3w/8) + round(w/4))`.
- Everything else (Tabs' underline, the rail's bar, the press give) moves in one step: cells can't slide.
- Snapshots, `bin/review` and the live tests run with `OMARCHY_UI_REDUCE_MOTION=1` except where they test the fade.

## Components

Every component of `components.md` and `components/*.md`, under the same names. "w" is the width given (columns), "edges" the control frame above. Glyphs are codepoints.

### Basics (`components.md`)

| Component | Cells |
|---|---|
| Button | 1 row, `label + 4` wide (`2 + glyph + 1 + label + 2` with an icon), label centered. Plain: no fill, no edges at rest; edges reserved. Bordered: normal border edges. Hover/cursor: hover fill and edges. Selected: selected fill, bold selected text. |
| IconButton | 5 × 1: edges, the glyph at column 2. Hover fill under the cursor; edges only when bordered. |
| ButtonGroup | Bordered Buttons 1 column apart; one selected. Left/Right, h/l move it; Home/End. |
| ToggleSwitch | 6 × 1: edges (the cursor ring: hover border), then 4 track cells in `normal.fill` (off) or `selected.fill` (on) with the knob `██`: cells 0–1 in foreground darker 1.25 (off), cells 2–3 in `selected.text` (on). Bare (inside a row): the 4 track cells. |
| Toggle | w × 2 (× 1 without a description): edges on both rows (normal at rest, hover with the cursor), normal fill; title bold at column 2, description `textDim` under it; the bare switch ends at `w − 2`. |
| TextField | w × 1, normal fill and edges (hover with the cursor), text from column 2, placeholder `textFaint`; the caret is the reverse-video cell at the insertion point while focused. |
| NumberField | Label row (`textDim`) when labeled, then w × 1: edges, `−` at column 2, `+` at `w − 3` (`textFaint` at the range's ends), the value centered between. |
| Dropdown | Label row (bold `textDim`) when labeled, then the trigger: w × 1, normal fill and edges, value from column 2, chevron `󰅀` U+F0140 at `w − 3` in foreground darker 1.2. Open, the list (DropdownList) under the trigger: a box in the popups border, one row per option, the highlighted row in the hover fill, the chosen bold with an accent `✓` at the end. |
| Slider | w × 1: edges with the cursor (hover border); the track is `w − 2` cells: knob at `k = round(v × (w − 3))` as `●` foreground, played cells `━` foreground, the rest `─` `track`; notches at `round(n / (N − 1) × (w − 3))` as `╋` (played) or `┼` (rest). |
| Card | A box in `surfaces.popups.border.color`, title in the top edge, padding 2 × 1, children 1 row apart; height = children + 4. |
| SectionHeader | 1 row, bold `textDim`. |
| Separator | 1 row of `─` in `track`. |
| MenuRow | w × 1: label at column 1, accent bold `✓` at `w − 2` when checked, hover fill when highlighted. |
| Tooltip | A box in `surfaces.tooltip.border.color`, 3 rows; text (`tooltip.text`) from column 2; with a shortcut, a KeyCombo one column after the text. |
| ConfirmDialog | A Dialog with no title, the message, a destructive Confirm selected. |

### Surfaces (`components/surfaces.md`)

| Component | Cells |
|---|---|
| Pane | Padding 2 × 1. Plain draws nothing. Divided: a `│`/`─` rule in `track` on its side (one cell inside the pane's bounds, the padding after it). Bordered: a box in `controls.normal.border` filled with `controls.normal.fill`. Header: a Heading (title, subtitle) with the actions at the end, 1 row above the content; children 1 row apart. |
| Heading | Title bold foreground, subtitle `textDim` on the row under it; the trailing items end at the width's end, 1 column apart and 1 after the lines, centered on the lines' rows. Every level the same (Type, above); `centered` centers the lines in the space before the trailing slot. |

### Lists (`components/lists.md`)

| Component | Cells |
|---|---|
| ListRow | w × 1 (+ the expansion's rows): edges reserved (hover border on the cursor row), fill: hover (cursor) > selected (current) > none. From column 2: the glyph and two spaces (a leading item, such as a small Avatar, and one space); the title bold; two spaces; the status in its tone, else the subtitle in `textDim`; elided to what is left. From the end: `[edge]`, a pad, the end slot (1 column: the trailing glyph in `textDim` or its tone; the action instead while the row has the cursor or `actionCursor`, `urgent` when destructive, with the hover alpha of its own ink behind it under `actionCursor`), a column, the trailing item, a column, the value in `textDim`. Disabled: everything `textFaint`. The expansion sits under the main line at column 2, inside the fill. |
| CursorModel | The same model and keys as the GUI bindings; the pointer moves it only when it reaches a different cell (the gate). |
| ScrollArea | Children stacked, clipped to `maxRows`; while the content overflows, the bar overlays the last column: groove `│` in `track`, thumb `┃` in `controls.normal.border` (`round(view² / content)` rows, ≥ 1, at `round(offset × (view − thumb) / (content − view))`). |
| KeyValue | Per grid column: labels (`textDim`) as wide as the widest, a space, values (foreground or tone) as wide as the widest; 3 columns between pairs; lines 0 rows apart. |
| Hero | w × 2: the glyph at column 0 (at its opacity), the title bold at column 3, the status uppercased bold `textDim` (urgent on error) under it; the trailing control ends at w, on the first row. |
| Badge | `label + 2` × 1, bold, padded a column: neutral `normal.fill` + foreground; toned: the tone at `selected.fill`'s alpha + the tone; filled: the tone + `background`. |
| StatusDot | `●` in its tone. |
| Avatar | Small 2 × 1, medium 4 × 2, large 6 × 3. Picture: half-block pixels (Icons, above). Otherwise a tile in `selected.fill` with the mark at `((w − 1) / 2, (h − 1) / 2)`: the initial bold, or the glyph. The status dot `●` (tone) in the bottom-right cell. |
| TileGrid | Tiles 11 × 4, no gap: the glyph centered on row 1 (or a medium Avatar on rows 0–1), the label wrapped to 9 columns on rows 2–3, centered, the second line elided. Cursor: hover fill and edges; current: selected fill, bold label. |

### Feedback (`components/feedback.md`)

| Component | Cells |
|---|---|
| Meter | Header row when labeled: label (foreground) at the start, value text (`textDim`) at the end. Bar row: `━` in `track` across w, the first `round(v × w)` cells `━` in the tone (urgent when alarming), markers `╋` in that color at `round(f × (w − 1))`. |
| ProgressBar | A Meter in accent; indeterminate: Motion, above. |
| Spinner | 1 cell: Motion, above. |
| EmptyState | A centered column in w: the glyph, the title (wrapped), the hint (wrapped), a blank row and the bordered action Button. Panel: glyph and title `textDim`, hint `textFaint`. Overlay: glyph `menu.selectedText` at 0.8, title foreground at 0.7, hint `textDim`. |
| Banner | w × 1, the control frame with edges in the tone (neutral is info) and the normal fill: the tone glyph at column 2 (info `󰋽` F02FD, success `󰄬` F012C, warning `󰀪` F002A, error `󰅚` F015A), the text from column 4; from the end: edge, pad, dismiss `󰅖` F0156 in `textDim`, a column, the bordered action Button, a column. |
| Toast | w × 4 (3 without text): a box in `notifications.border.color`, the hover fill inside with the cursor; glyph at column 2, title bold from column 4 (2 without a glyph), text (notifications text darker 1.15) under it, the bordered action Button at the end of the first row; the countdown is the bottom edge: its first `round(c × (w − 2))` inner cells `━` in `notifications.countdown`. |

### Navigation (`components/navigation.md`)

| Component | Cells |
|---|---|
| Menu, ContextMenu | A box in `surfaces.menu.border.color`, one row per entry, `max(20, widest row)` inner width. Item: column 1, `[glyph + space]` when any item has a glyph, the label, the hint (`textDim`) right-aligned before the mark column, `[mark]` (2 columns: `✓` or `●` accent bold, or the submenu chevron `` F054 `textDim`) when any item has one. Cursor row: `menu.selectedBackground` fill, label and glyph `menu.selectedText`. Disabled `textFaint`; destructive urgent. Separator: `─` in `track` across; section: bold `textDim` at column 1. Keys: Up/Down, k/j wrap over enabled rows, Home/End. |
| Keycap | `label + 4` × 1: `normal.fill`, normal border edges, the normalized label (the GUI table) at column 2. |
| KeyCombo | Keycaps side by side, no gap. |
| ShortcutSheet | Groups in `columns` columns (round robin), 4 columns apart, groups 1 row apart: a SectionHeader, then rows of the KeyCombo (in a column as wide as the group's widest) and the description 2 columns after. |
| Toolbar | w × (1 + 1 when divided): leading items from column 0, center items centered on w, trailing items ending before the overflow IconButton (`` F141), at w; then a row of `─` in `track`. ToolbarSeparator: 3 × 1, `│` `track` in the middle. |
| Tabs | 2 rows: tabs ` label ` (label + 2 wide, no gap); row 1 is `─` `track` across w and `━` accent under the selected tab. Labels `textDim`; cursor: hover fill, foreground; selected: bold `selected.text`. Left/Right, h/l, Home/End. |
| IconRail | 5 columns, a row per item: column 0 `▎` accent for the selected item, columns 1–3 the item (` G `, hover fill under the cursor), the glyph `textDim` / foreground (cursor) / accent (selected); column 4 `│` `track` the rail's height. Up/Down, k/j, Home/End. |

### Forms (`components/forms.md`)

| Component | Cells |
|---|---|
| SettingRow | w × (1, or 2 with a description): the label (foreground) at column 1, the description (`textDim`) under it, the control ending a column before the row's end, on the first row; hover fill across with the cursor; disabled words `textFaint`. |
| Checkbox | `[✓] Label`: the box is 3 cells: brackets in `normal.border` (hover border with the cursor), its 3 cells in `normal.fill` / `selected.fill` (checked) / `hover.fill` (cursor); the mark `✓` accent bold. Label at column 4, description under it. Disabled: box at half alpha, words `textFaint`. |
| Radio | `(●) Label`, the same. |
| RadioGroup | Radios stacked, no gap; arrows and j/k/h/l move the choice, stopping at the ends. |
| PasswordField | A TextField with the lock `` F023 (`textDim`) at column 2, the text as `•` from column 4, the eye `󰈈` F0208 / `󰈉` F0209 at `w − 3` when revealable. Error: edges and lock in `polkit.borderError` / `polkit.textError`, the message on the row under it from column 2. |
| TextArea | w × `clamp(lines, min, max)`: the TextField frame on every row, text wrapped to `w − 4`. |
| MultiSelect | Label row (bold `textDim`), the trigger (as Dropdown: the chosen labels joined by ", ", "N selected" past three, "None selected" in `textFaint`). Open, under it: a box in the popups border, the search row (`󰍉` F0349 `textDim` at column 1, the query or the placeholder `textFaint` from column 3, in `controls.focus.fill` whenever no option row is highlighted: the keys are in the search row from the moment it opens until Down moves them into the list, and back when Up leaves its first row, so open with no row highlighted *is* the keys in the search row, whether or not the MultiSelect itself has focus), a rule `─` (foreground at 0.10), then a row per option (at most 6): the Checkbox box at column 1, the label at column 5 (bold when chosen), the highlighted row in the hover fill. Keys as the GUI bindings. |
| ColorWell | 7 × 1: the well `▏███▕` at columns 1–5 (edges in `normal.border`, the color as `█` ink); the cursor adds the ring: edges in `hover.border` at columns 0 and 6. |
| SwatchStrip | Chips `██` (ink) 1 column apart; row 2 marks `▔▔` under the chosen chip (accent) or the cursor's (hover border); 2 rows. |
| Dialog | Over the scrim (composited on what is under the whole area), a box in `menu.selectedText` centered: padding 2 × 1, title bold, a blank row, the message (wrapped), the body, a blank row, then the buttons ending at the inner right edge, 1 column apart: each `max(11, label + 4)` wide, label centered. Selected: `menu.selectedBackground` fill, edges and bold label `menu.selectedText`; destructive selected: urgent at 0.22, urgent edges and label. Unselected: no fill, edges foreground at 0.38 (urgent at 0.56 when destructive), label foreground. |

### Data (`components/data.md`)

| Component | Cells |
|---|---|
| Table | w × (2 + rows): header titles bold `textDim` padded a column each side, the sort chevron (`󰅃` F0143 / `󰅀` F0140, `textDim`) 1 column after the title (2 before a right-aligned one); a `─` `track` rule; then a row per item. Fixed columns take their width, the rest `floor((w − fixed) / flex)`. Row ladder: hover fill (cursor) > selected fill with bold `selected.text` > none, across w. No rules between body rows (they would double the height). |
| Tree | A row per visible node: the inset `1 + 2 × depth`, the chevron (`󰅂` F0142 / `󰅀` F0140, `textDim`) and a space on branches, the glyph (`textDim`, `selected.text` when selected) and a space, the label; the Table's row ladder across w. |
| SplitView | Panes side by side, a 1-column handle between each: `│` in `track`, in the hover fill when hot. |
| Sidebar | Sections 1 row apart: the header (bold `textDim`) at column 1, rows: the glyph (`textDim` / `selected.text`) at column 1, the label at column 4; the Table's ladder. Collapsed: 5 columns, the glyphs at column 2, no headers. |
| Calendar | 28 × 8: row 0 `󰅁` F0141 at column 1, `MONTH YEAR` (`textDim`) centered in columns 3–24, `󰅂` F0142 at column 26; row 1 `MO TU WE TH FR SA SU`, bold `textDim`, each at `4i + 1`; rows 2–7 the six weeks from the Monday on or before the 1st: day cells 4 wide, the number right-aligned in columns 1–2. Today: edges in accent, bold. Selected: selected fill on the 4 cells, bold `selected.text`. Cursor: hover fill. Weekends `textDim`, other months `textFaint`. |

### Editing (`components/editing.md`)

| Component | Cells |
|---|---|
| Timeline | w × h: `x(t) = min(w − 1, round(t / duration × w))`. Row 0: labels (m:ss, `textDim`) for the major ticks, centered on them (`x − (len − 1) / 2`), clamped inside. Row 1: ticks, `│` major, `╷` minor (a fifth of the step), `textFaint`. The step is the first of 0.1, 0.2, 0.5, 1, 2, 5, 10, 15, 30, 60, 120, 300, 600, 900, 1800, 3600 s with `step × w / duration ≥ 11`. Lanes share the rows under the ruler (`(h − 2) / n` each, the last takes the rest), filled with `normal.fill`. A clip covers `[x(start), max(x(end), x(start) + 2))`: `normal.fill` over the lane, normal border edges on every row, the label from its third cell on the lane's middle row (`(lh − 1) / 2`). The selected clip: `selected.fill`, bold label, its first and last columns trim handles. The playhead: `▼` on row 1 and `│` down the lanes in accent (hot accent with the cursor). |
| Scrubber | w × 1: elapsed and total (m:ss, `textDim`) at the ends, the bar between them a column from each: the Slider's track (no edges). |
| TrimHandle | Trim: a column of accent cells, the grip `│` in `background` on the middle `round(h × 0.4)` rows (0.6 hot, at least 1). Crop: heavy accent strokes: corners `┏━`/`━┓`/`┗━`/`━┛` with `┃` one row in, sides `━━` (top, bottom) and `┃` (left, right); hot accent when hot. |
| CropFrame | w × h: a light accent outline (square corners), thirds guides at `w·i/3` (`┆`) and `h·i/3` (`┄`, `┼` where they cross) in `track`, the eight crop handles over the outline, the sides' at `(w − 2) / 2` and `(h − 1) / 2`. |
| DragHandle | `⠿` U+283F, `textDim` (foreground when hot). |
| DropIndicator | A row of `━` in accent at the drop slot. Cells can't draw between rows, so it takes a row. |
| ColorPicker | w × h: the square is `(w − 6)` × `(h − 5)` cells of half-block pixels (2 per cell): pixel `(i, j)` is HSV(h, `i / (sqW − 1)`, `1 − j / (2·sqH − 1)`), converted as every binding's `Hsva::rgb8`. Its knob: `●` in the picked color on the theme background at `(round(s × (sqW − 1)), round((1 − v) × (sqH − 1)))`. Hue strip 2 columns at `sqW + 1` (hue `j / (2·sqH − 1)` top to bottom), alpha strip 2 columns at `sqW + 4` (the color at alpha `1 − j / (2·sqH − 1)` over `track` over the background); their knobs `◀▶` foreground on the background at the value's row. Row `sqH + 1`: the preview `██` (the color with alpha over the background) and, from column 3, a TextField with the hex. Row `sqH + 3`: the palette (`red orange yellow green cyan blue magenta foreground`, missing keys skipped) as `██` 1 column apart, the current color's marked `▔▔` (foreground) on the row under. |

### Composed (`components/composed.md`)

| Component | Cells |
|---|---|
| SearchList | w × h: a box in `menu.border.color`, padding 2 × 1; the query (`menu.text`) or the placeholder (`textDim`); a blank row; the body: ListRows (1 row each) in a ScrollArea; with the detail pane the list is `floor(55% × body)` wide, then 2 blank columns, a `│` `track` rule down the body, 2 blank columns, and the pane to the body's end (the rule at the list's end + 2, the pane at the rule + 3): the cursor item's title bold over its detail wrapped in `textDim`. No items: the overlay EmptyState (`󰍉` and "No matches for “query”") centered in the body. |

## Gallery

Every section is **96 columns** wide and its natural height tall (the reference sizes itself; `bin/review` gives every other terminal binding exactly the reference's size). A section is: a blank row; from column 2 the title (a SectionHeader), a blank row, the body; a blank row. Groups in a body are 1 row apart (`col(1)`), side-by-side items 1 column apart (`row(1)`, inline controls) or 2 (`row(2)`, groups), and items in a list 0 rows apart.

| Section | Size | Body (top to bottom) |
|---|---|---|
| `buttons` | 96 × 7 | `row(1)`: Plain; Bordered; Hover (bordered, cursor); Selected (bordered, selected); Icon (bordered, `` F013); IconButton `` F011 bordered; IconButton `` F021 bordered, cursor. ButtonGroup Left/Center/Right, Center selected. |
| `toggles` | 96 × 11 | `row(2)` of four ToggleSwitches: off, on, off + cursor, on + cursor. Toggle Wi-Fi / Connected to Basecamp, on, w 45. Toggle Bluetooth / Off, off, cursor, w 45. |
| `inputs` | 96 × 8 | `row(1)`: TextField placeholder "Search…" w 28; "Omarchy" w 28; "Hovered" cursor w 28. `row(2)`: NumberField "Gaps" 10 (0–40) w 15; Dropdown "Theme" Tokyo Night w 30. |
| `slider` | 96 × 7 | Slider 0.35 w 45. Slider 0.75 with 5 notches w 45. |
| `surfaces` | 96 × 34 | `row(2)`: Card "Network" w 38 (Toggle Wi-Fi / Basecamp on w 32, Separator 32, MenuRows Basecamp checked, Basecamp Guest highlighted, Coffee Shop, w 32, no gap); `col(1)`: Tooltip "Restart Wi-Fi", and a 43 × 11 box holding ConfirmDialog "Remove the Coffee Shop network?" (w 39) centered over the scrim. `row(2)`: Pane Layers / 3 items, divider left, action IconButton `` F067, w 30, MenuRows Background, Title (checked), Logo w 25; Pane Night light / Sunset to sunrise, bordered, action ToggleSwitch on, w 30, SettingRow "Strength" with a Slider 0.6 w 13 (row w 24). `row(2)`: `col(1)` Heading Settings / Omarchy (display) w 30, Heading Appearance / Theme, font and rounding (heading) w 30; `col(1)` Heading Frame with a trailing IconButton `` F141 w 30, Heading notes.md / ~/Documents centered w 30. |
| `lists` | 96 × 25 | ScrollArea w 53, 7 rows, the nine ListRows of `lists.md` (w 53; cursor on row 1, Basecamp current; Messages leads with a small Avatar and trails a filled accent Badge "3"; Headphones trails a bare ToggleSwitch on; Office with the action cursor). Three rows with trailing glyphs (w 53, no gap). Hero Wi-Fi w 53 (trailing ToggleSwitch on). Hero Bluetooth (error, glyph at 0.5) w 53. KeyValue, 2 columns. |
| `tiles` | 96 × 25 | `row(1)` of Badges New, 3 (accent filled), 12 (urgent filled), Online (success), Low (warning), Offline (urgent). `row(2)`: StatusDots success, warning, urgent, muted, accent (`row(1)`), then medium Avatars Ryan (round, success), the picture (warning), glyph `󰒋` F048B (urgent). `row(2)` of Avatars: ada small, Basecamp, Omarchy large round, glyph `󰚩` F06A9, the picture small, medium round, large, and Missing (no picture: "M"). `row(2)` of Avatars by icon name, which draw their initials (Files, Terminal, Firefox, Chromium small, Steam large, Btop small) and Nothing with glyph `` F120. TileGrid of 4 columns (Browser under the cursor; Photos the picture). |
| `progress` | 96 × 12 | `row(2)` of Meters w 28 with markers 0.5 and 0.8: 0.2; 0.65; 0.92 alarming. Meter 0.65 Battery / 65% w 45. `row(2)`: ProgressBar 0.4 w 43; indeterminate w 43. `row(2)`: Spinner, Spinner, Spinner pulse, Spinner pulse. |
| `messages` | 96 × 26 | `row(2)` of two 44 × 9 boxes, each centering an EmptyState w 44: panel `󰖪` F05AA No networks found / Turn on Wi-Fi to scan / Scan; overlay `󰅌` F014C Clipboard is empty / Copy something to see it here. Four dismissible Banners w 92, 1 row apart: info "Omarchy 3.2 is available" with Update; success "Backup finished"; warning "Battery is low (12%)"; error "Could not reach the update server". Toast `󰄬` Export finished / omarchy-demo.mp4 · 42 MB / Open, countdown 0.6, w 45. |
| `menus` | 96 × 26 | `row(2)`: the Menu and ContextMenu of `navigation.md` (cursor on Open…; none on the ContextMenu), Tooltip "Save" with Ctrl+S. `row(1)` of Keycaps Super, Ctrl, Alt, Shift, Tab, Esc, Enter, Backspace, Up, Space, K. KeyCombo Ctrl+Shift+P (its own row: 96 columns don't hold both). ShortcutSheet, 2 columns. |
| `navigation` | 96 × 15 | Toolbar w 92: leading IconButtons `` F032, `` F033 (cursor), `` F0CD, separator, `` F0CA, `` F0CB, separator, `` F03E, `` F0C1; center Heading notes.md centered; trailing bordered Button Share; overflow. Tabs General, Display (selected), Audio (cursor), Network, w 92. `row(2)`: IconRail of five (Search selected), a 40 × 5 box with "Search" centered in `textDim`. |
| `forms` | 96 × 24 | SettingRows w 60, no gap: Night light / Warm the screen after sunset + ToggleSwitch on; Theme / Applies to every app + Dropdown w 23; Gaps / Space between windows + NumberField w 15. `row(4)`: Checkboxes, no gap (Show seconds; Show date checked; Show weekday cursor; Show week number disabled; Twelve-hour clock / Show AM and PM checked), RadioGroup Top/Bottom/Hidden (Bottom). `row(1)` of PasswordFields w 28: empty "Password"; "hunter22" revealable; "letmein" with "Wrong password, try again". TextArea w 60, the three lines. |
| `pickers` | 96 × 34 | `row(2)`: MultiSelect Bar modules w 33 closed, 4 chosen ("4 selected"); a 33 × 11 box with MultiSelect Notify for, open, Mail and Updates chosen. `row(1)` of ColorWells: accent, success with the cursor, urgent. SwatchStrip accent, urgent, warning, success, foreground, textDim, textFaint, muted, warning chosen. A 53 × 13 box: Card "Displays" filling it (MenuRows Built-in display checked, External monitor, Projector, w 47) under Dialog Discard changes? / Your edits to Tokyo Night will be lost. / Keep editing, Discard (destructive, selected), w 46, with the scrim. |
| `data` | 96 × 12 | `row(2)`: Table w 60 (Name flex, Kind 11, Size 9 right, Modified 13; sorted by Name ascending; Cargo.toml selected, the screenshot under the cursor); Tree w 28 (as `data.md`; root, qt and spec open; Table.qml selected; README.md under the cursor). |
| `layout` | 96 × 27 | A 90 × 14 box (`track`, the radius) framing a SplitView 88 × 12: a Sidebar w 25 (Library, Places; Recent selected), the handle, "No file selected" centered in `textFaint`. Calendar November 2026, today the 8th, the 14th selected. |
| `timeline` | 96 × 26 | Timeline 90 × 10, 0:30, playhead 0:12, the clips of `editing.md` (Demo selected). Scrubber 0:42 of 2:10, w 45. `row(2)`: a 45 × 5 list of Workspaces, Clock (cursor: hover fill, foreground grip), Battery, the DropIndicator row, Network (each `⠿` at column 1, the label at 3); a 30 × 9 rectangle in `normal.fill` with a CropFrame over it. |
| `color` | 96 × 20 | ColorPicker 38 × 16 for `#7aa2f7`. |
| `composed` | 96 × 37 | `row(2)`: the Wi-Fi Card w 45 (Hero, SectionHeader Networks, a ScrollArea of the four ListRows w 39 with Basecamp Guest under the cursor and Coffee Shop expanded with a PasswordField w 23 and a bordered Connect, Separator, KeyValue IP / Gateway / DNS); the inspector Pane w 45, divider left (Tabs Shape/Cursor/Zoom; Frame: Radius with a Slider w 14, Border with a Dropdown Solid w 14, Shadow under the cursor with a ToggleSwitch on; Background: Fill with an accent ColorWell, Padding with a NumberField 24 w 15; rows w 40). `row(2)`: SearchList 45 × 12 "term" with the detail pane, the second result under the cursor; SearchList 38 × 9 "zzz" (no matches). |
| `tokens` | 96 × 18 | Swatches 8 × 2 (the color as background, normal border edges) 13 columns apart with the names under them: foreground, background, accent, urgent, muted. State chips 15 × 1, 16 apart: normal, hover-cursor, focus, selected (fill, border edges when the state has a border, label centered, selected bold). The type scale: per token `name px` in `textDim`, then "The quick brown fox jumps over the lazy dog" at column 16 in the role (Type, above). |

The sample picture is `qt/gallery/assets/avatar.png`.

## Snapshots

A terminal gallery's `--snap out.ans --section NAME --size COLSxROWS` writes the section as its own frame buffer, serialized (never a re-implementation of the drawing), in this format:

- UTF-8, exactly ROWS lines, each ending in `\n`; each line exactly COLS columns wide (Widths, above). `--size COLSx0` means the section's natural height; a given ROWS crops or pads with blank default cells; COLS defaults to 96.
- Only SGR sequences (`ESC [ … m`). Every line starts in the default state.
- 24-bit colors only (`38;2;r;g;b`, `48;2;r;g;b`), `39`/`49` for the defaults, attributes `1` bold, `2` faint, `3` italic, `4` underline, `7` reverse, `9` crossed out. Rendered as the theme was resolved under the caller's `OMARCHY_UI_*` (reduced motion: no fade, still spinners), after the "page is the terminal's" rule.
- **Canonical form** (what Ratatui writes; a binding should write it too, so snapshots compare byte for byte): per cell, left to right (a wide character's second column writes nothing), compute the style (fg, bg, bold, faint, italic, underline, reverse, crossed). A space with none of underline, reverse, crossed is normalized to the default foreground and no bold, faint or italic. When the style differs from the previous cell's (a line starts from the default), write `ESC[0`, then `;1`, `;2`, `;3`, `;4`, `;7`, `;9` for the attributes set, `;38;2;r;g;b` unless the foreground is the default, `;48;2;r;g;b` unless the background is, then `m` (`ESC[0m` alone for the default style); then the cell's text. At the end of a line whose last style isn't the default, `ESC[0m`; then `\n`.

`target/release/omarchy-ansi` (`crates/omarchy-ansi`, built by `bin/build core`) is the shared tool:

| Command | Does |
|---|---|
| `omarchy-ansi check IN.ans [--size CxR]` | Parses strictly (`ok CxR`, else exit 1 and why) |
| `omarchy-ansi size IN.ans` | Prints `CxR` |
| `omarchy-ansi canon IN.ans` | Prints the canonical form (compare it with your file) |
| `omarchy-ansi png IN.ans OUT.png [--scale 2] [--font-size 12]` | Renders it (below) |
| `omarchy-ansi diff A.ans B.ans [--png D.png]` | Prints `{"cols","rows","sizeMatches","differing","total","percent"}`; `--png` writes the differing cells as a translucent overlay |

**The cell diff** compares effective cells: the text, the foreground and background with defaults resolved to the palette and reverse applied, and bold, faint, italic, underline, crossed; a space without underline or crossed compares only its background. Cells outside the smaller grid count as differing.

**The renderer** draws every terminal binding through one rasterizer, so review differences are real cell differences: the core's resolved monospace face (fontconfig, honoring `OMARCHY_UI_FONT_FAMILY`; bold and italic faces by `fc-match "<family>:style=Bold"`; a missing glyph from `fc-match ":charset=<hex>"`), at `font-size × scale` px (12 × 2: cells 14 × 32 px for JetBrainsMono Nerd Font), cell width the advance of `M`, height `ceil(ascent − descent + gap)`; backgrounds first, then glyphs unclipped (a Nerd glyph may overflow into the next cell, as in foot); box drawing, block elements and braille drawn as pixels (light lines `max(1, round(P/12))`, heavy twice that); faint ink is the average of ink and background.

## Harness

A terminal binding is a registry entry with `"family": "terminal"` ([`AGENTS.md`](../AGENTS.md), A new framework):

```json
{
  "name": "bubbletea",
  "title": "Bubble Tea",
  "family": "terminal",
  "order": 120,
  "docs": "docs/using-bubbletea.md",
  "available": { "commands": ["go"] },
  "build": "tui/bubbletea/build",
  "gallery": {
    "sections": "build/bubbletea/omarchy-bubbletea-gallery --list-sections",
    "snap": "build/bubbletea/omarchy-bubbletea-gallery --snap {out} --section {section} --size {width}x{height}",
    "window": "tools/terminal-window build/bubbletea/omarchy-bubbletea-gallery"
  },
  "tokens": "core",
  "icons": "none",
  "tests": [
    { "name": "behavior", "run": "..." },
    { "name": "live switch", "run": "tools/live/run ..." },
    { "name": "live switch, reduced motion", "run": "tools/live/run ...", "env": { "OMARCHY_UI_REDUCE_MOTION": "1" } }
  ]
}
```

- `{width}` × `{height}` are columns × rows. Ratatui (`"reference": true` in the terminal family) is called with 96 × 0 and sizes itself; every other terminal binding gets exactly its size and must pass `omarchy-ansi check --size`.
- `bin/review` renders all terminal snapshots with `omarchy-ansi png` and reports, per section, the cell diff and the pixel diff against Ratatui; the page shows Qt beside the terminal bindings for eyeballing and overlays, swipes and diffs only within the family.
- Tokens come from the core (the C library, or `omarchy-theme --watch --state` for one JSON line per change); never a resolver of the binding's own.
- The live-switch test is `spec/following.md`'s with the terminal's checks: the background through the public API, the changed hook once with the new tokens, revision 1, a fade at 24-bit that landed exactly (and the frame's painted colors the new ones), none under reduced motion.
- Interactive window: `--section NAME` starts on a section; `[` / `]` change section, Tab / Shift+Tab walk the focusable controls, the focused one takes its keys, mouse clicks and hover reach the components, `q` quits.
