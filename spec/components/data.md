# Data and layout components

Tables, trees, split panes, sidebars and the month grid. Roadmap Tier 2 #24 (SplitView / Sidebar) and Tier 3 (Table, Tree, calendar grid). Both bindings draw them; none wraps rune-kit, because rune-kit's `Table`, `Tree`, `shell::SplitView` and `Sidebar` carry their own metrics and chrome. Token names refer to `spec/tokens.md`.

| Component | Qt (`import Omarchy.Ui`) | Rune (`omarchy_rune::ui`) | What it is |
|---|---|---|---|
| Table | `Table` | `Table`, `Column` | Header of dim bold captions with a sort chevron, rows of elided text, hairline rules, one selected and one cursor row. |
| Tree | `Tree` | `Tree`, `TreeNode` | FileTree-style rows: indent per level, chevron on branches, optional glyph, label. |
| SplitView | `SplitView` (wraps QtQuick.Controls `SplitView`) | `SplitView`, `SplitPane` | Panes with a draggable handle: a hairline that lights with the hover glass. |
| Sidebar | `Sidebar` | `Sidebar`, `SidebarSection`, `SidebarItem` | Sections of SectionHeaders over glyph + label rows, one selected; collapses to a glyph rail. |
| Calendar | `Calendar` | `Calendar`, `Date` | Month grid from Monday, today ringed in accent, selected day filled, previous/next IconButtons. |

## Shared rules

- **Rows** (Table, Tree, Sidebar, Calendar days) use one state ladder: the hover fill under the pointer, keyboard focus or the cursor flag; else the selected fill when selected; else nothing. Hover replaces selected (it does not stack). Selected text is bold in `controls.selected.text` (the selected-state color, foreground by default). Fills are `radius`-rounded, like MenuRow.
- **The app owns the state.** Selection, cursor, sort, expanded nodes, sizes and the shown month are passed in; the component asks for changes through signals (Qt) or `on_*` handlers (Rune). The Qt components also update `cursor*` themselves from the pointer and keys, as the shell's `CursorSurface` contract asks (one highlight on screen).
- **Keyboard**: every component is a focus scope. Qt: arrows move the cursor, Return/Space choose, as listed per component. Rune: rows are focusable inside a `focus_group` (`grid`, `tree`, `list`), so arrows move focus and Return/Space press.
- **Cursor flags**: Qt `hasCursor: true` paints the hover look on the `cursor*` row without focus; Rune `.cursor(Some(..))` forces it on one row.
- **Keys** (Rune): `.key(prefix)` keys the parts for tests (`rt.find`) as listed per component.

## Table

**Anatomy.** Header row: one cell per column, the title in `caption` bold `textDim`, cut with "…"; the sorted column adds a chevron (`󰅃` ascending, `󰅀` descending, `caption`, `textDim`) `xs` after the title (before it in a right-aligned column, so the title stays flush with the numbers). A `hairline` rule in `track` under the header. Body rows: one cell per column, `body` text in the foreground, cut with "…", left- or right-aligned per column; a `hairline` `track` rule under every row but the last when `separators`.

**Metrics.** Header and rows `popupRowHeight` (28) tall, rules drawn inside the row's bottom edge. Cells padded `controlPaddingX` (10) each side. Fixed columns take their width; the rest share `floor((width − fixed) / flexCount)`.

**States.** Row: rest, selected (selected fill, bold, selected text color), cursor (hover fill; bold stays if also selected).

| | Qt | Rune |
|---|---|---|
| Columns | `columns: [{ title, width?, align?: "right" }]` | `Table::new([Column::new("Size").width(70.0).right(), ..])` |
| Rows | `rows: [["a", "b", ..], ..]` | `.rows(rows)` (any iterables of strings) |
| Sort shown | `sortColumn: int` (−1 none), `sortAscending: bool` | `.sort(column, ascending)` |
| Selected | `selectedRow: int` (−1 none) | `.selected(Option<usize>)` |
| Cursor | `cursorRow: int`, `hasCursor: bool` | `.cursor(Option<usize>)` |
| Rules | `separators: bool` (true) | `.separators(bool)` (true) |
| Width | the item's width | `.width(w)` (else the parent's; flex columns grow) |
| Choose | `rowActivated(int row)` (click, Return/Space; also sets `selectedRow`) | `.on_select(\|s, row\|)` |
| Sort asked | `sortRequested(int column)` (header click) | `.on_sort(\|s, column\|)` |
| Keys | Up/Down/Home/End/PageUp/PageDown move `cursorRow` | `.key(p)`: rows `p-<row>`, headers `p-col-<column>` |

The Qt body is a `ListView`: give the Table a height and the rows scroll under a fixed header.

## Tree

**Anatomy.** Per visible row (depth first, children shown only for expanded branches): an inset of `md` + depth × `huge`; a chevron slot `xxxl` wide holding `󰅂` (collapsed) or `󰅀` (expanded) in `iconSmall` `textDim`, centered on whole pixels (empty on leaves); `sm` gap; the glyph at `icon` size in `textDim` (selected text color when selected) in a slot `icon + md` wide; the label in `body`, cut with "…", `md` from the right edge.

**Metrics.** Rows `popupRowHeight` (28).

**States.** As Table rows. Branches report `expanded` to accessibility (Rune).

| | Qt | Rune |
|---|---|---|
| Nodes | `nodes: [{ id, label, glyph?, children? }]` | `Tree::new([TreeNode::new(id, label).glyph(g).children([..])])` |
| Expanded | `expanded: [id, ..]` | `.expanded(ids)` |
| Selected | `selectedId: string` | `.selected(Option<&str>)` |
| Cursor | `cursorId: string`, `hasCursor` | `.cursor(Option<&str>)` |
| Width | the item's | `.width(w)` (else the parent's) |
| Fold asked | `toggled(string id)` (chevron click, double click, Right on a closed branch, Left on an open one) | `.on_toggle(\|s, id\|)` (the same: chevron press, double press, Right and Left on a focused branch row) |
| Choose | `activated(string id)` (click, Return/Space) | `.on_select(\|s, id\|)` |
| Keys | Up/Down move `cursorId` | `.key(p)`: rows `p-<id>`, chevrons `p-<id>-toggle` |

## SplitView

**Anatomy.** Panes in a row (or a column when vertical) with a handle between each visible pair. The handle is a strip `md` (6) thick holding a `hairline` line in `track` at `floor((md − hairline) / 2)`; under the pointer, while dragged, or with the cursor flag, the strip fills with the hover fill at `min(4, radius)` (the separator line stays drawn over it). Handles show the resize-column (resize-row) pointer.

**Behavior.** Dragging a handle resizes the sized pane beside it (the leading one first), clamped to its minimum and maximum. A hidden pane takes its handle with it (collapsing a side).

| | Qt | Rune |
|---|---|---|
| Panes | children, sized with `QQC.SplitView.preferredWidth`, `minimumWidth`, `maximumWidth`, `fillWidth` (import `QtQuick.Controls as QQC`) | `.pane(SplitPane::new(el).size(200.0).min(120.0).max(..))`, `SplitPane::new(el)` fills |
| Collapse | the pane's `visible: false` | `SplitPane::visible(false)` |
| Direction | `orientation` | `.vertical(bool)` |
| Hot look | `hasCursor: bool` | `.cursor(bool)` |
| Resize | Qt's own (the attached `preferredWidth` changes); `saveState()`/`restoreState()` | `.on_resize(\|s, pane_index, size\|)`; the app stores the size |
| Keys | — | `.key(p)`: handles `p-handle-<n>` from 0 |

Qt: `handleThickness` (= `md`).

## Sidebar

**Anatomy.** A column padded `sm`; sections `md` apart. A section: a SectionHeader (`caption` bold, foreground darker 1.4), inset `controlPaddingX` with `xxs` below it, then its rows. A row, `popupRowHeight` tall, inset `controlPaddingX` both sides: the glyph at `icon` size in `textDim` (selected text color when selected), then the label `icon + controlGap` from the glyph's start, in `body`, cut with "…".

**Collapsed.** Headers and labels hide; the bar is `sm × 2 + controlPaddingX × 2 + icon` wide (Qt `railWidth`, Rune `Sidebar::rail_width(t)`), a rail of glyph rows.

**States.** As Table rows.

| | Qt | Rune |
|---|---|---|
| Sections | `sections: [{ title, items: [{ id, label, glyph }] }]` | `Sidebar::new([SidebarSection::new(title, [SidebarItem::new(id, label, glyph)])])` |
| Selected | `selectedId: string` | `.selected(Option<&str>)` |
| Cursor | `cursorId: string`, `hasCursor` | `.cursor(Option<&str>)` |
| Collapsed | `collapsed: bool` | `.collapsed(bool)` |
| Width | `implicitWidth` `space(200)` expanded | `.width(w)` (else the parent's) |
| Choose | `activated(string id)` (click, Return/Space) | `.on_select(\|s, id\|)` |
| Keys | Up/Down walk every row across sections | `.key(p)`: rows `p-<id>` |

## Calendar

**Anatomy.** A column `sm` gap, as wide as the grid:

1. Header, as tall as an IconButton: previous (`󰅁`) at the left, next (`󰅂`) at the right (IconButtons), and between them the month and year in capitals, `body`, foreground darker 1.4, centered across the grid.
2. Weekday row, `space(16)` tall: `MO TU WE TH FR SA SU` in `caption` bold `textDim`, centered per column. Weeks start on Monday.
3. Six week rows (always six, so the height never changes), from the Monday on or before the 1st. A day cell is `space(36)` × `controlHeight`, cells `xxs` apart in both directions, `radius`-rounded; the number in `body`, centered.

**States (day).** In month: foreground; weekend columns foreground darker 1.45 (as the shell's clock). Outside the month: `textFaint`. Today: a `hairline` ring in the accent, bold. Selected: the selected fill, bold, selected text color. Cursor: the hover fill (keeps today's ring).

| | Qt | Rune |
|---|---|---|
| Month shown | `year: int`, `month: int` (1–12) | `Calendar::new(year, month)` |
| Today | `today: "YYYY-MM-DD"` (the app passes it; nothing reads the clock) | `.today(Date::new(y, m, d))` |
| Selected | `selected: "YYYY-MM-DD"` | `.selected(Option<Date>)` |
| Cursor | `cursorDate: "YYYY-MM-DD"`, `hasCursor` | `.cursor(Option<Date>)` |
| Labels | `monthNames`, `weekdayNames` (English by default) | English |
| Pick | `picked(string date)` (click, Return/Space; also sets `selected`) | `.on_pick(\|s, Date\|)` |
| Step month | the buttons and PageUp/PageDown change `year`/`month` | `.on_month(\|s, year, month\|)` |
| Keys | arrows move `cursorDate` by a day or week, following it into the next month | `.key(p)`: days `p-YYYY-MM-DD`, steppers `p-prev`, `p-next` |

Rune's `Date` has `weekday()` (0 = Monday), `add_days(n)`, `add_months(n)` and `days_in_month(y, m)`.

## Gallery

| Section | Specimens |
|---|---|
| `data` | Side by side, top-aligned, `huge` apart. A Table 480 wide with columns Name (flex), Kind (90), Size (70, right-aligned), Modified (100); sorted by Name ascending; rows `assets / Folder / — / 2026-11-02`, `Cargo.toml / TOML / 1.2 KB / 2026-11-07`, `README.md / Markdown / 4.8 KB / 2026-11-05`, `screenshot-of-the-new-settings-panel.png / PNG image / 1.4 MB / 2026-11-08` (elided), `src / Folder / — / 2026-11-08`, `theme.toml / TOML / 812 B / 2026-10-30`; row 2 (Cargo.toml) selected, row 4 (the screenshot) under the cursor. A Tree 220 wide: `omarchy-ui` (open, `󰝰`) › `qt` (open) › `Button.qml`, `Table.qml` (selected); `rune` (collapsed, `󰉋`); `spec` (open) › `tokens.md`; `README.md` (cursor). Files use `󰈙`. 8 visible rows, 3 levels. |
| `layout` | A SplitView 720 × 220 framed by a `hairline` `track` border at `radius`: a Sidebar 200 wide (min 120) with sections Library (All files `󰉋`, Recent `󰋚` selected, Starred `󰓎`, Trash `󰩺`) and Places (Documents `󰈙`, Downloads `󰇚`); the handle at rest; a content pane with "No file selected" centered in `body` `textFaint`. Below it, a Calendar for November 2026, today the 8th, the 14th selected. |
