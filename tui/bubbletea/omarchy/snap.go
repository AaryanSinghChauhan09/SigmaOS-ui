package omarchy

import (
	"strconv"
	"strings"
)

// style is what SGR carries for one cell.
type style struct {
	fg, bg Color
	attrs  Attrs
}

// normalized: a space without underline, reverse or crossed shows only its background
// (spec/terminal.md, Snapshots: canonical form).
func normalized(text string, c *Cell) style {
	s := style{c.Fg, c.Bg, c.Attrs}
	if text == " " && c.Attrs&(Underline|Reverse|Crossed) == 0 {
		s.fg = Color{}
		s.attrs &^= Bold | Faint | Italic
	}
	return s
}

func pushColor(out *strings.Builder, base int, c Color, sgr bool) {
	switch c.Kind {
	case RGBColor:
		out.WriteString(";" + strconv.Itoa(base) + ";2;" + strconv.Itoa(int(c.R)) + ";" + strconv.Itoa(int(c.G)) + ";" + strconv.Itoa(int(c.B)))
	case IndexedColor:
		n := int(c.Index)
		switch {
		case sgr && n < 8:
			out.WriteString(";" + strconv.Itoa(base-8+n))
		case sgr && n < 16:
			out.WriteString(";" + strconv.Itoa(base+52+n-8))
		default:
			out.WriteString(";" + strconv.Itoa(base) + ";5;" + strconv.Itoa(n))
		}
	}
}

// write serializes buf: per cell left to right (a wide character's second column writes nothing),
// `ESC[0` plus the attributes and colors whenever the style changes, a reset at the end of a line
// whose last style isn't the default. canonical writes indices as `38;5;n` (the snapshot form);
// otherwise ANSI indices 0–15 are written as 30–37 / 90–97 (the terminal form).
func write(buf *Buffer, canonical bool) string {
	var out strings.Builder
	w, h := buf.Area.W, buf.Area.H
	for y := range h {
		prev := style{}
		x := 0
		for x < w {
			c := &buf.Cells[y*w+x]
			sw := Width(c.Symbol)
			cw := min(max(sw, 1), 2)
			if x+cw > w {
				cw = 1
			}
			text := c.Symbol
			if sw == 0 {
				text = " "
			}
			s := normalized(text, c)
			if s != prev {
				out.WriteString("\x1b[0")
				for _, a := range []struct {
					on   Attrs
					code string
				}{{Bold, ";1"}, {Faint, ";2"}, {Italic, ";3"}, {Underline, ";4"}, {Reverse, ";7"}, {Crossed, ";9"}} {
					if s.attrs&a.on != 0 {
						out.WriteString(a.code)
					}
				}
				pushColor(&out, 38, s.fg, !canonical)
				pushColor(&out, 48, s.bg, !canonical)
				out.WriteByte('m')
				prev = s
			}
			out.WriteString(text)
			x += cw
		}
		if prev != (style{}) {
			out.WriteString("\x1b[0m")
		}
		if canonical || y+1 < h {
			out.WriteByte('\n')
		}
	}
	return out.String()
}

// Snapshot is buf as the canonical ANSI snapshot every terminal binding's `--snap` writes
// (spec/terminal.md, Snapshots). Call [Theme.Finish] on it first.
func Snapshot(buf *Buffer) string { return write(buf, true) }

// ANSI is buf as terminal output (lines joined by newlines, no trailing one): what a Bubble Tea
// View carries. Call [Theme.Finish] on it first.
func ANSI(buf *Buffer) string { return write(buf, false) }

// RenderSnapshot draws c into a fresh cols × rows buffer (rows 0: c's natural height), finishes it
// in truecolor and returns the snapshot text.
func (t *Theme) RenderSnapshot(c Component, cols, rows int) string {
	depth := t.depth
	t.depth = TrueColor
	defer func() { t.depth = depth }()
	_, h := c.Size(t)
	if rows == 0 {
		rows = max(h, 1)
	}
	area := R(0, 0, cols, rows)
	buf := NewBuffer(area)
	t.ClearHits()
	c.Draw(t, area, buf)
	t.Finish(buf)
	return Snapshot(buf)
}
