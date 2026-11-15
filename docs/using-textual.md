# Using Omarchy UI from Textual

> Paths such as `examples/...` are relative to a checkout.

`omarchy-textual` (`tui/textual`, import `omarchy_textual`) gives a [Textual](https://textual.textualize.io) app Omarchy's look with one call: the live theme, followed without a restart, colors that fade on a switch, Textual's stock widgets restyled to the kit's one-row forms, and the kit's components (`Button`, `Toggle`, `ListRow`, `Menu`, `Table`, …) as Textual widgets that draw exactly the cells [`spec/terminal.md`](../spec/terminal.md) gives them, the same cells as the reference, Ratatui. [`using-terminal.md`](using-terminal.md) has what every terminal binding shares.

## Adopt it

With [uv](https://docs.astral.sh/uv/), in the app's `pyproject.toml`:

```toml
[project]
dependencies = ["omarchy-textual"]

[tool.uv.sources]
omarchy-textual = { path = "../omarchy-ui/tui/textual", editable = true }   # or
omarchy-textual = { git = "https://github.com/omacom/omarchy-ui", subdirectory = "tui/textual", rev = "<commit>" }
```

`omarchy-textual` pins what it is built against exactly (`textual==8.2.8`, `rich==15.0.0`, `wcwidth==0.9.4`, in `tui/textual/pyproject.toml` and its `uv.lock`; move them together); don't pin Textual or Rich in the app as well. The tokens come from the core's `omarchy-theme` CLI: `$OMARCHY_THEME_BIN`, else `omarchy-theme` on `PATH` (the `omarchy-ui` package installs it), else this checkout's `target/release/omarchy-theme` (`bin/build core`).

## The one call

[`examples/textual/wifi.py`](../examples/textual/wifi.py), a Wi-Fi panel:

```python
from textual.app import ComposeResult
from omarchy_textual import OmarchyApp, OmarchyThemeChanged, Tone
from omarchy_textual.snap import from_args
from omarchy_textual.widgets import Card, Hero, ListRow, ScrollArea, SectionHeader, ToggleSwitch

class Wifi(OmarchyApp):                       # instead of App
    def compose(self) -> ComposeResult:
        with Card(width=48):
            yield Hero("\U000f0928", "Wi-Fi", status="Connected to Basecamp", trailing=ToggleSwitch(True), width=42)
            yield SectionHeader("Networks")
            with ScrollArea(width=42, max_rows=8, id="networks"):
                yield ListRow("Basecamp", glyph="\U000f0928", status=("Connected", Tone.SUCCESS), current=True, width=42)
                yield ListRow("Coffee Shop", glyph="\U000f0928", subtitle="WPA2", width=42)

    def on_scroll_area_activated(self, event: ScrollArea.Activated) -> None:
        ...                                   # Return or a click on a row

    def on_omarchy_theme_changed(self, event: OmarchyThemeChanged) -> None:
        ...                                   # the changed hook: event.tokens, event.revision

if __name__ == "__main__":
    app = Wifi()
    if not from_args(app):                    # --snap out.ans [--size COLSxROWS]
        app.run(mouse=True)
```

Subclass `OmarchyApp` instead of `App`, or call `omarchy_textual.follow(app)` once in any app's `__init__`. Either:

- reads the tokens from the core (`omarchy-theme --watch --state`, an asyncio subprocess on the app's loop; the first line is the state the app starts with) and follows the desktop: theme switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads, the language;
- maps them onto a Textual `Theme` named `omarchy` (`$primary`, `$surface`, `$error`, … for stock widgets and your CSS) and the kit's `$omarchy-*` variables (below);
- restyles Textual's stock widgets (`Button`, `Input`, `TextArea`, `Switch`, `Checkbox`, `RadioButton`, `Select`, `OptionList`, `ListView`, `DataTable`, `Tree`, `Tabs`, `ProgressBar`, `Footer`, …) to the kit's one-row forms: thin `▏ ▕` edges, foreground glass fills, the hover look under the pointer and with the keys, accent only for what is chosen ([`omarchy_textual/stock.tcss`](../tui/textual/omarchy_textual/stock.tcss)); your own CSS still wins;
- writes the theme background as the terminal's default on every line Textual draws, so the terminal's background and opacity show, and below truecolor maps every color to the nearest xterm-256 index or the ANSI index of its theme role ([`spec/terminal.md`](../spec/terminal.md#color); `OMARCHY_UI_COLORS=24bit|256|16` overrides the detection);
- on a change: puts the new tokens in effect (the revision counts it), fades colors over `motion.theme` (280 ms) on `motion.ease` by redrawing at up to 60 frames a second, in truecolor only (none under reduced motion, at 256 or 16 colors, or on the first frame), and posts `OmarchyThemeChanged` to the app, once.

`app.kit` (or `omarchy_textual.kit_of(app)`) is the kit on an app: `kit.theme.tokens` (in effect: after a change, the new ones), `kit.theme.shown` (on screen, mid-fade), `kit.revision` (0, plus one per change: a cache key), `kit.set_tokens(tokens)` (deliver a change by hand: previews, tests).

### The changed hook

`OmarchyThemeChanged(tokens, revision)` ([`spec/following.md`](../spec/following.md#the-changed-hook)) is posted to the app once per change that alters the tokens, after they are in effect, and handled on Textual's event loop like any message (`on_omarchy_theme_changed`, or `@on(OmarchyThemeChanged)`); never for the tokens the app started with. Use it for what the kit can't reach: a cache, a child process to re-theme. The kit's widgets, stock widgets and your TCSS follow by themselves.

## Components

Every component of [`spec/components.md`](../spec/components.md) under the same name in `omarchy_textual.widgets`, a Textual widget that draws the cells of [`spec/terminal.md`](../spec/terminal.md) (Textual's own `Button`, `Checkbox`, `Tabs`, `Tree` share names with some: import the kit's qualified, or from `omarchy_textual.widgets`). Each is focusable where the component takes keys (focus looks like hover: keyboard first), takes clicks and hover, keeps its state in reactives (`toggle.value = True`, `slider.value`, `tabs.current`), and posts messages the way Textual's widgets do, each with a `control`:

| Widgets | Messages |
|---|---|
| `Button`, `IconButton`, `ButtonGroup`, `ToggleSwitch`, `Toggle` | `Button.Pressed`, `ButtonGroup.Changed`, `ToggleSwitch.Changed` (`Toggle` too) |
| `TextField`, `PasswordField`, `TextArea`, `NumberField`, `Dropdown`, `Slider` | `TextField.Changed`, `.Submitted`; `TextArea.Changed`; `NumberField.Changed`; `Dropdown.Changed`; `Slider.Changed` |
| `Checkbox`, `Radio`, `RadioGroup`, `ColorWell`, `SwatchStrip`, `SettingRow`, `MultiSelect` | `Checkbox.Changed`, `RadioGroup.Changed`, `ColorWell.Pressed`, `SwatchStrip.Changed`, `MultiSelect.Changed` |
| `Card`, `Pane`, `Heading`, `SectionHeader`, `Separator`, `MenuRow`, `Tooltip`, `Dialog`, `ConfirmDialog`, `DialogScreen` | `MenuRow.Selected`, `Dialog.Chosen`; `DialogScreen` dismisses with the choice |
| `ListRow`, `ScrollArea`, `KeyValue`, `Hero`, `Badge`, `StatusDot`, `Avatar`, `TileGrid`, `SearchList` | `ListRow.Selected`, `.ActionPressed`; `ScrollArea.Activated`, `.Deleted`; `TileGrid.Selected`; `SearchList.Changed`, `.Activated`, `.Deleted`, `.Closed` |
| `Meter`, `ProgressBar`, `Spinner`, `EmptyState`, `Banner`, `Toast` | `EmptyState.ActionPressed`, `Banner.ActionPressed`, `.Dismissed`, `Toast.ActionPressed` |
| `Menu`, `ContextMenu`, `Keycap`, `KeyCombo`, `ShortcutSheet`, `Toolbar`, `ToolbarSeparator`, `Tabs`, `IconRail` | `Menu.Selected`, `Toolbar.Overflow`, `Tabs.Changed`, `IconRail.Changed` |
| `Table`, `Tree`, `SplitView`, `Sidebar`, `Calendar` | `Table.Highlighted`, `.Selected`; `Tree.Toggled`, `.Selected`; `SplitView.Resized`; `Sidebar.Selected`; `Calendar.Selected` |
| `Timeline`, `Scrubber`, `TrimHandle`, `CropFrame`, `DragHandle`, `DropIndicator`, `ColorPicker` | `Timeline.Seeked`, `.ClipSelected`; `Scrubber.Seeked`; `ColorPicker.Changed` |

- **Layout.** Inside the kit's containers (`Card`, `Pane`, and `Col(gap=…)`, `Row(gap=…)`, `Box(width=, height=, center=)`, `Stack` for one over another, `Spacer`) every kit widget sits at its natural size where spec/terminal.md puts it: write them like Textual's `Vertical` and `Horizontal` (`with Card(title="Network"): yield Toggle(…)`). Slots take widgets too: `ListRow(trailing=Badge("3", Tone.ACCENT, filled=True))`, `SettingRow("Strength", control=Slider(0.6, width=13))`, `Hero(trailing=ToggleSwitch(True))`, `Pane(actions=[IconButton(…)])`. Outside them (in a `Vertical`, a `Grid`, the screen), a kit widget sizes itself (`width: auto; height: auto`) and your TCSS places it.
- **How they draw.** The outermost kit widget of a tree paints the whole tree into one cell buffer, in order, as the reference paints a frame: a child's alpha composites over what its parent painted, a scrim darkens what is under it. Every widget in the tree is still a real Textual widget, placed where it was drawn: it takes focus, keys and clicks, and shows its part of the buffer.
- **Modals.** `self.push_screen(DialogScreen("Discard changes?", "…", labels=("Keep editing", "Discard"), destructive=True), callback)`: the scrim composites over the cells of the screen under it, exactly.
- **Your own drawing.** `Canvas(draw, width, height)` calls `draw(theme, area, buffer)` with the kit's painting (`omarchy_textual.cells.paint`: alpha compositing over what is painted, control edges, box corners by the radius, elision) and the theme's colors (`theme.fg()`, `theme.dim()`, `theme.accent()`, `theme.shown.controls.hover.fill`, …), read at every draw so they fade. `Cells(lambda: cells.Badge("3"))` shows any cell component. The cell components themselves (`omarchy_textual.cells`) work without Textual.
- **TCSS.** For your own widgets, the kit's variables, every color composited over the theme background (so Textual's blending never rounds them):

| Variable | Token |
|---|---|
| `$omarchy-foreground`, `-background`, `-accent`, `-urgent`, `-success`, `-warning`, `-muted` | `palette.*` |
| `$omarchy-text-dim`, `-text-faint`, `-track`, `-scrim` | `roles.*` (`scrim` keeps its alpha) |
| `$omarchy-normal-fill`, `-normal-border`, `-hover-fill`, `-hover-border`, `-focus-fill`, `-focus-border`, `-selected-fill`, `-selected-text`, `-pressed-fill`, `-selection-fill` | `controls.*` |
| `$omarchy-popup-border`, `-menu-border`, `-menu-selected-background`, `-menu-selected-text`, `-tooltip-border`, `-notification-border`, `-error-border`, `-error-text` | `surfaces.*` |

Never paint `$omarchy-background` behind a widget yourself: the page is the terminal's (it is written as the default background either way).

## Look at it

```sh
tui/textual/build                                                 # build/textual: the environment (uv) and the gallery
build/textual/omarchy-textual-gallery                             # every section, in this terminal
build/textual/omarchy-textual-gallery --snap out.ans --section lists
target/release/omarchy-ansi png out.ans out.png                   # render a snapshot to look at
```

In the gallery `[` and `]` change section, Tab and Shift+Tab walk the controls, the mouse clicks and hovers, `q` quits. Any app gets `--snap out.ans [--size COLSxROWS]` from `omarchy_textual.snap.from_args(app)`: the app runs headless at that size and what Textual's compositor put on the screen is written as the canonical snapshot (one takes about 0.3 s, Python's start and Textual's import included). Prefix with `OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8` for a light theme with rounded corners, `OMARCHY_UI_COLORS=16` for the 16-color fallback. `tui/textual/dev/diff [--png] SECTION…` diffs the gallery against Ratatui's in `bin/review`'s themes and roundings.

## Tests

`tools/bindings.d/textual.json` lists them: the behavior tests (`tui/textual/tests/test_*.py`, Textual's Pilot driving the keys and clicks the other galleries' tests drive), every section in every fixture theme at its spec'd size and canonical (`tests/snapshots.py`), the gallery in a pty at truecolor and 16 colors (`tui/textual/pty-test`, a private tmux server), and the live switch (`tests/live_switch.py` under `tools/live/run`, with and without reduced motion).
