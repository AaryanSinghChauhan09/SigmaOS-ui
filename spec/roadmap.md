# Primitives roadmap

What the kit should grow next, ranked by how often real Omarchy UI builds it by hand. Evidence comes from two surveys (2026-11-08):

- **Shell plugins**: every plugin in `~/omarchy/shell/plugins` on `quattro`, plus third-party plugins (Windows 98/XP themes, `ryan.bar`, `ryan.menu`).
- **Apps**: hype, omareel and omasnap from `~/Work`, plus rune-kit's inventory.

Counts are copies built by hand, not uses of a kit component.

## What the surveys say

- **The shell's kit covers controls; plugins hand-build everything around them.** Lists, rows, scrolling, empty states, meters, spinners and keyboard cursors are rebuilt in nearly every panel.
- **Kit controls go unused.** ButtonGroup, Dropdown, SearchableDropdown, NumberField and Toggle appear only in the dev gallery. Plugins write `Button { bordered; selected }` rows and hand-made rows instead.
- **There is no dim-text token.** `Qt.darker(foreground, 1.5)` appears 48 times, 1.4 appears 27 times and 1.55 appears 13 times. Nine panels redeclare their own `dim` and `hoverFill`.
- **Keyboard navigation is copy-pasted.** Every panel reimplements `cursorActive`, `focusSection`, `selectedIndex`, `moveCursor` and `ensureCursorVisible`. Every overlay reimplements arrows, Page Up/Down, Home/End and the printable-key filter for type-ahead. j/k is handled three different ways.
- **Apps need app chrome the shell never had**: menus with shortcut hints, toolbars, keycaps and shortcut sheets, modals with a scrim, toasts, setting rows, color wells and split panes.
  - omareel already has a tokenized `Om*` kit for these (`~/Work/omareel/src/ui/qml/ui`). It is the best source for the app-side Qt components.
  - hype only has color tokens; it hard-codes 58 font sizes and repeats a scrim color 6 times. omasnap has no theme at all.
- **Theme plugins fork whole files** (the XP and `ryan.menu` menus are byte-identical, and the bars differ by 3 lines). Tokens can't fix that; composable primitives can.

## Tokens to add first

These go in `spec/tokens.md`, then both resolvers, keeping `bin/test` green.

| Token | Value | Why |
|---|---|---|
| `text.dim` | foreground darker 1.5 (what the shell writes by hand; the theme's `dark_foreground` is dimmer, e.g. `#565f89` against `#71768f` in tokyo-night) | 48 copies. Secondary lines, subtitles, values. |
| `text.faint` | foreground darker 2.0 | Disabled and placeholder text, already `Qt.darker(fg, 2.0)` in the kit. |
| `success`, `warning` | the theme's `green`, `yellow` (falling back to `color2`, `color3`), as `urgent` already falls back from `red` to `color1` | Status dots, meters, banners. Today only `urgent` exists, so "connected" and "low battery" have no color. |
| `track` | foreground at 0.12 | Meter, progress and slider tracks, today ad hoc rgba at 0.12–0.45. |
| `scrim` | `menu.scrim` (background at 0.5) | Modals. hype repeats `#660b0d14` six times. |
| `meter.thickness`, `scrollbar.thickness` | `space(6)`, `space(6)` | OSD uses 6, power 8, agents `controlHeight × 0.14`. |
| `row.height`, `row.iconSize` | `controlHeight × 1.6`, `font.title` | One list-row rhythm instead of 16. |

## Tier 1: what plugins rebuild most

For each binding, the plan says where the component comes from: **harvest** from the shell, **own** (written in this repo, a candidate to send upstream to the shell), **port** from omareel's `Om*` kit, **wrap** a rune-kit component restyled through `Kit`, or **draw** it in `omarchy-rune`.

| # | Component | Evidence | What it is | Qt | Rune |
|---|---|---|---|---|---|
| 1 | **ListRow** | ~16 row components (bluetooth, network, tailscale ×6, audio, clipboard, XP menu) | Leading glyph or Avatar; title plus dim subtitle; trailing slot (value text, Badge, switch, or an action revealed on hover). States: cursor, current (accent), selected, disabled, busy. | own, on CursorSurface | draw |
| 2 | **CursorModel** | Every panel and overlay | Keyboard cursor over sections and indexes: `move`, `moveSection`, `activate`, `ensureVisible`. j/k/arrows/PgUp/PgDn/Home/End handled the same way everywhere. Hover moves the cursor through PointerMoveGate. Includes a type-ahead key filter. | own (QtObject plus attached helpers, built on PanelKeyCatcher) | own struct plus `on_key` helper |
| 3 | **ScrollArea** | 12, all with an unstyled ScrollBar | Capped max height, themed thin scrollbar, `ensureVisible(item)`, stop at bounds. | own | wrap `scroll` plus rune-kit `ScrollBar` |
| 4 | **EmptyState** | ~14 | Glyph, title, optional hint and action. Two sizes: panel (dim body text) and overlay (`displayLarge` glyph). | own (port OmEmptyState) | draw |
| 5 | **Meter** / **ProgressBar** | 5 meters plus progress in 2 apps | Determinate fill on a `track`. Optional markers. `alarming` turns it urgent or warning. Indeterminate sweep for progress. | own | wrap `Progress` |
| 6 | **Spinner** | 6 (spinning glyph or opacity pulse) | Busy glyph or pulse. Still under reduced motion. Also the `busy` state of Button, ToggleSwitch and ListRow. | own | wrap `Spinner` |
| 7 | **KeyValue** | 3 identical copies (power, dropbox, network) | Dim label, value, aligned in a column grid. | own | draw |
| 8 | **Hero** | 6 hand-made plus 3 PanelHero | Big glyph, bold title, dim status (urgent on error), trailing control. | harvest (`PanelHero`) | draw |
| 9 | **SearchList** | 7 type-ahead overlays (clipboard, emojis, reminders, app grid, menus) | FilterInput header, results list of ListRows, optional detail pane, empty state, type-ahead CursorModel. | own, on FilterInput | draw, on rune-kit `List` |
| 10 | **Badge** / **StatusDot** | 4, sizes hard-coded | Count or label pill; a dot with a surface ring, docked on an icon's corner. | own | wrap `Badge` |
| 11 | **Avatar** / **AppIcon** | 5 fallback chains | Sources tried in order (image, icon theme, desktop entry, initial on a disc), DPR-sized, masked by radius. | own, on SizedImage | draw |
| 12 | **TileGrid** | 3 (emojis, app grid, agents) | Grid of cells with a cursor highlight and a 2-line elided label. | own | draw, on `virtual_rows` |
| 13 | **PasswordField** | 2 (lock, polkit) | TextField with secure echo, a leading lock glyph, and an error flash and shake. | own (TextField variant) | `TextField::secure` plus error state |

## Tier 2: app chrome

| # | Component | Evidence | What it is | Qt | Rune |
|---|---|---|---|---|---|
| 14 | **Menu** / **ContextMenu** | All 3 apps; 4 shell copies | MenuRows with icon, check/radio, shortcut hint, submenu chevron, separators. Opens at the pointer or an anchor. | port (OmMenu, OmMenuItem) | wrap `Menu`, `context_menu` |
| 15 | **Keycap** / **ShortcutSheet** | All 3 apps | A key cap; a grouped sheet of keys and descriptions. | port (OmKeycap) | wrap `Keybind`; draw the sheet |
| 16 | **Tooltip with shortcut** | All 3 apps | The tooltip plus a trailing Keycap. | extend PanelToolTip | extend Tooltip |
| 17 | **Dialog** | All 3 apps (6 popups in hype) | Modal on `scrim`, title, body slot, button row. Escape and Enter. ConfirmDialog becomes a preset of it. | own | wrap `Dialog` |
| 18 | **Toolbar** | All 3 apps | Groups of IconButtons with dividers; leading, center and trailing slots. | own | wrap shell `Toolbar` |
| 19 | **Tabs** / **IconRail** | All 3 apps | Text tabs with a sliding underline; vertical icon rail. ButtonGroup stays for small choices. | own (OmButtonGroup glide) | wrap `SegmentedControl` and `TabStrip` |
| 20 | **SettingRow** | omareel inspectors; shell Toggle | Label and description on the left, any control on the right. Toggle becomes SettingRow plus ToggleSwitch. | port (OmSettingRow) | draw |
| 21 | **Toast** / **Banner** | All 3 apps (flash lines, notices, pills) | Transient in-app message, and an inline info/warning/error banner. Desktop notifications go through `omarchy-notification-send` instead. | own | wrap `Toast`; draw Banner |
| 22 | **Checkbox** / **Radio** / **MultiSelect** | MultiSelect in the shell; omareel exports | Check and radio rows, then the shell's MultiSelect without its command-backed loading. | harvest MultiSelect plus a patch | wrap `Checkbox`, `RadioGroup` |
| 23 | **ColorWell** / **SwatchStrip** | All 3 apps | A swatch that opens a picker; a strip of theme or preset colors. | port (OmColorWell) | draw |
| 24 | **SplitView** / **Sidebar** | hype, omareel | Resizable panes, collapsible sidebar. | own (QtQuick SplitView, themed handle) | wrap `SplitView`, `Sidebar` |
| 25 | **TextArea** | apps | Multi-line TextField. | own | wrap `TextArea` |

## Tier 3: later

These are specialized, used by one app each, or already provided by rune-kit:

- Timeline, scrubber and trim/crop handles (omareel, omasnap)
- HSV color picker (omasnap)
- Table and Tree (rune-kit has them)
- Date and calendar grid (shell clock)
- Drag-reorder handles (bar, dock)
- Classic bevel surfaces (Win98 themes): a theme concern; tokens alone can't cover it

## Galleries

New `spec/gallery.md` sections, so `bin/review` covers everything:

- `lists`: ListRow states, KeyValue, Badge, StatusDot, Avatar
- `feedback`: Meter, ProgressBar, Spinner, EmptyState, Banner, Toast
- `navigation`: Tabs, IconRail, Toolbar, Menu, Keycap, ShortcutSheet
- `forms`: SettingRow, Checkbox, Radio, PasswordField, TextArea, MultiSelect, ColorWell
- `composed`: the dev gallery's Wi-Fi panel and a SearchList overlay, built only from kit parts

The `composed` section is the real test: if an agent can build a shell panel or an app inspector from these alone, the kit is complete enough.

## Upstream

Most Tier 1 components belong in the shell too: its 16 hand-built rows and 12 scroll areas would shrink to kit uses. Build them here first, with both bindings and the review. Then offer the Qt versions to `shell/Ui` so they become harvested rather than owned.
