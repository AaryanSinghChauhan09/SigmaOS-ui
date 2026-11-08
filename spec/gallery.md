# Gallery sections

Every binding ships a gallery with these sections, in this order, with these specimens, so `bin/review` can put the same section from each framework side by side. A gallery renders one section with `--section <name>` (Qt) or `--scene <name>` (Rune), at 760 points wide, at 2× for snapshots.

| Section | Specimens |
|---|---|
| `buttons` | Plain; Bordered; Bordered + cursor ("Hover"); Bordered + selected; Bordered with icon `` ("Icon"); IconButton `` bordered; IconButton `` bordered + cursor. Then ButtonGroup Left / Center / Right with Center selected. |
| `toggles` | Four ToggleSwitches: off, on, off + cursor, on + cursor. Toggle "Wi-Fi" / "Connected to Basecamp", on, 360 wide. Toggle "Bluetooth" / "Off", off, cursor, 360 wide. |
| `inputs` | Three TextFields 220 wide: placeholder "Search…"; text "Omarchy"; text "Hovered" with cursor. Then NumberField "Gaps" 10 (0–40) and Dropdown "Theme" = Tokyo Night. |
| `slider` | Slider at 0.35, 360 wide. Slider at 0.75 with 5 notches, 360 wide. |
| `surfaces` | Card "Network" 300 wide: Toggle "Wi-Fi" / "Basecamp" on, Separator, MenuRows Basecamp (checked), Basecamp Guest (highlighted), Coffee Shop. Beside it: Tooltip "Restart Wi-Fi", and a ConfirmDialog "Remove the Coffee Shop network?" centered in a 340 × 170 box. |
| `surfaces` (cont.) | Then Panes and Headings: see the Gallery part of [`components/surfaces.md`](components/surfaces.md). |
| `tokens` | Swatches (64 × 32) for foreground, background, accent, urgent, muted. State chips (120 × controlHeight) normal, hover-cursor, focus, selected. The type scale from displayLarge to caption. |
| `lists`, `tiles` | See the Gallery part of [`components/lists.md`](components/lists.md). |
| `progress`, `messages` | See [`components/feedback.md`](components/feedback.md). |
| `menus`, `navigation` | See [`components/navigation.md`](components/navigation.md). |
| `forms`, `pickers` | See [`components/forms.md`](components/forms.md). |
| `data`, `layout` | See [`components/data.md`](components/data.md). |
| `timeline`, `color` | See [`components/editing.md`](components/editing.md). |
| `composed` | See [`components/composed.md`](components/composed.md). |

The order is the galleries' order: `qt/gallery/Sections.qml` and the `gallery!` list in `rune/gallery/src/main.rs`, which `tools/review.py` reads.

Adding a component means adding a specimen here and to every gallery, so the review covers it.
