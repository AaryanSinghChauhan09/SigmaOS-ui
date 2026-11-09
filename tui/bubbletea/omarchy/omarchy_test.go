package omarchy

import (
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"testing"
	"time"

	tea "charm.land/bubbletea/v2"
)

func repo() string {
	_, file, _, _ := runtime.Caller(0)
	return filepath.Join(filepath.Dir(file), "..", "..", "..")
}

// fixture resolves a fixture theme through the core's CLI, as the gallery does.
func fixture(tb testing.TB, name string, rounding string) *Theme {
	tb.Helper()
	tb.Setenv("OMARCHY_UI_THEME_DIR", filepath.Join(repo(), "fixtures/themes", name))
	tb.Setenv("OMARCHY_UI_ROUNDING", rounding)
	tb.Setenv("OMARCHY_UI_REDUCE_MOTION", "1")
	t, err := Load()
	if err != nil {
		tb.Fatal(err)
	}
	t.SetDepth(TrueColor)
	return t
}

func TestDarkerRoundsHalvesAsQtDoes(t *testing.T) {
	fg := RGB(0xa9, 0xb1, 0xd6)
	if got := fg.Darker(2.0).Hex(); got != "#54596bff" {
		t.Errorf("darker 2: %s", got)
	}
	if got := fg.Darker(1.5).Hex(); got != "#71768fff" {
		t.Errorf("darker 1.5: %s", got)
	}
}

func TestCompositesLikeTheCore(t *testing.T) {
	fg, bg := RGB(0xa9, 0xb1, 0xd6), RGB(0x1a, 0x1b, 0x26)
	for a := 0; a <= 255; a++ {
		top := fg
		top.A = uint8(a)
		mine := Over(top, bg)
		core := bg.Mix(fg, float64(a)/255)
		if mine.R != core.R || mine.G != core.G || mine.B != core.B {
			t.Fatalf("alpha %d: %v vs %v", a, mine, core)
		}
	}
}

func TestWidths(t *testing.T) {
	for s, w := range map[string]int{"\U000f0928": 1, "…": 1, "─": 1, "⠋": 1, "●": 1, "—": 1, "×": 1, "“": 1, "−": 1, "⏎": 1, "⇧": 1, "日": 2, "é": 1, "é": 1, "▏": 1, "\uf120": 1} {
		if got := Width(s); got != w {
			t.Errorf("width %q = %d, want %d", s, got, w)
		}
	}
}

func TestDepth(t *testing.T) {
	env := func(kv ...string) func(string) string {
		return func(k string) string {
			for i := 0; i+1 < len(kv); i += 2 {
				if kv[i] == k {
					return kv[i+1]
				}
			}
			return ""
		}
	}
	if DepthFromEnv(env("COLORTERM", "truecolor")) != TrueColor || DepthFromEnv(env("TERM", "xterm-256color")) != ANSI256 ||
		DepthFromEnv(env("TERM", "linux")) != ANSI16 || DepthFromEnv(env("COLORTERM", "truecolor", "OMARCHY_UI_COLORS", "16")) != ANSI16 {
		t.Error("depth detection")
	}
	if Nearest256([3]uint8{0, 0, 0}) != 16 || Nearest256([3]uint8{255, 255, 255}) != 231 || Nearest256([3]uint8{128, 128, 128}) != 244 {
		t.Error("nearest 256")
	}
	if e := CubicBezier([4]float64{0.25, 0.1, 0.25, 1}, 0.5); e < 0.8014 || e > 0.8034 {
		t.Errorf("ease(0.5) = %v", e)
	}
}

// The page background is never painted; other fills are, mapped down below truecolor.
func TestFinishLeavesTheBackgroundToTheTerminal(t *testing.T) {
	th := fixture(t, "tokyo-night", "0")
	paint := func() *Buffer {
		b := NewBuffer(R(0, 0, 3, 1))
		Fill(th, b, R(0, 0, 1, 1), th.Bg())
		Fill(th, b, R(1, 0, 1, 1), th.Shown().Controls.Hover.Fill)
		Put(th, b, 2, 0, "x", th.Dim())
		return b
	}
	b := paint()
	th.Finish(b)
	if b.At(0, 0).Bg.Kind != DefaultColor || b.At(1, 0).Bg.Kind != RGBColor {
		t.Error("truecolor")
	}
	th.SetDepth(ANSI16)
	b = paint()
	th.Finish(b)
	if b.At(1, 0).Bg != Indexed(8) || b.At(2, 0).Fg != Indexed(8) {
		t.Errorf("16 colors: %+v %+v", b.At(1, 0).Bg, b.At(2, 0).Fg)
	}
	th.SetDepth(ANSI256)
	b = paint()
	th.Finish(b)
	if c := b.At(1, 0).Bg; c.Kind != IndexedColor || c.Index < 16 {
		t.Error("256 colors")
	}
}

// A change counts once, fades in truecolor and lands exactly; none at 16 colors.
func TestChangesFadeAndLand(t *testing.T) {
	th := fixture(t, "tokyo-night", "0")
	next := th.Tokens()
	next.Palette.Background = RGB(0xef, 0xf1, 0xf5)
	next.ReduceMotion = false
	if !th.SetTokens(next) || th.SetTokens(next) {
		t.Fatal("a change counts once")
	}
	if th.Revision() != 1 || !th.Fading() || th.Shown().Palette.Background == next.Palette.Background {
		t.Fatal("fading")
	}
	time.Sleep(time.Duration(next.Motion.Theme+20) * time.Millisecond)
	if th.Tick() || th.Shown().Palette.Background != next.Palette.Background {
		t.Fatal("landed")
	}
	t16 := fixture(t, "tokyo-night", "0")
	t16.SetDepth(ANSI16)
	t16.SetTokens(next)
	if t16.Fading() {
		t.Fatal("no fade at 16 colors")
	}
}

func TestSnapshotIsCanonical(t *testing.T) {
	th := fixture(t, "catppuccin-latte", "8")
	box := Custom(10, 3, func(t *Theme, area Rect, buf *Buffer) {
		Fill(t, buf, area, t.Shown().Controls.Hover.Fill)
		Frame(t, buf, area, t.Accent(), "日本", InkOf(t.Dim()).WithBold(true))
	})
	s := th.RenderSnapshot(box, 12, 0)
	if strings.Count(s, "\n") != 3 || !strings.HasPrefix(s, "\x1b[0;38;2;") {
		t.Fatalf("%q", s)
	}
	out := filepath.Join(t.TempDir(), "x.ans")
	if err := os.WriteFile(out, []byte(s), 0o644); err != nil {
		t.Fatal(err)
	}
}

type recorder struct {
	theme *Theme
	hooks []ThemeChangedMsg
	early bool
}

func (r *recorder) Init() tea.Cmd { return nil }
func (r *recorder) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	if m, ok := msg.(ThemeChangedMsg); ok {
		// The hook runs after the new tokens are in effect.
		r.early = r.early || r.theme.Tokens().Palette.Background != m.Tokens.Palette.Background
		r.hooks = append(r.hooks, m)
	}
	return r, nil
}
func (r *recorder) View() tea.View { return tea.NewView("") }

// Wrap applies a change, then delivers the hook once with the new tokens and revision; the same
// tokens again (the watch's first line) are no change; a fade asks for frames until it lands.
func TestWrapDeliversTheHookOnceAfterTheChange(t *testing.T) {
	th := fixture(t, "tokyo-night", "0")
	rec := &recorder{theme: th}
	var m tea.Model = Wrap(th, rec)
	m, _ = m.Update(themeLineMsg{th.Tokens(), true})
	if len(rec.hooks) != 0 || th.Revision() != 0 {
		t.Fatal("the initial state is no change")
	}
	next := th.Tokens()
	next.Palette.Background = RGB(0xef, 0xf1, 0xf5)
	next.ReduceMotion = false
	m, cmd := m.Update(themeLineMsg{next, true})
	if len(rec.hooks) != 1 || rec.early || rec.hooks[0].Revision != 1 || rec.hooks[0].Tokens.Palette.Background != next.Palette.Background {
		t.Fatalf("hooks %d early %v", len(rec.hooks), rec.early)
	}
	if cmd == nil || !th.Fading() {
		t.Fatal("a fade asks for frames")
	}
	time.Sleep(time.Duration(next.Motion.Theme+20) * time.Millisecond)
	if _, cmd = m.Update(fadeFrameMsg{}); cmd != nil || th.Fading() || th.Shown().Palette.Background != next.Palette.Background {
		t.Fatal("the fade lands exactly and stops asking")
	}
	if len(rec.hooks) != 1 {
		t.Fatal("fade frames are not changes")
	}
}
