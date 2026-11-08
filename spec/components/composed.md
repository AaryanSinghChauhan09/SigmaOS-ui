# Composed

SearchList, the last Tier 1 pattern, and the `composed` gallery section: real UIs built only from kit components and tokens. If an agent can build a shell panel, an overlay and an app inspector from the kit alone, the kit is complete enough; where it can't, the gap is listed at the end. Token names refer to `spec/tokens.md`; `space(n)` is `Style.space(n)` / `Theme::space(n)`.

| Component | Qt (`import Omarchy.Ui`) | Rune (`omarchy_rune::ui`) | Source |
|---|---|---|---|
| SearchList | `SearchList` | `SearchList`, `SearchItem` | own, on the harvested FilterInput, ListRow, CursorModel, ScrollArea and EmptyState (clipboard, emojis, menu, app grid) |

## SearchList

The type-ahead overlay the shell builds seven times: a filter line, the matching rows under a keyboard cursor, an optional detail pane beside them (master–detail, as the clipboard has), and an empty state when nothing matches.

**Anatomy.** On the `[menu]` surface (the clipboard's and the menu's): a card with the menu background, border and corners. In it, top to bottom:

1. **Filter line**: the query in `heading`, menu text, no caret (the line only grows at its end; Left and Right belong to the list). Qt draws it with the harvested `FilterInput`, so input-method text works; Rune with a bare `text_input` in the same type. While the query is empty, the placeholder in `textDim`.
2. **Body**, `md` below: a ScrollArea of ListRows (each item's glyph, bold title, dim subtitle, dim value), `xxs` apart, the cursor row in the hover look. With the detail pane on, then `xxl`, a 1px `track` rule the body's height, `xxl`, and the pane: the cursor item's title (`title`, bold, menu text, elided) over its detail (`body`, `textDim`, wrapped), `sm` apart, clipped to the body. A custom pane replaces both.
3. **Empty**: when there are no items, an overlay EmptyState centered in the body: the magnifier U+F0349 and "No matches for “query”", or the caller's empty text when the query itself is empty.

**Filtering is the caller's.** The list shows exactly the items it is given, so a caller can rank, fuzzy-match or search a database. `matches(text, query)` is the plain case-insensitive substring test, for callers with nothing better (it is the clipboard's filter).

**Keys** (the filter line holds focus; the list sees every key first):

| Key | Does |
|---|---|
| A printable key (CursorModel's `printable`) or Space | Typed into the query. So `j`, `k`, `h`, `l` and `x` are text here, not moves. |
| Backspace | Edits the query. |
| Up, Down, Page Up, Page Down, Home, End | Move the cursor (CursorModel), revealing its row. |
| Return, Enter | Activate the cursor row. |
| Delete | Ask to delete the cursor row. |
| Escape | Clear the query; when it is already empty, ask to close. |

A new query puts the cursor back on the first match. The pointer moves the cursor through CursorModel's gate (rows sliding under a still pointer don't steal it); a click activates.

**API.**

| Qt | Rune | |
|---|---|---|
| `query` | `new(key, &query_edit, &cursor)` | The query text; Rune keeps it in a `TextEdit` |
| `items: [{ title, subtitle, glyph, value, detail }]` | `.items([SearchItem::new(t).subtitle(s).glyph(g).value(v).detail(d)])` | Already filtered |
| `placeholderText` (default `common.search`) | `.placeholder(p)` | |
| `showDetail`, `detail: Component`, `detailWidth` | `.detail(b)`, `.detail_el(El)`, `.detail_width(w)` | Pane default 45% of the body |
| `emptyGlyph`, `emptyText`, `noMatchesText` (`%1` is the query) | `.empty(glyph, text)`, `.no_matches(t)` (`{}` is the query) | |
| `cursor` (its CursorModel), `cursorIndex`, `currentItem`, `filterInput` | `SearchList::cursor()` makes the model (one section, `SEARCH_SECTION`, showing) | Rune keeps the model in app state |
| `matches(text, query)` | `SearchList::matches(text, query)` | |
| width, height (implicit `space(480)` × `space(360)`) | `.size(w, h)` (same default) | |
| | `.on_state(|s| (&mut s.query, &mut s.cursor))` | Where typing and the keys land |
| `activated(index)`, `deleteRequested(index)`, `closeRequested()` | `.on_activate(f(s, i))`, `.on_delete(f(s, i))`, `.on_close(f(s))` | |

Rows are keyed `<key>-row-<i>` and the filter line `<key>-filter` in Rune.

**States.** Rows as ListRow: the cursor row takes `controls.hover` fill and border; the rest are bare. The cursor shows from the start (the shell's overlays open with it on the first row). No items: the body is the empty state, and the rule and pane hide.

**Metrics.** Card padding `panelPadding` inside the menu border (`max(1, space(2))` by default), corners `radius`. Filter line `max(space(34), title + 2 × controlPaddingY)` tall, text vertically centered. Body `md` below it, filling the rest. Rows `xxs` apart. Detail: `xxl`, 1px rule, `xxl`, pane; pane lines `sm` apart.

## Gallery

### `composed`

Built only from kit components and tokens; the gallery adds nothing but layout (rows, columns, fixed widths). Two rows, `huge × 2` apart within each row and top-aligned:

1. **A Wi-Fi panel**: a Card 360 wide holding, `panelGap` apart:
   - Hero: glyph U+F0928, "Wi-Fi", status "Connected to Basecamp", trailing ToggleSwitch on.
   - SectionHeader "Networks".
   - A ScrollArea (max 280) of ListRows `xxs` apart:
     1. Basecamp: glyph U+F0928, status "Connected" in success, current.
     2. Basecamp Guest: glyph U+F0925, subtitle "Open", under the cursor, its forget action U+F0159 (urgent) revealed.
     3. Coffee Shop: glyph U+F0922, subtitle "WPA2", trailing glyph the lock U+F033E, expanded: a PasswordField (placeholder "Password") filling the row and a bordered Button "Connect", `md` apart.
     4. Library: glyph U+F091F, subtitle "Saved", the lock U+F033E with the forget action U+F0159 (urgent) in its place under the cursor.
   - A Separator.
   - KeyValue, one pair to a line: IP 192.168.1.24 · Gateway 192.168.1.1 · DNS 1.1.1.1.

   **An app inspector**: a Pane ([`surfaces.md`](surfaces.md)) 300 wide with its divider on the left, as a side panel ruled off from the canvas. Its header: title "Recording 1", subtitle "1920 × 1080 · 0:42", actions IconButtons U+F0E2 undo and U+F01E redo and a bordered Button "Export". Then, `panelGap` apart:
   - Tabs Shape, Cursor, Zoom with Shape selected.
   - SectionHeader "Frame", then SettingRows: "Radius" with a 110-wide Slider at 0.4; "Border" with a 110-wide Dropdown (no label) = Solid (Solid, None); "Shadow" under the cursor with a ToggleSwitch on.
   - SectionHeader "Background", then SettingRows: "Fill" with a ColorWell in accent; "Padding" with a NumberField (no label) 24 (0–96).
2. **A SearchList** 360 × 300 with "term" typed, the detail pane on, and four results, the second under the cursor (so the pane shows "Terminal font" / "Pick the monospace font every terminal uses."):
   1. Terminal: glyph U+F120, subtitle "App", detail "Open a new terminal window."
   2. Terminal font: glyph U+F031, subtitle "Style", detail "Pick the monospace font every terminal uses."
   3. Terminal theme: glyph U+F1FC, subtitle "Style", detail "Match the terminal colors to the desktop theme."
   4. Intermediate CA: glyph U+F0A3, subtitle "Certificate", detail "Trust a certificate authority for this machine."

   **A SearchList** 300 × 200 with "zzz" typed over the same items, filtered with `matches`: the empty state "No matches for “zzz”".

## What the composition showed

Fixed in the primitives while building it (both bindings):

- **ListRow** had no way to open: the network panel's password row hangs a field and a button under the row. Added `expansion`.
- **ValueSlider** (Qt) drew a hard-coded `#333` track and `#101315` knob ring without the shell's `bar` object. It now defaults to the theme, as the Rune Slider does.
- **PasswordField**'s placeholder was TextField's `darker(1.6)`, which is stronger than the text in a light theme. It is `textFaint` now, as TextArea's. The harvested TextField, Dropdown, MultiSelect and the rest followed (`qt/patches/0003-semantic-text-roles.patch`): every placeholder is `textFaint` and every hand-darkened label `textDim`, the theme's own `muted` and `dark_foreground`.
- **A toolbar or pane title.** A centered "notes.md" or an inspector heading needed an ad-hoc `Text`; SectionHeader is a caption. Added **Heading** ([`surfaces.md`](surfaces.md)): title, heading and display levels, a dim subtitle, a trailing slot, centered for a toolbar. The `navigation` toolbar's title and the inspector's header use it.
- **A pane surface.** Card is the popups surface (accent border), so the inspector was a Card. Added **Pane** ([`surfaces.md`](surfaces.md)): flat on the window, with a one-side hairline rule or the controls' normal glass, and a Heading header with actions. The inspector is a Pane now.
- **A persistent trailing glyph on ListRow.** The network panel shows a lock that turns into forget under the cursor; the kit had the revealed action only, so the lock was passed as `value` text (`bodySmall`, `textDim`). Added `trailingGlyph` / `.trailing_glyph(g, tone)` ([`lists.md`](lists.md)): a `subtitle` glyph in the end slot at rest, replaced by the action while it shows. The Wi-Fi panel's locks use it.
- **ScrollArea's reserved gutter** made a short list `scrollbarThickness + space(4)` narrower than its siblings (the Hero, the Separator), so row fills stopped short of the panel's content edge. The bar now overlays the content's right edge, as the shell's `AsNeeded` bars do: nothing is reserved and nothing reflows when it appears.
- **App icons by name.** Avatar took an image file only. Added `iconName` / `.icon(name)`: a freedesktop icon name resolved through the theme's icon theme (`icons.theme`, its Inherits chain, hicolor, pixmaps) by the same lookup in both bindings, following theme switches ([`lists.md`](lists.md), Avatar).

Still missing:

- **`common.noMatchesFor`** is not in the harvested strings (`bin/harvest-qt` carries `common.noMatches` only), so Qt falls back to the English "No matches for “%1”".
