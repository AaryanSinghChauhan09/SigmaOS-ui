// A Wi-Fi panel in the terminal on Bubble Tea: a Card with a Hero and a list of networks under a
// keyboard cursor, following the Omarchy theme live. j/k or the arrows move, Return connects, the
// mouse hovers and clicks, q quits. `--snap out.ans` writes it as an ANSI snapshot instead.
package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
	"omarchy-ui/tui/bubbletea/ui"
)

type network struct {
	name, security string
	locked         bool
}

var networks = []network{{"Basecamp", "WPA2", true}, {"Basecamp Guest", "Open", false}, {"Coffee Shop", "WPA2", true}, {"Library", "Saved", true}}

type model struct {
	theme     *omarchy.Theme
	cursor    ui.CursorModel
	connected int
	// changes counts theme changes: the changed hook.
	changes int
}

func (m model) Init() tea.Cmd { return nil }

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case omarchy.ThemeChangedMsg:
		m.changes++
	case tea.KeyPressMsg:
		if omarchy.IsQuit(msg) {
			return m, tea.Quit
		}
		if m.cursor.ApplyKey(msg) == ui.CursorActivate {
			m.connected = m.cursor.Index()
		}
	case tea.MouseMotionMsg:
		if i, ok := m.networkAt(msg.Mouse()); ok {
			m.cursor.PointerMoved(msg.Mouse().X, msg.Mouse().Y, "", i)
		}
	case tea.MouseClickMsg:
		if i, ok := m.networkAt(msg.Mouse()); ok {
			m.connected = i
		}
	}
	return m, nil
}

// networkAt is the network row under the pointer (rows are keyed net-<i> as they draw).
func (m model) networkAt(at tea.Mouse) (int, bool) {
	i, err := strconv.Atoi(strings.TrimPrefix(m.theme.HitAt(at.X, at.Y), "net-"))
	return i, err == nil
}

// panel is the view: kit components built from the model, every frame (colors fade).
func (m model) panel() omarchy.Component {
	list := ui.NewScrollArea(42).MaxRows(8)
	for i, n := range networks {
		row := ui.NewListRow(n.name).Glyph("\U000f0928").Width(42).Cursor(m.cursor.IsAt("", i)).Current(i == m.connected).Key("net-" + strconv.Itoa(i))
		if i == m.connected {
			row = row.Status("Connected", ui.Success)
		} else {
			row = row.Subtitle(n.security)
		}
		if n.locked {
			row = row.TrailingGlyph("\U000f033e", ui.Neutral)
		}
		list = list.Child(row)
	}
	return ui.NewCard().Width(48).Child(
		ui.NewHero("\U000f0928", "Wi-Fi").Status("Connected to "+networks[m.connected].name).Trailing(ui.NewToggleSwitch(true)).Width(42),
		ui.SectionHeader("Networks"),
		list,
		omarchy.Column(0,
			ui.Separator(42),
			ui.NewKeyValue().Pair("Theme changes", strconv.Itoa(m.changes)).Pair("Revision", strconv.FormatUint(m.theme.Revision(), 10)),
		),
	)
}

func (m model) View() tea.View { return tea.NewView(m.theme.Render(m.panel())) }

func main() {
	theme, err := omarchy.Load()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	cursor := ui.NewCursorList(len(networks))
	cursor.Set("", 1)
	m := model{theme: theme, cursor: cursor}
	if done, err := omarchy.SnapFromArgs(theme, m.panel()); done || err != nil {
		if err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
		return
	}
	if _, err := omarchy.Run(theme, m); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
