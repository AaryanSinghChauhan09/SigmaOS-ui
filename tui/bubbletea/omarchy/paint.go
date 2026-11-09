package omarchy

import (
	"strings"
	"unicode"

	"github.com/mattn/go-runewidth"
)

// Cell painting in the kit's terms (spec/terminal.md): colors with alpha composite over the cell
// they land on (its painted background, else the theme background), box corners follow the
// radius, controls frame themselves with eighth-block edges. Every component draws through these;
// an app's own drawing can too.

const (
	// EdgeLeft is a control's left edge: U+258F LEFT ONE EIGHTH BLOCK.
	EdgeLeft = "▏"
	// EdgeRight is a control's right edge: U+2595 RIGHT ONE EIGHTH BLOCK.
	EdgeRight = "▕"
	// Ellipsis ends elided text.
	Ellipsis = "…"
)

// widths: unicode-width's (spec/terminal.md, Widths): East Asian ambiguous is 1, Nerd Font glyphs
// (private use) are 1, marks 0.
var widths = func() *runewidth.Condition {
	c := runewidth.NewCondition()
	c.EastAsianWidth = false
	c.StrictEmojiNeutral = true
	return c
}()

// RuneWidth is a character's width in cells (0 for marks and controls).
func RuneWidth(r rune) int {
	if r < 0x20 || (r >= 0x7f && r < 0xa0) {
		return 0
	}
	return widths.RuneWidth(r)
}

// Width is a string's display width in cells.
func Width(s string) int {
	n := 0
	for _, r := range s {
		n += RuneWidth(r)
	}
	return n
}

// Fit is s cut to w cells: the longest prefix that fits in w − 1 and "…", when it had to be cut.
func Fit(s string, w int) string {
	if Width(s) <= w {
		return s
	}
	if w <= 0 {
		return ""
	}
	var out strings.Builder
	used := 0
	for _, r := range s {
		cw := RuneWidth(r)
		if used+cw > w-1 {
			break
		}
		out.WriteRune(r)
		used += cw
	}
	out.WriteString(Ellipsis)
	return out.String()
}

// Ink is a color (alpha composites over the cell's background) and attributes.
type Ink struct {
	Color                   Rgba
	Bold, Italic, Underline bool
}

// InkOf is plain ink in a color.
func InkOf(c Rgba) Ink { return Ink{Color: c} }

// WithBold is the ink, bold or not.
func (i Ink) WithBold(b bool) Ink { i.Bold = b; return i }

func (i Ink) attrs() Attrs {
	var a Attrs
	if i.Bold {
		a |= Bold
	}
	if i.Italic {
		a |= Italic
	}
	if i.Underline {
		a |= Underline
	}
	return a
}

// Inkable is a color or an Ink.
type Inkable interface{ ink() Ink }

func (c Rgba) ink() Ink { return Ink{Color: c} }
func (i Ink) ink() Ink  { return i }

// Under is the background a cell shows now: what was painted there, else the theme background.
func Under(t *Theme, buf *Buffer, x, y int) Rgba {
	if c := buf.At(x, y); c != nil && c.Bg.Kind == RGBColor {
		return RGB(c.Bg.R, c.Bg.G, c.Bg.B)
	}
	return t.Bg()
}

// Fill composites c over the background of every cell in area (the characters stay).
func Fill(t *Theme, buf *Buffer, area Rect, c Rgba) {
	if c.A == 0 {
		return
	}
	area = area.Intersect(buf.Area)
	for y := area.Y; y < area.Bottom(); y++ {
		for x := area.X; x < area.Right(); x++ {
			u := Under(t, buf, x, y)
			buf.At(x, y).Bg = CellRGB(Over(c, u))
		}
	}
}

// Clear blanks the cells of area: spaces on the theme background (a surface drawn over something).
func Clear(buf *Buffer, area Rect) {
	area = area.Intersect(buf.Area)
	for y := area.Y; y < area.Bottom(); y++ {
		for x := area.X; x < area.Right(); x++ {
			buf.At(x, y).Reset()
		}
	}
}

// Put sets one symbol (a character and any marks on it) at a cell, in ink.
func Put(t *Theme, buf *Buffer, x, y int, sym string, ink Inkable) {
	k := ink.ink()
	u := Under(t, buf, x, y)
	if c := buf.At(x, y); c != nil {
		c.Symbol = sym
		c.Fg = CellRGB(Over(k.Color, u))
		c.Attrs = k.attrs()
	}
}

// Text writes s from (x, y), at most max cells, in ink; it returns the cells written. Wide
// characters take two cells; marks join the character before them.
func Text(t *Theme, buf *Buffer, x, y int, s string, max int, ink Inkable) int {
	col := 0
	var lastX int
	var last strings.Builder
	have := false
	flush := func() {
		if have {
			Put(t, buf, lastX, y, last.String(), ink)
			have = false
			last.Reset()
		}
	}
	for _, r := range s {
		if unicode.IsControl(r) {
			continue
		}
		cw := RuneWidth(r)
		if cw == 0 {
			if have {
				last.WriteRune(r)
			}
			continue
		}
		if col+cw > max {
			break
		}
		flush()
		lastX, have = x+col, true
		last.WriteRune(r)
		if cw == 2 {
			// The column a wide character covers: blank, same background.
			if c := buf.At(x+col+1, y); c != nil {
				c.Symbol = " "
			}
		}
		col += cw
	}
	flush()
	return col
}

// TextFit is [Text] elided with "…" to max cells.
func TextFit(t *Theme, buf *Buffer, x, y int, s string, max int, ink Inkable) int {
	return Text(t, buf, x, y, Fit(s, max), max, ink)
}

// TextRight is [Text] right-aligned to end at right (exclusive).
func TextRight(t *Theme, buf *Buffer, right, y int, s string, ink Inkable) int {
	w := Width(s)
	return Text(t, buf, max(0, right-w), y, s, w, ink)
}

// TextCenter is [Text] centered in w cells from x (an odd remainder goes right).
func TextCenter(t *Theme, buf *Buffer, x, y, w int, s string, ink Inkable) int {
	s = Fit(s, w)
	sw := Width(s)
	return Text(t, buf, x+max(0, w-sw)/2, y, s, sw, ink)
}

// Edges draws a control's edges: [EdgeLeft] at x and [EdgeRight] at x + w − 1 in color, when the
// state shows a border (nil: the cells stay as they are, reserved).
func Edges(t *Theme, buf *Buffer, x, y, w int, color *Rgba) {
	if color != nil && w >= 2 {
		Put(t, buf, x, y, EdgeLeft, *color)
		Put(t, buf, x+w-1, y, EdgeRight, *color)
	}
}

// HLine is a horizontal rule of sym across w cells.
func HLine(t *Theme, buf *Buffer, x, y, w int, sym string, color Rgba) {
	for i := range max(w, 0) {
		Put(t, buf, x+i, y, sym, color)
	}
}

// VLine is a vertical rule of sym down h cells.
func VLine(t *Theme, buf *Buffer, x, y, h int, sym string, color Rgba) {
	for i := range max(h, 0) {
		Put(t, buf, x, y+i, sym, color)
	}
}

// Corners are the box corners for the theme's radius: square at 0, rounded above.
func Corners(t *Theme) [4]string {
	if t.Rounded() {
		return [4]string{"╭", "╮", "╰", "╯"}
	}
	return [4]string{"┌", "┐", "└", "┘"}
}

// Frame draws a box border around area in color (light lines whatever the border width), corners
// by the radius, and an optional title in the top edge (`┌ Title ───┐`, from the second column).
// It returns the area inside the border.
func Frame(t *Theme, buf *Buffer, area Rect, color Rgba, title string, titleInk Ink) Rect {
	if area.W < 2 || area.H < 2 {
		return area
	}
	c := Corners(t)
	x, y, r, b := area.X, area.Y, area.Right()-1, area.Bottom()-1
	Put(t, buf, x, y, c[0], color)
	Put(t, buf, r, y, c[1], color)
	Put(t, buf, x, b, c[2], color)
	Put(t, buf, r, b, c[3], color)
	HLine(t, buf, x+1, y, area.W-2, "─", color)
	HLine(t, buf, x+1, b, area.W-2, "─", color)
	VLine(t, buf, x, y+1, area.H-2, "│", color)
	VLine(t, buf, r, y+1, area.H-2, "│", color)
	if title != "" && area.W > 4 {
		Text(t, buf, x+1, y, " "+Fit(title, area.W-4)+" ", area.W-2, titleInk)
	}
	return Rect{area.X + 1, area.Y + 1, area.W - 2, area.H - 2}
}

// Scrim composites c over everything in area, backgrounds and ink alike, so what is under a modal
// fades toward it.
func Scrim(t *Theme, buf *Buffer, area Rect, c Rgba) {
	if c.A == 0 {
		return
	}
	area = area.Intersect(buf.Area)
	for y := area.Y; y < area.Bottom(); y++ {
		for x := area.X; x < area.Right(); x++ {
			u := Under(t, buf, x, y)
			cell := buf.At(x, y)
			cell.Bg = CellRGB(Over(c, u))
			if cell.Fg.Kind == RGBColor {
				cell.Fg = CellRGB(Over(c, RGB(cell.Fg.R, cell.Fg.G, cell.Fg.B)))
			}
		}
	}
}

// WrapText word-wraps s to w cells at spaces; hard newlines are kept; a word longer than a line
// stays whole on its own line.
func WrapText(s string, w int) []string {
	var out []string
	for _, para := range strings.Split(s, "\n") {
		line := ""
		for _, word := range strings.Split(para, " ") {
			candidate := word
			if line != "" {
				candidate = line + " " + word
			}
			if Width(candidate) <= w || line == "" {
				line = candidate
			} else {
				out = append(out, line)
				line = word
			}
		}
		out = append(out, line)
	}
	return out
}
