// Package ui has every component of spec/components.md under the same names, drawn on the cell
// grid as spec/terminal.md maps them. Each is a value built like a lipgloss.Style
// (ui.NewButton("Save").Bordered(true)) and an omarchy.Component: theme.Render(c) makes it a
// string for a View. Components are views of your model's state (the model owns selection,
// cursors and values); the key handlers that move that state live beside them (CursorModel,
// HandleMenuKey, ...), taking Bubble Tea's tea.KeyPressMsg.
package ui

import (
	"unicode"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

type (
	Theme     = omarchy.Theme
	Rect      = omarchy.Rect
	Buffer    = omarchy.Buffer
	Rgba      = omarchy.Rgba
	Ink       = omarchy.Ink
	Component = omarchy.Component
)

// None is "no index": no cursor, nothing selected.
const None = -1

// If is i when on, else None: `.Cursor(ui.If(focused, m.align))`.
func If(on bool, i int) int {
	if on {
		return i
	}
	return None
}

// Tone is a component's color by meaning: neutral (its own ink), accent, success, warning, urgent,
// muted. The feedback components' names are aliases: Info is accent, Error urgent.
type Tone int

const (
	Neutral Tone = iota
	Accent
	Success
	Warning
	Urgent
	Muted
)

const (
	Info  = Accent
	Error = Urgent
)

// Color is the tone's color; neutral for Neutral.
func (tone Tone) Color(t *Theme, neutral Rgba) Rgba {
	switch tone {
	case Accent:
		return t.Accent()
	case Success:
		return t.Success()
	case Warning:
		return t.Warning()
	case Urgent:
		return t.Urgent()
	case Muted:
		return t.Muted()
	}
	return neutral
}

// Look is how a control looks in its state (spec/terminal.md, States): the fill over its cells,
// the color of its edges (nil: no edges shown), its label ink and weight.
type Look struct {
	Fill   Rgba
	Border *Rgba
	Text   Rgba
	Bold   bool
}

func borderOf(b omarchy.BorderSpec) *Rgba {
	if b.None() {
		return nil
	}
	c := b.Color
	return &c
}

func some(c Rgba) *Rgba { return &c }

// LookOf is pressed > focus > hover-cursor > selected > normal, for a control that shows the
// normal fill (restFill) and border (restBorder) at rest.
func LookOf(t *Theme, hot, selected, restFill, restBorder bool) Look {
	c := &t.Shown().Controls
	if hot {
		text := t.Fg()
		if selected {
			text = c.Selected.Text
		}
		return Look{Fill: c.Hover.Fill, Border: borderOf(c.Hover.Border), Text: text, Bold: selected}
	}
	if selected {
		border := borderOf(c.Selected.Border)
		if border == nil && restBorder {
			border = borderOf(c.Normal.Border)
		}
		return Look{Fill: c.Selected.Fill, Border: border, Text: c.Selected.Text, Bold: true}
	}
	l := Look{Text: t.Fg()}
	if restFill {
		l.Fill = c.Normal.Fill
	}
	if restBorder {
		l.Border = borderOf(c.Normal.Border)
	}
	return l
}

// control paints a control's frame on row y over w cells: the fill across them, the edges on its
// first and last.
func control(t *Theme, buf *Buffer, x, y, w int, l Look) {
	if w < 2 {
		return
	}
	omarchy.Fill(t, buf, omarchy.R(x, y, w, 1), l.Fill)
	omarchy.Edges(t, buf, x, y, w, l.Border)
}

// Glyphs the components share.
const (
	GlyphCheck        = "✓"
	GlyphDot          = "●"
	GlyphChevronDown  = "\U000f0140"
	GlyphChevronRight = "\U000f0142"
	GlyphChevronLeft  = "\U000f0141"
	GlyphChevronUp    = "\U000f0143"
	GlyphSubmenu      = ""
	GlyphClose        = "\U000f0156"
	GlyphLoading      = "\U000f0996"
)

// SpinnerFrames are a turning spinner's frames, one per 90 ms (900 ms a turn).
var SpinnerFrames = [10]string{"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"}

func sub(a, b int) int { return max(0, a-b) }

func ink(c Rgba) Ink           { return omarchy.InkOf(c) }
func bold(c Rgba, b bool) Ink  { return omarchy.InkOf(c).WithBold(b) }
func rect(x, y, w, h int) Rect { return omarchy.R(x, y, w, h) }
func width(s string) int       { return omarchy.Width(s) }
func clamp(v, lo, hi float64) float64 {
	return max(lo, min(hi, v))
}

// keyOf is a key press's name as Bubble Tea spells it ("down", "j", "shift+tab", "space").
func keyOf(msg tea.KeyPressMsg) string { return msg.String() }

// Printable is the text a key types (type-ahead filters), or "" for a non-printable key, a
// space, or a key with Ctrl, Alt or Super.
func Printable(msg tea.KeyPressMsg) string {
	if msg.Mod&(tea.ModCtrl|tea.ModAlt|tea.ModSuper|tea.ModMeta|tea.ModHyper) != 0 {
		return ""
	}
	for _, r := range msg.Text {
		if unicode.IsControl(r) || unicode.IsSpace(r) {
			return ""
		}
	}
	return msg.Text
}
