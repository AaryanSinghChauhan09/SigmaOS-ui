# Using Omarchy UI from Bubble Tea

> `<omarchy-ui>` below stands for a clone of this repository. Paths such as `examples/...` are relative to a checkout.

The Go module `omarchy-ui/tui/bubbletea` (`tui/bubbletea`) gives a [Bubble Tea](https://github.com/charmbracelet/bubbletea) app Omarchy's look in the terminal with one call: the live theme, followed without a restart and fed into your program as a message, colors that fade on a switch, and the kit's components (`Button`, `Toggle`, `ListRow`, `Menu`, `Table`, …) drawn on the cell grid the way [`spec/terminal.md`](../spec/terminal.md) maps them, cell for cell the same as the Ratatui reference. [`using-terminal.md`](using-terminal.md) has what every terminal binding shares.

## Adopt it

```
// go.mod
require (
	charm.land/bubbletea/v2 v2.1.1
	omarchy-ui/tui/bubbletea v0.0.0
)

replace omarchy-ui/tui/bubbletea => ../omarchy-ui/tui/bubbletea
```

The module pins, in `tui/bubbletea/go.mod`: Bubble Tea `charm.land/bubbletea/v2 v2.1.1`, Lip Gloss `charm.land/lipgloss/v2 v2.1.0` and Bubbles `charm.land/bubbles/v2 v2.2.1` (the v2 lines, current stable), and `github.com/mattn/go-runewidth v0.0.31` for cell widths. Use the same versions in your app (Go's minimum version selection lifts yours to at least these).

Tokens come from the core, never from Go: the module runs the `omarchy-theme` CLI (`--state` once, `--watch --state` to follow; no cgo). It finds it as the web binding does: `$OMARCHY_THEME_BIN`, else `omarchy-theme` on `PATH` (the `omarchy-ui` package), else this checkout's `target/release/omarchy-theme` (`bin/build core`).

## The one call

[`examples/bubbletea/main.go`](../examples/bubbletea/main.go), a Wi-Fi panel:

```go
type model struct {
	theme     *omarchy.Theme
	cursor    ui.CursorModel
	connected int
}

func (m model) Init() tea.Cmd { return nil }

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case omarchy.ThemeChangedMsg: // the changed hook: msg.Tokens, msg.Revision
	case tea.KeyPressMsg:
		if omarchy.IsQuit(msg) {
			return m, tea.Quit
		}
		if m.cursor.ApplyKey(msg) == ui.CursorActivate {
			m.connected = m.cursor.Index()
		}
	}
	return m, nil
}

func (m model) View() tea.View {
	card := ui.NewCard().Width(48).Child(
		ui.NewHero("\U000f0928", "Wi-Fi").Status("Connected to Basecamp").Trailing(ui.NewToggleSwitch(true)).Width(42),
		ui.NewListRow("Basecamp").Glyph("\U000f0928").Status("Connected", ui.Success).Current(true).Width(42),
	)
	return tea.NewView(m.theme.Render(card))
}

func main() {
	theme := omarchy.MustLoad()
	omarchy.Run(theme, model{theme: theme, cursor: ui.NewCursorList(4)})
}
```

`omarchy.Run(theme, model, opts...)` runs your model in a full-screen program (the alternate screen, mouse reporting with motion, Bubble Tea's color profile set to the theme's depth) and:

- follows the desktop through the core's watch (theme switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads, the language), on Bubble Tea's event loop; each change takes effect (`theme.Tokens()` has it, `theme.Revision()` counts it), then your `Update` gets an `omarchy.ThemeChangedMsg` with the new tokens and revision, once; never for the tokens the app started with;
- fades colors over `motion.theme` (280 ms) on `motion.ease` by redrawing 60 times a second, in truecolor only; none under reduced motion, at 256 or 16 colors, or on the first frame;
- returns your model as it was when the program ended.

With your own `tea.NewProgram`, wrap the model instead: `tea.NewProgram(omarchy.Wrap(theme, model), tea.WithColorProfile(theme.Profile()))`, set `AltScreen` and `MouseMode` on your view, and `theme.Stop()` when it returns. `omarchy.Animate()` asks for a `FrameMsg` in 33 ms: return it while a spinner or an indeterminate bar should move (and again on each `FrameMsg`).

`theme.Render(c)` draws a component at its natural size and returns it as a styled string, after the "page is the terminal's" rule (cells on the theme background are written as the terminal's default, so its opacity shows) and the mapping to 256 or 16 colors. `theme.RenderSize(c, w, h)` fills a w × h area: a full-screen root at the window's size (`tea.WindowSizeMsg`).

## Components

Every component of [`spec/components.md`](../spec/components.md) under the same name in the `ui` package (`omarchy-ui/tui/bubbletea/ui`), each a value built like a `lipgloss.Style`: `ui.NewButton("Save").Bordered(true).Cursor(focused)` returns a copy at each step. Each is an `omarchy.Component` (`Size(t)`, `Draw(t, area, buf)`). They are views of your model: the model owns selection, cursors and values, and the key handlers beside the components move that state from a `tea.KeyPressMsg` (`CursorModel.ApplyKey`, `ui.HandleMenuKey`, `ui.HandleTabsKey`, `ui.HandleMultiSelectKey`, `ui.HandleSearchListKey`, `ui.HandleCalendarKey`, …). Bubbles fit where they already hold the state: the gallery keeps its text in `bubbles/textinput` models and draws them with `ui.NewTextField(in.Value()).Caret(in.Position())`; its window keys are `bubbles/key` bindings.

- **Layout**: `omarchy.Column(gap, …)`, `omarchy.Line(gap, …)`, `omarchy.Sized`, `omarchy.Centered`, `omarchy.FillBox` stack components at their natural sizes inside one buffer, so fills composite over what is under them (a control on a bordered Pane takes the Pane's glass into account). Strings from `theme.Render` also join with `lipgloss.JoinVertical` / `JoinHorizontal`.
- **Lip Gloss**: `theme.Styles()` has the kit's text roles as `lipgloss.Style` values (`Text`, `Title`, `Dim`, `Faint`, `Section`, `Accent`, `Urgent`, …), built from what is on screen: build them in `View` and they fade. `omarchy.Rgba` is an `image/color.Color`, so `lipgloss.NewStyle().Foreground(theme.Accent())` works. `omarchy.Styled(s)` draws any Lip Gloss string into a kit layout.
- **Clicks and hover**: components given `.Key("…")` record where they drew; `theme.HitAt(x, y)` names the part under a `tea.MouseClickMsg` or `tea.MouseMotionMsg` and `theme.Find(key)` gives its `Rect`. `CursorModel.PointerMoved(x, y, section, index)` follows the pointer only when it reaches another cell. Hits are in the rendered component's coordinates: render the root at the screen's top left.
- **Your own drawing**: `omarchy.Custom(w, h, func(t, area, buf) {…})` with `omarchy.Fill` (alpha composites over what's painted), `Put`, `Text`, `TextFit`, `Edges`, `Frame` (corners by the radius), `Scrim`, and the theme's colors (`theme.Fg()`, `Dim()`, `Accent()`, `Shown().Controls.Hover.Fill`, …), read every frame so they fade.
- **Tokens**: `theme.Tokens()` (in effect), `theme.Shown()` (on screen, mid-fade), `theme.Revision()` (a cache key), `theme.String(key, fallback)` (the kit's strings in the display language).

## Look at it

```sh
tui/bubbletea/build                                                  # into build/bubbletea
build/bubbletea/omarchy-bubbletea-gallery                            # every section, in this terminal
build/bubbletea/omarchy-bubbletea-gallery --snap out.ans --section lists
target/release/omarchy-ansi png out.ans out.png                      # render a snapshot to look at
bin/review --bindings bubbletea --open                               # against Ratatui, cell by cell
```

Any app gets `--snap out.ans [--size COLSxROWS]` from `omarchy.SnapFromArgs(theme, view)`. Prefix with `OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8` for a light theme with rounded corners, `OMARCHY_UI_COLORS=16` to see the 16-color fallback.

## Tests

`tools/bindings.d/bubbletea.json` lists them: the kit's and the gallery's tests (`go -C tui/bubbletea test ./...`: the keys and clicks the other galleries' tests drive, and every section's snapshot in every fixture theme at both radii, canonical and at its spec'd size), the gallery in a pty (`tui/bubbletea/pty-test`, a private tmux server: truecolor and 16 colors), and the live switch (`omarchy-bubbletea-live-test` under `tools/live/run`: a real `tea.Program` on `omarchy.Wrap`, with and without reduced motion).
