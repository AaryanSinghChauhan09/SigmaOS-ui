package omarchy

import "math"

// CubicBezier is `cubic-bezier(x1, y1, x2, y2)` at time x (0–1): the curve's y where its x is x,
// solved by Newton steps with a bisection fallback, as browsers do.
func CubicBezier(c [4]float64, x float64) float64 {
	x = math.Max(0, math.Min(1, x))
	if x == 0 || x == 1 {
		return x
	}
	x1, y1, x2, y2 := c[0], c[1], c[2], c[3]
	coord := func(t, a, b float64) float64 {
		u := 1 - t
		return 3*u*u*t*a + 3*u*t*t*b + t*t*t
	}
	slope := func(t, a, b float64) float64 {
		u := 1 - t
		return 3*u*u*a + 6*u*t*(b-a) + 3*t*t*(1-b)
	}
	t := x
	for range 8 {
		err := coord(t, x1, x2) - x
		if math.Abs(err) < 1e-7 {
			return coord(t, y1, y2)
		}
		d := slope(t, x1, x2)
		if math.Abs(d) < 1e-6 {
			break
		}
		t -= err / d
	}
	lo, hi := 0.0, 1.0
	t = x
	for range 40 {
		v := coord(t, x1, x2)
		if math.Abs(v-x) < 1e-7 {
			break
		}
		if v < x {
			lo = t
		} else {
			hi = t
		}
		t = (lo + hi) / 2
	}
	return coord(t, y1, y2)
}

func mixBorder(a, b BorderSpec, p float64) BorderSpec {
	out := BorderSpec{Color: a.Color.Mix(b.Color, p), Widths: b.Widths, Gradient: b.Gradient}
	if a.Gradient != nil && b.Gradient != nil && len(a.Gradient.Colors) == len(b.Gradient.Colors) {
		g := Gradient{Angle: b.Gradient.Angle, Colors: make([]Rgba, len(b.Gradient.Colors))}
		for i := range g.Colors {
			g.Colors[i] = a.Gradient.Colors[i].Mix(b.Gradient.Colors[i], p)
		}
		out.Gradient = &g
	}
	return out
}

func mixSurface(a, b Surface, p float64) Surface {
	return Surface{Background: a.Background.Mix(b.Background, p), Text: a.Text.Mix(b.Text, p), Border: mixBorder(a.Border, b.Border, p)}
}

func mixState(a, b State, p float64) State {
	return State{Fill: a.Fill.Mix(b.Fill, p), Text: a.Text.Mix(b.Text, p), Border: mixBorder(a.Border, b.Border, p)}
}

// MixTokens is `to` with every color p of the way from `from`'s (0 is from's colors, 1 exactly
// to): the theme fade's frame (spec/following.md). Everything that isn't a color is to's.
func MixTokens(from, to *Tokens, p float64) Tokens {
	t := *to
	if p >= 1 {
		return t
	}
	a, b := &from.Palette, &t.Palette
	b.Foreground = a.Foreground.Mix(b.Foreground, p)
	b.Background = a.Background.Mix(b.Background, p)
	b.Accent = a.Accent.Mix(b.Accent, p)
	b.Urgent = a.Urgent.Mix(b.Urgent, p)
	b.Muted = a.Muted.Mix(b.Muted, p)
	b.Success = a.Success.Mix(b.Success, p)
	b.Warning = a.Warning.Mix(b.Warning, p)
	named := make(map[string]Rgba, len(to.Palette.Named))
	for k, v := range to.Palette.Named {
		if old, ok := from.Palette.Named[k]; ok {
			v = old.Mix(v, p)
		}
		named[k] = v
	}
	b.Named = named
	r, rr := &from.Roles, &t.Roles
	rr.TextDim = r.TextDim.Mix(rr.TextDim, p)
	rr.TextFaint = r.TextFaint.Mix(rr.TextFaint, p)
	rr.Track = r.Track.Mix(rr.Track, p)
	rr.Scrim = r.Scrim.Mix(rr.Scrim, p)
	c, cc := &from.Controls, &t.Controls
	cc.Normal = mixState(c.Normal, cc.Normal, p)
	cc.Hover = mixState(c.Hover, cc.Hover, p)
	cc.Focus = mixState(c.Focus, cc.Focus, p)
	cc.Selected = mixState(c.Selected, cc.Selected, p)
	cc.Pressed.Fill = c.Pressed.Fill.Mix(cc.Pressed.Fill, p)
	cc.Selection.Fill = c.Selection.Fill.Mix(cc.Selection.Fill, p)
	s, ss := &from.Surfaces, &t.Surfaces
	ss.Popups = mixSurface(s.Popups, ss.Popups, p)
	ss.Tooltip = mixSurface(s.Tooltip, ss.Tooltip, p)
	ss.Menu.Surface = mixSurface(s.Menu.Surface, ss.Menu.Surface, p)
	ss.Menu.Scrim = s.Menu.Scrim.Mix(ss.Menu.Scrim, p)
	ss.Menu.SelectedBackground = s.Menu.SelectedBackground.Mix(ss.Menu.SelectedBackground, p)
	ss.Menu.SelectedText = s.Menu.SelectedText.Mix(ss.Menu.SelectedText, p)
	ss.Menu.SelectedBorder = mixBorder(s.Menu.SelectedBorder, ss.Menu.SelectedBorder, p)
	ss.Notifications.Surface = mixSurface(s.Notifications.Surface, ss.Notifications.Surface, p)
	ss.Notifications.Countdown = s.Notifications.Countdown.Mix(ss.Notifications.Countdown, p)
	ss.Polkit.Surface = mixSurface(s.Polkit.Surface, ss.Polkit.Surface, p)
	ss.Polkit.TextError = s.Polkit.TextError.Mix(ss.Polkit.TextError, p)
	ss.Polkit.BorderError = mixBorder(s.Polkit.BorderError, ss.Polkit.BorderError, p)
	ss.Polkit.Accent = s.Polkit.Accent.Mix(ss.Polkit.Accent, p)
	ss.Polkit.Scrim = s.Polkit.Scrim.Mix(ss.Polkit.Scrim, p)
	return t
}
