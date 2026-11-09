package omarchy

import (
	"time"
)

// Theme is the live Omarchy theme for a Bubble Tea app (spec/following.md, spec/terminal.md):
// the tokens in effect (a fade's target), what is on screen (the same tokens with colors
// part-way through a theme fade), the revision, the color depth and the hit map components record
// as they draw. Keep one in your model; [Run] (or [Wrap]) follows the desktop for it and feeds
// the changes into your program. Components read colors from [Theme.Shown], so they fade; read
// yours there too, every View, never cached.
type Theme struct {
	tokens   Tokens
	shown    Tokens
	revision uint64
	depth    Depth
	fade     *fadeState
	watch    *watcher
	hits     []hit
	started  time.Time
	clock    *uint64
}

type fadeState struct {
	from     Tokens
	start    time.Time
	duration time.Duration
}

type hit struct {
	key  string
	area Rect
}

// Load is the theme as it is now (or the OMARCHY_UI_THEME_DIR fixture), not following anything
// yet ([Run] and [Wrap] start following). The color depth comes from the environment.
func Load() (*Theme, error) {
	t, err := ReadState()
	if err != nil {
		return nil, err
	}
	return FromTokens(t), nil
}

// MustLoad is [Load] that panics when the core can't be found or read.
func MustLoad() *Theme {
	t, err := Load()
	if err != nil {
		panic(err)
	}
	return t
}

// FromTokens is a theme pinned to tokens (tests, previews), at the environment's color depth.
func FromTokens(tokens Tokens) *Theme {
	return &Theme{tokens: tokens, shown: tokens, depth: DetectDepth(), started: time.Now()}
}

// SetTokens delivers a change by hand (pin or preview a theme): the new tokens take effect at
// once, colors start fading (24-bit only, none under reduced motion), the revision counts it.
// Tokens equal to the current ones change nothing and return false.
func (t *Theme) SetTokens(fresh Tokens) bool {
	if t.tokens.Same(&fresh) {
		return false
	}
	from := t.shown
	t.tokens = fresh
	t.revision++
	if fresh.ReduceMotion || fresh.Motion.Theme == 0 || t.depth != TrueColor {
		t.shown = fresh
		t.fade = nil
	} else {
		// Layout, sizes and strings switch now; the colors move from what is on screen.
		t.shown = MixTokens(&from, &fresh, 0)
		t.fade = &fadeState{from: from, start: time.Now(), duration: time.Duration(fresh.Motion.Theme) * time.Millisecond}
	}
	return true
}

// Tick advances the fade to now; true while it still runs (draw again soon).
func (t *Theme) Tick() bool {
	if t.fade == nil {
		return false
	}
	x := float64(time.Since(t.fade.start)) / float64(t.fade.duration)
	if x >= 1 {
		// Land exactly on the tokens.
		t.shown = t.tokens
		t.fade = nil
		return false
	}
	p := CubicBezier(t.tokens.Motion.Ease, x)
	t.shown = MixTokens(&t.fade.from, &t.tokens, p)
	return true
}

// Fading is true while a theme fade runs.
func (t *Theme) Fading() bool { return t.fade != nil }

// Tokens are the tokens in effect: after a change, the new ones (a fade's target, never a frame
// of it).
func (t *Theme) Tokens() Tokens { return t.tokens }

// Shown is what is on screen: [Theme.Tokens] with colors where the fade has them now.
func (t *Theme) Shown() *Tokens { return &t.shown }

// Revision is 0 for the tokens the app started with, plus one per change: a cache key.
func (t *Theme) Revision() uint64 { return t.revision }

// Depth is the color depth frames are written for.
func (t *Theme) Depth() Depth { return t.depth }

// SetDepth draws for this many colors (snapshots pin truecolor).
func (t *Theme) SetDepth(d Depth) { t.depth = d }

// String is the kit's string key in the display language, else fallback.
func (t *Theme) String(key, fallback string) string { return t.tokens.String(key, fallback) }

// Colors, from what is on screen.

func (t *Theme) Fg() Rgba      { return t.shown.Palette.Foreground }
func (t *Theme) Bg() Rgba      { return t.shown.Palette.Background }
func (t *Theme) Accent() Rgba  { return t.shown.Palette.Accent }
func (t *Theme) Urgent() Rgba  { return t.shown.Palette.Urgent }
func (t *Theme) Success() Rgba { return t.shown.Palette.Success }
func (t *Theme) Warning() Rgba { return t.shown.Palette.Warning }
func (t *Theme) Muted() Rgba   { return t.shown.Palette.Muted }

// Dim is secondary text (roles.textDim).
func (t *Theme) Dim() Rgba { return t.shown.Roles.TextDim }

// Faint is disabled and placeholder text (roles.textFaint).
func (t *Theme) Faint() Rgba { return t.shown.Roles.TextFaint }

// Track is tracks, rules and separators (roles.track).
func (t *Theme) Track() Rgba { return t.shown.Roles.Track }

// HotAccent is the accent with a quarter of the foreground mixed in.
func (t *Theme) HotAccent() Rgba { return t.Accent().Mix(t.Fg(), 0.25) }

// Radius is Hyprland's rounding; above 0 boxes get rounded corners.
func (t *Theme) Radius() int64 { return t.shown.Radius }
func (t *Theme) Rounded() bool { return t.shown.Radius > 0 }

func (t *Theme) ReduceMotion() bool { return t.shown.ReduceMotion }

// Clock is milliseconds since the theme was made, for component animations (spinners, the
// indeterminate bar); pinned by [Theme.SetClock] in tests. Under reduced motion components draw
// their still frame.
func (t *Theme) Clock() uint64 {
	if t.clock != nil {
		return *t.clock
	}
	return uint64(time.Since(t.started).Milliseconds())
}

// SetClock pins the animation clock (nil: the real one).
func (t *Theme) SetClock(ms *uint64) { t.clock = ms }

// Hit testing: components given a key record where they drew.

// Hit records that key was drawn at area this frame.
func (t *Theme) Hit(key string, area Rect) {
	if area.W > 0 && area.H > 0 {
		t.hits = append(t.hits, hit{key, area})
	}
}

// HitAt is the topmost key drawn at a cell this frame ("" for none).
func (t *Theme) HitAt(col, row int) string {
	for i := len(t.hits) - 1; i >= 0; i-- {
		if t.hits[i].area.Contains(col, row) {
			return t.hits[i].key
		}
	}
	return ""
}

// Find is where key was drawn this frame.
func (t *Theme) Find(key string) (Rect, bool) {
	for i := len(t.hits) - 1; i >= 0; i-- {
		if t.hits[i].key == key {
			return t.hits[i].area, true
		}
	}
	return Rect{}, false
}

// HitsLen is how many keys were recorded this frame (a scroll view notes it before drawing
// offscreen).
func (t *Theme) HitsLen() int { return len(t.hits) }

// RemapHits moves (or drops, when f returns false) the keys recorded since from.
func (t *Theme) RemapHits(from int, f func(Rect) (Rect, bool)) {
	tail := append([]hit(nil), t.hits[from:]...)
	t.hits = t.hits[:from]
	for _, h := range tail {
		if r, ok := f(h.area); ok {
			t.hits = append(t.hits, hit{h.key, r})
		}
	}
}

// ClearHits forgets last frame's keys; [Wrap] calls it before each View.
func (t *Theme) ClearHits() { t.hits = t.hits[:0] }

// Finish is the last step of every frame: cells whose background is the theme's background are
// written as the terminal's default (so terminal opacity shows), and below truecolor every color
// maps down (spec/terminal.md, Color). [Theme.Render] and the snapshot writer call it.
func (t *Theme) Finish(buf *Buffer) {
	bg := rgb3(t.Bg())
	fg := rgb3(t.Fg())
	var ink, fill []role
	if t.depth == ANSI16 {
		ink, fill = tables(&t.shown)
	}
	toRole := func(r role) Color {
		if r.index < 0 {
			return Color{}
		}
		return Indexed(uint8(r.index))
	}
	for i := range buf.Cells {
		c := &buf.Cells[i]
		if c.Bg.Kind == RGBColor && [3]uint8{c.Bg.R, c.Bg.G, c.Bg.B} == bg {
			c.Bg = Color{}
		}
		switch t.depth {
		case ANSI256:
			if c.Bg.Kind == RGBColor {
				c.Bg = Indexed(Nearest256([3]uint8{c.Bg.R, c.Bg.G, c.Bg.B}))
			}
			if c.Fg.Kind == RGBColor {
				if [3]uint8{c.Fg.R, c.Fg.G, c.Fg.B} == fg {
					c.Fg = Color{}
				} else {
					c.Fg = Indexed(Nearest256([3]uint8{c.Fg.R, c.Fg.G, c.Fg.B}))
				}
			}
		case ANSI16:
			if c.Bg.Kind == RGBColor {
				c.Bg = toRole(nearestRole([3]uint8{c.Bg.R, c.Bg.G, c.Bg.B}, fill))
			}
			if c.Fg.Kind == RGBColor {
				r := nearestRole([3]uint8{c.Fg.R, c.Fg.G, c.Fg.B}, ink)
				c.Fg = toRole(r)
				if r.faint {
					c.Attrs |= Faint
				}
			}
			// Ink the same as its fill would vanish: use the default foreground.
			if c.Fg == c.Bg && c.Fg.Kind != DefaultColor {
				c.Fg = Color{}
			}
		}
	}
}
