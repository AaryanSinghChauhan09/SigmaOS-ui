package ui

import (
	"math"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// Levels, work in progress, emptiness and outcomes (spec/terminal.md, Feedback).

// ---------------------------------------------------------------------------------------------
// Meter, ProgressBar

// Meter is a determinate level: an optional header row (label, value text at the end in
// textDim), then the bar: round(value × w) cells of `━` in the tone, the rest `━` in track;
// markers `╋` in the fill's color at round(f × (w − 1)).
type Meter struct {
	value            float32
	tone             Tone
	alarming         bool
	markers          []float32
	label, valueText string
	width            int
	segment          *[2]int
}

func NewMeter(value float32) Meter { return Meter{value: max(0, min(1, value)), width: 28} }

func (m Meter) Tone(t Tone) Meter          { m.tone = t; return m }
func (m Meter) Alarming(on bool) Meter     { m.alarming = on; return m }
func (m Meter) Markers(f ...float32) Meter { m.markers = f; return m }
func (m Meter) Label(l string) Meter       { m.label = l; return m }
func (m Meter) ValueText(v string) Meter   { m.valueText = v; return m }
func (m Meter) Width(w int) Meter          { m.width = w; return m }

func (m Meter) header() bool { return m.label != "" || m.valueText != "" }

func (m Meter) Size(*Theme) (int, int) {
	if m.header() {
		return m.width, 2
	}
	return m.width, 1
}

func (m Meter) Draw(t *Theme, area Rect, buf *Buffer) {
	y := area.Y
	if m.header() {
		omarchy.TextFit(t, buf, area.X, y, m.label, area.W, t.Fg())
		omarchy.TextRight(t, buf, area.Right(), y, m.valueText, t.Dim())
		y++
	}
	w := area.W
	k := m.tone.Color(t, t.Fg())
	if m.alarming {
		k = t.Urgent()
	}
	omarchy.HLine(t, buf, area.X, y, w, "━", t.Track())
	from, to := 0, int(roundf(m.value*float32(w)))
	if m.segment != nil {
		from, to = m.segment[0], m.segment[1]
	}
	for i := from; i < min(to, w); i++ {
		omarchy.Put(t, buf, area.X+i, y, "━", k)
	}
	for _, f := range m.markers {
		i := int(roundf(max(0, min(1, f)) * float32(sub(w, 1))))
		omarchy.Put(t, buf, area.X+i, y, "╋", k)
	}
}

// ProgressBar is a Meter in accent, plus an indeterminate mode: a segment a quarter of the track
// wide sweeping across every 1.4 s; still (reduced motion, snapshots) it sits at
// [round(3w/8), round(3w/8) + round(w/4)).
type ProgressBar struct {
	meter         Meter
	indeterminate bool
}

func NewProgressBar(value float32) ProgressBar { return ProgressBar{meter: NewMeter(value).Tone(Info)} }

// NewIndeterminateProgressBar sweeps (Motion, spec/terminal.md).
func NewIndeterminateProgressBar() ProgressBar {
	return ProgressBar{meter: NewMeter(0).Tone(Info), indeterminate: true}
}

func (p ProgressBar) Tone(t Tone) ProgressBar        { p.meter = p.meter.Tone(t); return p }
func (p ProgressBar) Label(l string) ProgressBar     { p.meter = p.meter.Label(l); return p }
func (p ProgressBar) ValueText(v string) ProgressBar { p.meter = p.meter.ValueText(v); return p }
func (p ProgressBar) Width(w int) ProgressBar        { p.meter = p.meter.Width(w); return p }

// Animating is true while it moves (ask for frames: omarchy.Animate).
func (p ProgressBar) Animating(t *Theme) bool { return p.indeterminate && !t.ReduceMotion() }

func (p ProgressBar) Size(t *Theme) (int, int) { return p.meter.Size(t) }

func (p ProgressBar) Draw(t *Theme, area Rect, buf *Buffer) {
	if !p.indeterminate {
		p.meter.Draw(t, area, buf)
		return
	}
	w := float64(area.W)
	seg := math.Round(w / 4)
	var start float64
	if t.ReduceMotion() {
		start = math.Round(3 * w / 8)
	} else {
		x := float64(t.Clock()%1400) / 1400
		e := omarchy.CubicBezier([4]float64{0.42, 0, 0.58, 1}, x)
		start = math.Round(-seg + (w+seg)*e)
	}
	from := int(math.Max(start, 0))
	to := int(clamp(start+seg, 0, w))
	m := NewMeter(0).Tone(p.meter.tone).Width(area.W).Label(p.meter.label).ValueText(p.meter.valueText)
	m.segment = &[2]int{from, to}
	m.Draw(t, area, buf)
}

// ---------------------------------------------------------------------------------------------
// Spinner

// Spinner is busy. Turning: the braille frames, one per 90 ms (a turn in 900 ms); still (reduced
// motion, snapshots) the first. Pulse: the glyph (󰦖) breathing between half and full opacity,
// 420 ms each way; still at half.
type Spinner struct {
	pulse bool
	glyph string
	color *Rgba
}

const (
	SpinnerTurnMs  = 900
	SpinnerPulseMs = 840
)

func NewSpinner() Spinner { return Spinner{glyph: GlyphLoading} }

func (s Spinner) Pulse(on bool) Spinner  { s.pulse = on; return s }
func (s Spinner) Glyph(g string) Spinner { s.glyph = g; return s }
func (s Spinner) Color(c Rgba) Spinner   { s.color = &c; return s }

// SpinnerFrame is the turning frame at clock ms.
func SpinnerFrame(clock uint64) int { return int((clock % SpinnerTurnMs) / 90) }

// SpinnerOpacity is the pulse's opacity at clock ms: 0.5 → 1 → 0.5, eased in and out.
func SpinnerOpacity(clock uint64) float64 {
	p := float64(clock%SpinnerPulseMs) / 420
	x := p
	if p > 1 {
		x = 2 - p
	}
	return 0.5 + 0.5*omarchy.CubicBezier([4]float64{0.42, 0, 0.58, 1}, x)
}

func (s Spinner) Size(*Theme) (int, int) { return 1, 1 }

func (s Spinner) Draw(t *Theme, area Rect, buf *Buffer) {
	k := t.Fg()
	if s.color != nil {
		k = *s.color
	}
	still := t.ReduceMotion()
	if s.pulse {
		o := 0.5
		if !still {
			o = SpinnerOpacity(t.Clock())
		}
		omarchy.Put(t, buf, area.X, area.Y, s.glyph, k.WithAlpha(o))
	} else {
		f := 0
		if !still {
			f = SpinnerFrame(t.Clock())
		}
		omarchy.Put(t, buf, area.X, area.Y, SpinnerFrames[f], k)
	}
}

// ---------------------------------------------------------------------------------------------
// EmptyState

// EmptyState is what a list or panel shows when it has nothing: a centered column of glyph,
// title, hint and a bordered action Button (keyed "empty-action"), a blank row before the action.
// Panel: glyph and title textDim, hint textFaint. Overlay: glyph menu.selectedText at 0.8, title
// foreground at 0.7, hint textDim.
type EmptyState struct {
	title, glyph, hint, action string
	overlay, cursor            bool
	width                      int
}

func NewEmptyState(title string) EmptyState { return EmptyState{title: title, width: 44} }

func (e EmptyState) Glyph(g string) EmptyState  { e.glyph = g; return e }
func (e EmptyState) Hint(h string) EmptyState   { e.hint = h; return e }
func (e EmptyState) Action(a string) EmptyState { e.action = a; return e }
func (e EmptyState) Overlay(on bool) EmptyState { e.overlay = on; return e }
func (e EmptyState) Cursor(on bool) EmptyState  { e.cursor = on; return e }
func (e EmptyState) Width(w int) EmptyState     { e.width = w; return e }

func (e EmptyState) lines() ([]string, []string) {
	var hint []string
	if e.hint != "" {
		hint = omarchy.WrapText(e.hint, e.width)
	}
	return omarchy.WrapText(e.title, e.width), hint
}

func (e EmptyState) Size(*Theme) (int, int) {
	title, hint := e.lines()
	h := len(title) + len(hint)
	if e.glyph != "" {
		h++
	}
	if e.action != "" {
		h += 2
	}
	return e.width, h
}

func (e EmptyState) Draw(t *Theme, area Rect, buf *Buffer) {
	s := t.Shown()
	glyphInk, titleInk, hintInk := t.Dim(), t.Dim(), t.Faint()
	if e.overlay {
		glyphInk, titleInk, hintInk = s.Surfaces.Menu.SelectedText.WithAlpha(0.8), t.Fg().WithAlpha(0.7), t.Dim()
	}
	y := area.Y
	if e.glyph != "" {
		omarchy.TextCenter(t, buf, area.X, y, area.W, e.glyph, glyphInk)
		y++
	}
	title, hint := e.lines()
	for _, l := range title {
		omarchy.TextCenter(t, buf, area.X, y, area.W, l, titleInk)
		y++
	}
	for _, l := range hint {
		omarchy.TextCenter(t, buf, area.X, y, area.W, l, hintInk)
		y++
	}
	if e.action != "" {
		b := NewButton(e.action).Bordered(true).Cursor(e.cursor).Key("empty-action")
		bw, _ := b.Size(t)
		b.Draw(t, rect(area.X+sub(area.W, bw)/2, y+1, bw, 1), buf)
	}
}

// ---------------------------------------------------------------------------------------------
// Banner

// Banner is an inline message, one row: the control frame with its edges in the tone, the normal
// fill, the tone glyph, the text, then an optional bordered action Button ("banner-action") and a
// dismiss 󰅖 ("banner-dismiss").
type Banner struct {
	text, glyph, action string
	tone                Tone
	dismissible, cursor bool
	width               int
}

func NewBanner(text string) Banner { return Banner{text: text, tone: Info, width: 92} }

func (b Banner) Tone(t Tone) Banner         { b.tone = t; return b }
func (b Banner) Glyph(g string) Banner      { b.glyph = g; return b }
func (b Banner) Action(a string) Banner     { b.action = a; return b }
func (b Banner) Dismissible(on bool) Banner { b.dismissible = on; return b }
func (b Banner) Cursor(on bool) Banner      { b.cursor = on; return b }
func (b Banner) Width(w int) Banner         { b.width = w; return b }

func (b Banner) toneGlyph() string {
	if b.glyph != "" {
		return b.glyph
	}
	switch b.tone {
	case Success:
		return "\U000f012c"
	case Warning:
		return "\U000f002a"
	case Urgent:
		return "\U000f015a"
	}
	return "\U000f02fd"
}

func (b Banner) Size(*Theme) (int, int) { return b.width, 1 }

func (b Banner) Draw(t *Theme, area Rect, buf *Buffer) {
	tone := b.tone
	if tone == Neutral {
		tone = Info
	}
	k := tone.Color(t, t.Accent())
	control(t, buf, area.X, area.Y, area.W, Look{Fill: t.Shown().Controls.Normal.Fill, Border: &k, Text: t.Fg()})
	omarchy.Put(t, buf, area.X+2, area.Y, b.toneGlyph(), k)
	right := sub(area.Right(), 2)
	if b.dismissible {
		right = sub(right, 1)
		omarchy.Put(t, buf, right, area.Y, GlyphClose, t.Dim())
		t.Hit("banner-dismiss", rect(right, area.Y, 1, 1))
		right = sub(right, 1)
	}
	if b.action != "" {
		btn := NewButton(b.action).Bordered(true).Cursor(b.cursor).Key("banner-action")
		bw, _ := btn.Size(t)
		right = sub(right, bw)
		btn.Draw(t, rect(right, area.Y, bw, 1), buf)
		right = sub(right, 1)
	}
	omarchy.TextFit(t, buf, area.X+4, area.Y, b.text, sub(right, area.X+4), t.Fg())
}

// ---------------------------------------------------------------------------------------------
// Toast

// Toast is a transient message on the notifications surface: a box in its border color, the
// glyph, the bold title over the text (notifications text darker 1.15), an action Button
// ("toast-action") at the end of the first row; the countdown is the bottom edge: its first
// round(countdown × (w − 2)) cells `━` in notifications.countdown. Four rows (three without
// text).
type Toast struct {
	title, text, glyph, action string
	countdown                  float32
	hasCountdown, cursor       bool
	width                      int
}

func NewToast(title string) Toast { return Toast{title: title, width: 45} }

func (n Toast) Text(s string) Toast       { n.text = s; return n }
func (n Toast) Glyph(g string) Toast      { n.glyph = g; return n }
func (n Toast) Action(a string) Toast     { n.action = a; return n }
func (n Toast) Countdown(c float32) Toast { n.countdown, n.hasCountdown = c, true; return n }
func (n Toast) Cursor(on bool) Toast      { n.cursor = on; return n }
func (n Toast) Width(w int) Toast         { n.width = w; return n }

func (n Toast) Size(*Theme) (int, int) {
	if n.text == "" {
		return n.width, 3
	}
	return n.width, 4
}

func (n Toast) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Clear(buf, area)
	s := &t.Shown().Surfaces.Notifications
	inner := omarchy.Frame(t, buf, area, s.Border.Color, "", Ink{})
	if n.cursor {
		omarchy.Fill(t, buf, inner, t.Shown().Controls.Hover.Fill)
	}
	x := inner.X + 1
	if n.glyph != "" {
		omarchy.Put(t, buf, x, inner.Y, n.glyph, s.Text)
		x += 2
	}
	right := sub(inner.Right(), 1)
	if n.action != "" {
		b := NewButton(n.action).Bordered(true).Key("toast-action")
		bw, _ := b.Size(t)
		right = sub(right, bw)
		b.Draw(t, rect(right, inner.Y, bw, 1), buf)
		right = sub(right, 1)
	}
	omarchy.TextFit(t, buf, x, inner.Y, n.title, sub(right, x), bold(s.Text, true))
	if n.text != "" {
		omarchy.TextFit(t, buf, x, inner.Y+1, n.text, sub(right, x), s.Text.Darker(1.15))
	}
	if n.hasCountdown && n.countdown >= 0 {
		w := sub(area.W, 2)
		k := int(roundf(max(0, min(1, n.countdown)) * float32(w)))
		omarchy.HLine(t, buf, area.X+1, area.Bottom()-1, k, "━", s.Countdown)
	}
}
