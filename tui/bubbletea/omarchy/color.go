package omarchy

import (
	"encoding/json"
	"fmt"
	"math"
	"strconv"
	"strings"
)

// Rgba is an 8-bit sRGB color with straight alpha, as the core resolves it. It implements
// image/color.Color, so it goes straight into Lip Gloss (lipgloss.NewStyle().Foreground(t.Dim())).
type Rgba struct{ R, G, B, A uint8 }

// Transparent is no color at all.
var Transparent = Rgba{}

// RGB is an opaque color.
func RGB(r, g, b uint8) Rgba { return Rgba{r, g, b, 255} }

// RGBA implements image/color.Color (alpha-premultiplied 16-bit channels).
func (c Rgba) RGBA() (r, g, b, a uint32) {
	a = uint32(c.A) * 0x101
	r = uint32(c.R) * 0x101 * a / 0xffff
	g = uint32(c.G) * 0x101 * a / 0xffff
	b = uint32(c.B) * 0x101 * a / 0xffff
	return
}

// Hex is `#rrggbbaa`, lowercase: the canonical form in token dumps.
func (c Rgba) Hex() string { return fmt.Sprintf("#%02x%02x%02x%02x", c.R, c.G, c.B, c.A) }

func (c Rgba) String() string { return c.Hex() }

// AlphaF is the alpha as 0–1 (in float32, as the core's `alpha_f`).
func (c Rgba) AlphaF() float32 { return float32(c.A) / 255.0 }

func clampAlpha(v float64) float64 {
	if math.IsNaN(v) || math.IsInf(v, 0) {
		return 0
	}
	return math.Max(0, math.Min(1, v))
}

func alphaByte(alpha float64) uint8 { return uint8(math.Round(clampAlpha(alpha) * 255)) }

// WithAlpha is the same color at `alpha` (0–1), replacing its own.
func (c Rgba) WithAlpha(alpha float64) Rgba { c.A = alphaByte(alpha); return c }

// TimesAlpha multiplies the color's own alpha by `alpha` (0–1).
func (c Rgba) TimesAlpha(alpha float64) Rgba {
	c.A = alphaByte(float64(c.A) / 255.0 * alpha)
	return c
}

// Mix moves toward `other` by `t` (0 keeps c, 1 is other), per channel in sRGB, alpha included:
// the core's `Rgba::mix`.
func (c Rgba) Mix(other Rgba, t float64) Rgba {
	t = math.Max(0, math.Min(1, t))
	m := func(a, b uint8) uint8 { return uint8(math.Round(float64(a) + (float64(b)-float64(a))*t)) }
	return Rgba{m(c.R, other.R), m(c.G, other.G), m(c.B, other.B), m(c.A, other.A)}
}

// Over composites `top` over the opaque `under`, per 8-bit channel: `(t·a + u·(255 − a) + 127) /
// 255`, exactly the core's mix by a / 255 (spec/terminal.md, Compositing). The result is opaque.
func Over(top, under Rgba) Rgba {
	a := uint32(top.A)
	ch := func(t, u uint8) uint8 { return uint8((uint32(t)*a + uint32(u)*(255-a) + 127) / 255) }
	return Rgba{ch(top.R, under.R), ch(top.G, under.G), ch(top.B, under.B), 255}
}

// Darker is Qt's `QColor::darker(round(factor × 100))`, which the component specs ask for
// ("foreground darker 1.25"): the HSV value divided with Qt's 16-bit channels and float math,
// step for step as the core ports it (so halves round the way Qt rounds them).
func (c Rgba) Darker(factor float64) Rgba {
	f := int64(math.Round(factor * 100))
	if f <= 0 || f < 100 {
		return c
	}
	const max = float32(65535.0)
	qRound := func(d float32) int64 {
		if d >= 0 {
			return int64(float32(d + 0.5))
		}
		return int64(float32(d - 0.5))
	}
	abs := func(x float32) float32 {
		if x < 0 {
			return -x
		}
		return x
	}
	min32 := func(a, b float32) float32 {
		if a < b {
			return a
		}
		return b
	}
	max32 := func(a, b float32) float32 {
		if a > b {
			return a
		}
		return b
	}
	fuzzyEq := func(a, b float32) bool { return float32(abs(a-b)*100000.0) <= min32(abs(a), abs(b)) }
	r := float32(float32(c.R)*257.0) / max
	g := float32(float32(c.G)*257.0) / max
	b := float32(float32(c.B)*257.0) / max
	mx := max32(max32(r, g), b)
	mn := min32(min32(r, g), b)
	delta := float32(mx - mn)
	value := uint32(qRound(float32(mx * max)))
	var hue, sat int64
	if abs(delta) <= 0.00001 {
		hue, sat = 65535, 0
	} else {
		sat = qRound(float32(float32(delta/mx) * max))
		var h float32
		switch {
		case fuzzyEq(r, mx):
			h = float32(g-b) / delta
		case fuzzyEq(g, mx):
			h = float32(2.0 + float32(float32(b-r)/delta))
		default:
			h = float32(4.0 + float32(float32(r-g)/delta))
		}
		h = float32(h * 60.0)
		if h < 0 {
			h = float32(h + 360.0)
		}
		hue = qRound(float32(h * 100.0))
	}
	v16 := (value * 100) / uint32(f)
	var h float32
	if hue != 65535 {
		h = float32(hue) / 6000.0
	}
	s := float32(sat) / max
	v := float32(v16) / max
	var rr, gg, bb float32
	if s == 0 {
		rr, gg, bb = v, v, v
	} else {
		i := int32(h)
		fr := float32(h - float32(i))
		p := float32(v * float32(1.0-s))
		if i&1 == 1 {
			q := float32(v * float32(1.0-float32(s*fr)))
			switch i {
			case 1:
				rr, gg, bb = q, v, p
			case 3:
				rr, gg, bb = p, q, v
			default:
				rr, gg, bb = v, p, q
			}
		} else {
			t := float32(v * float32(1.0-float32(s*float32(1.0-fr))))
			switch i {
			case 0:
				rr, gg, bb = v, t, p
			case 2:
				rr, gg, bb = p, v, t
			default:
				rr, gg, bb = t, p, v
			}
		}
	}
	byteOf := func(x float32) uint8 {
		c16 := qRound(float32(x * max))
		if c16 < 0 {
			c16 = 0
		}
		if c16 > 65535 {
			c16 = 65535
		}
		return uint8(math.Round(float64(c16) / 65535.0 * 255.0))
	}
	return Rgba{byteOf(rr), byteOf(gg), byteOf(bb), c.A}
}

// ParseHex reads `#rgb`, `#rrggbb` or `#rrggbbaa`.
func ParseHex(s string) (Rgba, bool) {
	h, ok := strings.CutPrefix(strings.TrimSpace(s), "#")
	if !ok {
		return Rgba{}, false
	}
	byteAt := func(x string) (uint8, bool) {
		n, err := strconv.ParseUint(x, 16, 8)
		return uint8(n), err == nil
	}
	switch len(h) {
	case 3:
		var out [3]uint8
		for i := range 3 {
			v, ok := byteAt(h[i:i+1] + h[i:i+1])
			if !ok {
				return Rgba{}, false
			}
			out[i] = v
		}
		return RGB(out[0], out[1], out[2]), true
	case 6, 8:
		var out [4]uint8
		out[3] = 255
		for i := 0; i < len(h)/2; i++ {
			v, ok := byteAt(h[2*i : 2*i+2])
			if !ok {
				return Rgba{}, false
			}
			out[i] = v
		}
		return Rgba{out[0], out[1], out[2], out[3]}, true
	}
	return Rgba{}, false
}

func (c Rgba) MarshalJSON() ([]byte, error) { return json.Marshal(c.Hex()) }

func (c *Rgba) UnmarshalJSON(b []byte) error {
	var s string
	if err := json.Unmarshal(b, &s); err != nil {
		return err
	}
	v, ok := ParseHex(s)
	if !ok {
		return fmt.Errorf("not a color: %q", s)
	}
	*c = v
	return nil
}
