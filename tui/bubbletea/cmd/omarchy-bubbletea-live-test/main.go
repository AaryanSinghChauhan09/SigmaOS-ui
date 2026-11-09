// The live-switch test (spec/following.md), run under tools/live/run: a Bubble Tea program on the
// kit's one call (omarchy.Wrap, without a renderer or input) follows the theme, the theme is
// switched the way `omarchy theme set` does it, and the model must see catppuccin-latte through
// the public API, the changed hook (omarchy.ThemeChangedMsg) once with the new tokens, revision 1,
// a fade that ran and landed exactly (the frame's painted cell too); under
// OMARCHY_UI_REDUCE_MOTION=1, no fade.
package main

import (
	"fmt"
	"io"
	"os"
	"os/exec"
	"time"

	tea "charm.land/bubbletea/v2"

	o "omarchy-ui/tui/bubbletea/omarchy"
)

const latte = "#eff1f5ff"

func fail(format string, a ...any) {
	fmt.Fprintf(os.Stderr, "live switch: FAIL: "+format+"\n", a...)
	os.Exit(1)
}

type switchMsg struct{}
type checkMsg struct{}

type live struct {
	theme            *o.Theme
	home, switchBin  string
	startBg          o.Rgba
	switched, landed time.Time
	hooks            []string
	faded, between   bool
	// painted is the accent cell the last frame drew.
	painted *o.Color
}

func check() tea.Cmd {
	return tea.Tick(10*time.Millisecond, func(time.Time) tea.Msg { return checkMsg{} })
}

func (m *live) Init() tea.Cmd {
	return tea.Batch(tea.Tick(300*time.Millisecond, func(time.Time) tea.Msg { return switchMsg{} }), check())
}

func (m *live) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case o.ThemeChangedMsg:
		// After the new tokens are in effect: the public API already reads them.
		if m.theme.Tokens().Palette.Background != msg.Tokens.Palette.Background || m.theme.Revision() != msg.Revision {
			fail("the hook ran before the new tokens were in effect")
		}
		m.hooks = append(m.hooks, msg.Tokens.Palette.Background.Hex())
	case switchMsg:
		if err := exec.Command(m.switchBin, m.home, "catppuccin-latte").Run(); err != nil {
			fail("%s failed: %v", m.switchBin, err)
		}
		m.switched = time.Now()
	case checkMsg:
		t := m.theme
		if !m.switched.IsZero() {
			if m.landed.IsZero() && t.Tokens().Palette.Background.Hex() == latte && !t.Fading() {
				m.landed = time.Now()
			}
			if !m.landed.IsZero() && time.Since(m.landed) > 600*time.Millisecond {
				// Another 600 ms: the hook must still have run once.
				return m, tea.Quit
			}
			if m.landed.IsZero() && time.Since(m.switched) > 5*time.Second {
				fail("background is %s after 5 s, not %s", t.Tokens().Palette.Background.Hex(), latte)
			}
		}
		return m, check()
	}
	return m, nil
}

func (m *live) View() tea.View {
	t := m.theme
	buf := o.NewBuffer(o.R(0, 0, 4, 1))
	o.Put(t, buf, 0, 0, "█", t.Accent())
	t.Finish(buf)
	c := buf.At(0, 0).Fg
	m.painted = &c
	shown := t.Shown().Palette.Background
	m.faded = m.faded || t.Fading()
	m.between = m.between || (t.Tokens().Palette.Background.Hex() == latte && shown != m.startBg && shown != t.Tokens().Palette.Background)
	return tea.NewView(o.ANSI(buf))
}

func main() {
	home := os.Getenv("OMARCHY_UI_LIVE_HOME")
	if home == "" {
		fail("run me under tools/live/run")
	}
	switchBin := os.Getenv("OMARCHY_UI_LIVE_SWITCH")
	if switchBin == "" {
		fail("no OMARCHY_UI_LIVE_SWITCH")
	}
	reduced := os.Getenv("OMARCHY_UI_REDUCE_MOTION") == "1"
	theme, err := o.Load()
	if err != nil {
		fail("%v", err)
	}
	theme.SetDepth(o.TrueColor)
	app := &live{theme: theme, home: home, switchBin: switchBin, startBg: theme.Tokens().Palette.Background}
	p := tea.NewProgram(o.Wrap(theme, app), tea.WithoutRenderer(), tea.WithInput(nil), tea.WithOutput(io.Discard), tea.WithoutSignals())
	out, err := p.Run()
	theme.Stop()
	if err != nil {
		fail("%v", err)
	}
	if f, ok := out.(o.Followed); ok && f.Err() != nil {
		fail("following: %v", f.Err())
	}
	t := theme
	accent := t.Tokens().Palette.Accent
	if app.painted == nil || *app.painted != o.CellRGB(accent) {
		fail("the frame shows %+v, not the new accent %s", app.painted, accent.Hex())
	}
	if t.Shown().Palette.Background != t.Tokens().Palette.Background {
		fail("the fade didn't land exactly")
	}
	if len(app.hooks) != 1 || app.hooks[0] != latte {
		fail("changed hook fired %v, want once with %s", app.hooks, latte)
	}
	if t.Revision() != 1 {
		fail("revision %d, want 1", t.Revision())
	}
	if reduced && (app.faded || app.between) {
		fail("a fade ran under reduced motion")
	}
	if !reduced && !(app.faded && app.between) {
		fail("no fade seen (fading %v, a color between %v)", app.faded, app.between)
	}
	how := "faded and landed exactly"
	if reduced {
		how = "no fade (reduced motion)"
	}
	fmt.Printf("live switch: %s in %d ms, hook once, revision 1, %s\n", latte, app.landed.Sub(app.switched).Milliseconds(), how)
}
