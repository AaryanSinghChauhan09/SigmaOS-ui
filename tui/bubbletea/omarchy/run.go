package omarchy

import (
	"time"

	tea "charm.land/bubbletea/v2"
	"github.com/charmbracelet/colorprofile"
)

// ThemeChangedMsg is the changed hook (spec/following.md): it reaches your Update once per change
// that alters the tokens, on the program's event loop, after the new tokens are in effect
// (theme.Tokens() already has them, the revision counts the change); never for the tokens the
// app started with. For what the kit can't reach: caches, a child process to re-theme.
type ThemeChangedMsg struct {
	Tokens   Tokens
	Revision uint64
}

// themeLineMsg is one state line from the core's watch.
type themeLineMsg struct {
	tokens Tokens
	ok     bool
}

// fadeFrameMsg asks for the next frame of a theme fade.
type fadeFrameMsg struct{}

// FrameMsg is what [Animate] delivers: a redraw for an animation (a spinner, the indeterminate
// bar). Ask for the next one while it should keep moving.
type FrameMsg struct{ Time time.Time }

// Animate asks for a frame in ~33 ms (30 a second): return it from Update while something of
// yours animates, and again on each FrameMsg.
func Animate() tea.Cmd {
	return tea.Tick(33*time.Millisecond, func(at time.Time) tea.Msg { return FrameMsg{at} })
}

// Followed is a model wrapped by [Wrap]: it follows the theme for the model and forwards every
// other message. Model() is the app's model as it is now.
type Followed struct {
	theme      *Theme
	model      tea.Model
	fullscreen bool
	err        error
}

// Model is the wrapped app model.
func (f Followed) Model() tea.Model { return f.model }

// Err is why following stopped, if the core couldn't be started.
func (f Followed) Err() error { return f.err }

// Wrap makes model follow the desktop through theme (spec/following.md): theme switches,
// `omarchy font set`, `omarchy display text size`, Hyprland reloads and the language arrive from
// the core's watch on the event loop; each change takes effect (the revision counts it), then
// model gets a [ThemeChangedMsg]; colors fade over motion.theme on motion.ease by redrawing at
// 60 frames a second, in truecolor only (none under reduced motion, at 256 or 16 colors, or on
// the first frame). Use it with your own tea.NewProgram; [Run] does the rest for you.
func Wrap(theme *Theme, model tea.Model) Followed {
	return Followed{theme: theme, model: model}
}

func (f Followed) Init() tea.Cmd {
	w, err := startWatch()
	if err != nil {
		return tea.Batch(f.model.Init(), func() tea.Msg { return watchErrMsg{err} })
	}
	f.theme.watch = w
	return tea.Batch(f.model.Init(), f.theme.next())
}

type watchErrMsg struct{ err error }

// next waits for the core's next line.
func (t *Theme) next() tea.Cmd {
	w := t.watch
	if w == nil {
		return nil
	}
	return func() tea.Msg {
		tk, ok := <-w.lines
		return themeLineMsg{tk, ok}
	}
}

func fadeFrame() tea.Cmd {
	return tea.Tick(16*time.Millisecond, func(time.Time) tea.Msg { return fadeFrameMsg{} })
}

func (f Followed) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case themeLineMsg:
		if !msg.ok {
			return f, nil
		}
		next := f.theme.next()
		if !f.theme.SetTokens(msg.tokens) {
			return f, next
		}
		// The new tokens are in effect: now the hook, once, on this loop.
		var cmd tea.Cmd
		f.model, cmd = f.model.Update(ThemeChangedMsg{Tokens: f.theme.Tokens(), Revision: f.theme.Revision()})
		cmds := []tea.Cmd{next, cmd}
		if f.theme.Fading() {
			cmds = append(cmds, fadeFrame())
		}
		return f, tea.Batch(cmds...)
	case fadeFrameMsg:
		if f.theme.Tick() {
			return f, fadeFrame()
		}
		return f, nil
	case watchErrMsg:
		f.err = msg.err
		return f, nil
	}
	var cmd tea.Cmd
	f.model, cmd = f.model.Update(msg)
	return f, cmd
}

func (f Followed) View() tea.View {
	f.theme.ClearHits()
	v := f.model.View()
	if f.fullscreen {
		v.AltScreen = true
		v.MouseMode = tea.MouseModeAllMotion
	}
	return v
}

// Stop ends following: the core's watch process exits ([Run] calls it; with [Wrap], call it after
// your program returns. The watch also ends with this process).
func (t *Theme) Stop() {
	if t.watch != nil {
		t.watch.stop()
	}
}

// Profile is the colorprofile matching the theme's depth, so Bubble Tea writes exactly the
// colors the kit mapped (spec/terminal.md, Depth).
func (t *Theme) Profile() colorprofile.Profile {
	switch t.depth {
	case ANSI256:
		return colorprofile.ANSI256
	case ANSI16:
		return colorprofile.ANSI
	}
	return colorprofile.TrueColor
}

// Run is the one call: model in a full-screen program (the alternate screen, mouse reporting with
// motion), following the desktop through theme as [Wrap] does, at the theme's color depth. It
// returns the app's model as it was when the program ended.
func Run(theme *Theme, model tea.Model, opts ...tea.ProgramOption) (tea.Model, error) {
	w := Wrap(theme, model)
	w.fullscreen = true
	opts = append([]tea.ProgramOption{tea.WithColorProfile(theme.Profile())}, opts...)
	out, err := tea.NewProgram(w, opts...).Run()
	theme.Stop()
	if f, ok := out.(Followed); ok {
		return f.model, err
	}
	return model, err
}

// IsQuit is true for Ctrl+C, or `q` without modifiers.
func IsQuit(msg tea.Msg) bool {
	k, ok := msg.(tea.KeyPressMsg)
	if !ok {
		return false
	}
	s := k.String()
	return s == "ctrl+c" || s == "q"
}
