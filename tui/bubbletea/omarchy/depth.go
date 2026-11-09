package omarchy

import (
	"os"
	"strings"
)

// Depth is how many colors the terminal shows (spec/terminal.md, Color). Every component paints
// 24-bit colors; [Theme.Finish] writes the page background as the terminal's default and, below
// truecolor, maps every color down.
type Depth int

const (
	// TrueColor is 24-bit SGR (`38;2;r;g;b`): every Omarchy terminal.
	TrueColor Depth = iota
	// ANSI256 is xterm's 256 colors (`38;5;n`), indices 16–255 only.
	ANSI256
	// ANSI16 is the 16 ANSI colors (30–37, 90–97), by role.
	ANSI16
)

func (d Depth) String() string {
	switch d {
	case ANSI256:
		return "256"
	case ANSI16:
		return "16"
	}
	return "24bit"
}

// DetectDepth reads `OMARCHY_UI_COLORS` (`24bit`, `truecolor`, `256`, `16`) when set; else
// truecolor when `COLORTERM` is `truecolor` or `24bit`; else 256 when `TERM` mentions `256color`;
// else 16.
func DetectDepth() Depth { return DepthFromEnv(os.Getenv) }

// DepthFromEnv is [DetectDepth] over any environment lookup.
func DepthFromEnv(env func(string) string) Depth {
	switch strings.TrimSpace(env("OMARCHY_UI_COLORS")) {
	case "24bit", "truecolor", "24":
		return TrueColor
	case "256":
		return ANSI256
	case "16", "8":
		return ANSI16
	}
	switch env("COLORTERM") {
	case "truecolor", "24bit":
		return TrueColor
	}
	if strings.Contains(env("TERM"), "256color") {
		return ANSI256
	}
	return ANSI16
}

// Xterm256 is the xterm-256 color at index i (16–255): the 6×6×6 cube, then the 24 grays.
func Xterm256(i uint8) [3]uint8 {
	levels := [6]uint8{0, 95, 135, 175, 215, 255}
	if i >= 232 {
		v := 8 + 10*(i-232)
		return [3]uint8{v, v, v}
	}
	n := uint8(0)
	if i > 16 {
		n = i - 16
	}
	return [3]uint8{levels[n/36], levels[n/6%6], levels[n%6]}
}

func dist(a, b [3]uint8) uint32 {
	d := func(x, y uint8) uint32 { v := int32(x) - int32(y); return uint32(v * v) }
	return d(a[0], b[0]) + d(a[1], b[1]) + d(a[2], b[2])
}

// Nearest256 is the xterm-256 index in 16–255 nearest to c by squared sRGB distance; ties go to
// the lower index.
func Nearest256(c [3]uint8) uint8 {
	best, bi := uint32(1<<32-1), uint8(16)
	for i := 16; i <= 255; i++ {
		if d := dist(c, Xterm256(uint8(i))); d < best {
			best, bi = d, uint8(i)
		}
	}
	return bi
}

// role is one entry of the 16-color tables: its color in this theme and the ANSI index it becomes
// (-1: the terminal's default), with faint for textFaint.
type role struct {
	rgb   [3]uint8
	index int
	faint bool
}

func rgb3(c Rgba) [3]uint8 { return [3]uint8{c.R, c.G, c.B} }

// The 16-color tables (spec/terminal.md): ink roles in order, then fill roles. Earlier entries
// win exact ties.
func tables(t *Tokens) (ink, fill []role) {
	bg := t.Palette.Background
	r := func(c Rgba, index int) role { return role{rgb: rgb3(Over(c, bg)), index: index} }
	const def, black, red, green, yellow, blue, gray, darkGray = -1, 0, 1, 2, 3, 4, 7, 8
	faint := r(t.Roles.TextFaint, darkGray)
	faint.faint = true
	ink = []role{
		r(t.Palette.Foreground, def),
		r(t.Roles.TextDim, darkGray),
		faint,
		r(t.Palette.Muted, darkGray),
		r(t.Palette.Accent, blue),
		r(t.Palette.Urgent, red),
		r(t.Palette.Success, green),
		r(t.Palette.Warning, yellow),
		r(t.Palette.Background, black),
		r(t.Controls.Normal.Border.Color, darkGray),
		r(t.Controls.Hover.Border.Color, darkGray),
		r(t.Roles.Track, darkGray),
	}
	fill = []role{
		r(t.Palette.Background, def),
		r(t.Controls.Normal.Fill, def),
		r(t.Controls.Selected.Fill, def),
		r(t.Controls.Hover.Fill, darkGray),
		r(t.Controls.Focus.Fill, darkGray),
		r(t.Controls.Pressed.Fill, darkGray),
		r(t.Palette.Accent, blue),
		r(t.Palette.Urgent, red),
		r(t.Palette.Success, green),
		r(t.Palette.Warning, yellow),
		r(t.Palette.Foreground, gray),
	}
	return
}

func nearestRole(c [3]uint8, roles []role) role {
	best, br := uint32(1<<32-1), roles[0]
	for _, r := range roles {
		if d := dist(c, r.rgb); d < best {
			best, br = d, r
		}
	}
	return br
}
