// The Bubble Tea gallery: every Omarchy UI component in the live theme, section for section with
// the other galleries (spec/gallery.md), at the cell sizes spec/terminal.md lists.
//
//	omarchy-bubbletea-gallery                                  the gallery in this terminal
//	omarchy-bubbletea-gallery --section lists                  ...starting on a section
//	omarchy-bubbletea-gallery --list-sections                  the sections, in order
//	omarchy-bubbletea-gallery --snap out.ans --section buttons [--size 96x0]
//	                                                           one section as an ANSI snapshot
//
// In the window: `[` and `]` (or Ctrl+←/→) change section, Tab and Shift+Tab walk the section's
// controls, the focused one takes the keys, the mouse clicks and hovers, the wheel and Page
// Up/Down scroll, `q` quits.
package main

import (
	"fmt"
	"os"
	"strconv"
	"strings"

	"charm.land/bubbles/v2/key"
	tea "charm.land/bubbletea/v2"

	o "omarchy-ui/tui/bubbletea/omarchy"
	"omarchy-ui/tui/bubbletea/ui"
)

// keys are the window's own keys; the footer shows their help.
var keys = struct {
	Focus, Back, Prev, Next, Unfocus, PageDown, PageUp, Down, Up key.Binding
}{
	Focus:    key.NewBinding(key.WithKeys("tab"), key.WithHelp("Tab", "focus")),
	Back:     key.NewBinding(key.WithKeys("shift+tab")),
	Prev:     key.NewBinding(key.WithKeys("[", "ctrl+left"), key.WithHelp("[", "")),
	Next:     key.NewBinding(key.WithKeys("]", "ctrl+right"), key.WithHelp("]", "section")),
	Unfocus:  key.NewBinding(key.WithKeys("esc")),
	PageDown: key.NewBinding(key.WithKeys("pgdown")),
	PageUp:   key.NewBinding(key.WithKeys("pgup")),
	Down:     key.NewBinding(key.WithKeys("down")),
	Up:       key.NewBinding(key.WithKeys("up")),
}

var quitHelp = key.NewBinding(key.WithKeys("q"), key.WithHelp("q", "quit"))

// gallery is the interactive window's model. The State is shared (a pointer): the components are
// views of it.
type gallery struct {
	theme         *o.Theme
	state         *State
	section       int
	scroll        int
	width, height int
}

func (g gallery) name() string { return sections[g.section].name }

func (g gallery) animating() bool { return g.name() == "progress" && !g.theme.ReduceMotion() }

// drawPage draws section name into a buffer of its own (natural height plus room for an open
// dropdown), with hits recorded in that buffer's coordinates.
func drawPage(s *State, t *o.Theme, name string) *o.Buffer {
	p := page(s, name)
	_, h := p.Size(t)
	area := o.R(0, 0, cols, h+8)
	buf := o.NewBuffer(area)
	p.Draw(t, o.R(0, 0, cols, h), buf)
	if name == "inputs" && s.dropdownOpen {
		if r, ok := t.Find("theme"); ok {
			list := ui.NewDropdownList(themes, s.themeChoice).Highlighted(s.dropdownRow).Width(r.W).Key("theme-list")
			w, lh := list.Size(t)
			list.Draw(t, o.R(r.X, r.Y+1, w, lh), buf)
		}
	}
	return buf
}

func (g gallery) Init() tea.Cmd {
	if g.animating() {
		return o.Animate()
	}
	return nil
}

func (g *gallery) natural() int {
	_, h := page(g.state, g.name()).Size(g.theme)
	return h
}

func (g *gallery) clampScroll() {
	g.scroll = max(0, min(g.scroll, g.natural()-max(g.height-3, 0)))
}

func (g *gallery) go_(d int) tea.Cmd {
	n := len(sections)
	g.section = ((g.section+d)%n + n) % n
	g.scroll = 0
	g.state.focus = ""
	g.state.dropdownOpen = false
	if g.animating() {
		return o.Animate()
	}
	return nil
}

func (g *gallery) cycleFocus(back bool) {
	fs := focusables(g.name())
	if len(fs) == 0 {
		return
	}
	n, i := len(fs), -1
	for j, f := range fs {
		if f == g.state.focus {
			i = j
		}
	}
	switch {
	case i < 0 && !back:
		i = 0
	case i < 0:
		i = n - 1
	case !back:
		i = (i + 1) % n
	default:
		i = (i + n - 1) % n
	}
	g.state.focus = fs[i]
}

func (g gallery) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		g.width, g.height = msg.Width, msg.Height
		g.clampScroll()
	case o.FrameMsg:
		if g.animating() {
			return g, o.Animate()
		}
	case tea.KeyPressMsg:
		if msg.String() == "ctrl+c" || (o.IsQuit(msg) && !g.state.typing()) {
			return g, tea.Quit
		}
		// Tab is the window's: it walks the controls (spec/terminal.md, Harness), so a control
		// that would keep it (a list's sections, a dialog's buttons) never traps it.
		walk := key.Matches(msg, keys.Focus) || key.Matches(msg, keys.Back)
		if !walk && g.state.key(msg) {
			return g, nil
		}
		body := max(g.height-3, 0)
		switch {
		case key.Matches(msg, keys.Next):
			return g, g.go_(1)
		case key.Matches(msg, keys.Prev):
			return g, g.go_(-1)
		case key.Matches(msg, keys.Focus):
			g.cycleFocus(false)
		case key.Matches(msg, keys.Back):
			g.cycleFocus(true)
		case key.Matches(msg, keys.Unfocus):
			g.state.focus = ""
		case key.Matches(msg, keys.PageDown):
			g.scroll += max(body-1, 0)
		case key.Matches(msg, keys.PageUp):
			g.scroll -= max(body-1, 0)
		case key.Matches(msg, keys.Down) && g.state.focus == "":
			g.scroll++
		case key.Matches(msg, keys.Up) && g.state.focus == "":
			g.scroll--
		}
		g.clampScroll()
	case tea.MouseWheelMsg:
		switch msg.Mouse().Button {
		case tea.MouseWheelDown:
			g.scroll += 3
		case tea.MouseWheelUp:
			g.scroll -= 3
		}
		g.clampScroll()
	case tea.MouseClickMsg:
		m := msg.Mouse()
		if m.Button != tea.MouseLeft {
			break
		}
		if k := g.theme.HitAt(m.X, m.Y); k != "" {
			r, _ := g.theme.Find(k)
			if f := focusOf(g.name(), k); f != "" {
				g.state.focus = f
			}
			g.state.click(g.theme, g.name(), k, m.X, r)
		}
	case tea.MouseMotionMsg:
		m := msg.Mouse()
		if k := g.theme.HitAt(m.X, m.Y); k != "" {
			g.state.hover(k, m.X, m.Y)
		}
	}
	return g, nil
}

// frame draws the window: the header, the section scrolled between them, the footer's keys.
func (g gallery) frame(t *o.Theme, area o.Rect, buf *o.Buffer) {
	o.Text(t, buf, 1, 0, "Omarchy UI · Bubble Tea", area.W, o.InkOf(t.Fg()).WithBold(true))
	right := sections[g.section].title + "  " + strconv.Itoa(g.section+1) + "/" + strconv.Itoa(len(sections))
	o.TextRight(t, buf, max(area.W-1, 0), 0, right, t.Dim())
	o.HLine(t, buf, 0, 1, area.W, "─", t.Track())
	fy := max(area.H-1, 0)
	x := 1
	for _, b := range []key.Binding{keys.Focus, keys.Prev, keys.Next, quitHelp} {
		h := b.Help()
		kc := ui.KeyCombo(h.Key)
		w, _ := kc.Size(t)
		kc.Draw(t, o.R(x, fy, w, 1).Intersect(area), buf)
		x += w + 1
		if h.Desc != "" {
			x += o.Text(t, buf, x, fy, h.Desc, max(area.W-x, 0), t.Dim()) + 2
		}
	}
	body := o.R(0, 2, min(area.W, cols), max(area.H-3, 0))
	hits := t.HitsLen()
	pg := drawPage(g.state, t, g.name())
	scroll := g.scroll
	for dy := range body.H {
		for dx := range body.W {
			if src, dst := pg.At(dx, scroll+dy), buf.At(body.X+dx, body.Y+dy); src != nil && dst != nil {
				*dst = *src
			}
		}
	}
	shown := o.R(0, scroll, body.W, body.H)
	t.RemapHits(hits, func(r o.Rect) (o.Rect, bool) {
		r = r.Intersect(shown)
		if r.H <= 0 {
			return r, false
		}
		r.Y = r.Y - scroll + body.Y
		return r, true
	})
}

func (g gallery) View() tea.View {
	w, h := g.width, g.height
	if w == 0 || h == 0 {
		w, h = cols, 40
	}
	return tea.NewView(g.theme.RenderSize(o.Custom(w, h, g.frame), w, h))
}

func arg(args []string, name string) string {
	for i, a := range args {
		if a == name && i+1 < len(args) {
			return args[i+1]
		}
	}
	return ""
}

func has(args []string, name string) bool {
	for _, a := range args {
		if a == name {
			return true
		}
	}
	return false
}

func sizeArg(args []string) (int, int, bool) {
	c, r, ok := strings.Cut(arg(args, "--size"), "x")
	if !ok {
		return 0, 0, false
	}
	cn, err1 := strconv.Atoi(c)
	rn, err2 := strconv.Atoi(r)
	return cn, rn, err1 == nil && err2 == nil
}

func sectionIndex(name string) int {
	for i, s := range sections {
		if s.name == name {
			return i
		}
	}
	return -1
}

func main() {
	args := os.Args[1:]
	if has(args, "--list-sections") {
		for _, s := range sections {
			fmt.Println(s.name)
		}
		return
	}
	section := arg(args, "--section")
	if section != "" && sectionIndex(section) < 0 {
		fmt.Fprintf(os.Stderr, "no section %s (--list-sections)\n", section)
		os.Exit(2)
	}
	theme, err := o.Load()
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
	state := initState()
	if out := arg(args, "--snap"); out != "" {
		if section == "" {
			section = "buttons"
		}
		c, r, ok := sizeArg(args)
		if !ok {
			c, r = cols, 0
		}
		snap := theme.RenderSnapshot(page(&state, section), c, r)
		if err := os.WriteFile(out, []byte(snap), 0o644); err != nil {
			fmt.Fprintln(os.Stderr, err)
			os.Exit(1)
		}
		return
	}
	start := max(sectionIndex(section), 0)
	if _, err := o.Run(theme, gallery{theme: theme, state: &state, section: start}); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
