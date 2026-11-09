package omarchy

import (
	"strconv"
	"strings"
	"unicode"
	"unicode/utf8"

	"charm.land/lipgloss/v2"
)

// Lip Gloss, both ways: Styles gives an app's own text the kit's roles as lipgloss.Style values
// (built from what is on screen, so build them in View and they fade); Styled draws any Lip Gloss
// (or other SGR-styled) string into a kit layout, over whatever is painted under it.

// Styles are the kit's text roles as Lip Gloss styles (spec/terminal.md, Type).
type Styles struct {
	// Text is the foreground; Title bold foreground (Heading, Hero title); Dim secondary text;
	// Faint placeholders and disabled text; Section a SectionHeader (bold textDim); Accent and
	// Urgent mark what is chosen and what destroys.
	Text, Title, Dim, Faint, Section, Accent, Urgent, Success, Warning lipgloss.Style
}

// Styles are the kit's roles for an app's own Lip Gloss text. Never set a background: the page is
// the terminal's.
func (t *Theme) Styles() Styles {
	s := lipgloss.NewStyle()
	return Styles{
		Text:    s.Foreground(t.Fg()),
		Title:   s.Foreground(t.Fg()).Bold(true),
		Dim:     s.Foreground(t.Dim()),
		Faint:   s.Foreground(t.Faint()),
		Section: s.Foreground(t.Dim()).Bold(true),
		Accent:  s.Foreground(t.Accent()),
		Urgent:  s.Foreground(t.Urgent()),
		Success: s.Foreground(t.Success()),
		Warning: s.Foreground(t.Warning()),
	}
}

// StyledText is a styled string as a component: lines at their display width, the SGR colors
// and attributes kept; cells the string gives no background keep what is painted under them.
type StyledText struct{ s string }

// Styled is s (a lipgloss.Style.Render result, or any SGR-styled text) as a component.
func Styled(s string) StyledText { return StyledText{s} }

func (st StyledText) Size(*Theme) (int, int) {
	lines := strings.Split(st.s, "\n")
	w := 0
	for _, l := range lines {
		w = max(w, Width(stripSGR(l)))
	}
	return w, len(lines)
}

func stripSGR(s string) string {
	var out strings.Builder
	for i := 0; i < len(s); i++ {
		if s[i] == 0x1b {
			i = skipEscape(s, i) - 1
			continue
		}
		out.WriteByte(s[i])
	}
	return out.String()
}

// skipEscape is the index after the escape sequence starting at i.
func skipEscape(s string, i int) int {
	if i+1 >= len(s) {
		return len(s)
	}
	switch s[i+1] {
	case '[':
		j := i + 2
		for j < len(s) && (s[j] < 0x40 || s[j] > 0x7e) {
			j++
		}
		return min(j+1, len(s))
	case ']':
		j := i + 2
		for j < len(s) {
			if s[j] == 0x07 {
				return j + 1
			}
			if s[j] == 0x1b && j+1 < len(s) && s[j+1] == '\\' {
				return j + 2
			}
			j++
		}
		return len(s)
	}
	return min(i+2, len(s))
}

type sgrState struct {
	fg, bg *Rgba
	attrs  Attrs
}

func (st *sgrState) apply(params string) {
	if params == "" {
		*st = sgrState{}
		return
	}
	ps := strings.FieldsFunc(params, func(r rune) bool { return r == ';' || r == ':' })
	num := func(i int) int {
		if i >= len(ps) {
			return 0
		}
		n, _ := strconv.Atoi(ps[i])
		return n
	}
	ansi := func(n int) *Rgba {
		// The terminal's 16 colors are the theme's; approximate with the xterm defaults.
		c := Xterm256(uint8(max(n, 16)))
		if n < 16 {
			std := [16][3]uint8{{0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0}, {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229}, {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}, {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}}
			c = std[n]
		}
		v := RGB(c[0], c[1], c[2])
		return &v
	}
	for i := 0; i < len(ps); i++ {
		n := num(i)
		switch {
		case n == 0:
			*st = sgrState{}
		case n == 1:
			st.attrs |= Bold
		case n == 2:
			st.attrs |= Faint
		case n == 3:
			st.attrs |= Italic
		case n == 4:
			st.attrs |= Underline
		case n == 7:
			st.attrs |= Reverse
		case n == 9:
			st.attrs |= Crossed
		case n == 22:
			st.attrs &^= Bold | Faint
		case n == 23:
			st.attrs &^= Italic
		case n == 24:
			st.attrs &^= Underline
		case n == 27:
			st.attrs &^= Reverse
		case n == 29:
			st.attrs &^= Crossed
		case n >= 30 && n <= 37:
			st.fg = ansi(n - 30)
		case n >= 90 && n <= 97:
			st.fg = ansi(n - 90 + 8)
		case n >= 40 && n <= 47:
			st.bg = ansi(n - 40)
		case n >= 100 && n <= 107:
			st.bg = ansi(n - 100 + 8)
		case n == 39:
			st.fg = nil
		case n == 49:
			st.bg = nil
		case n == 38 || n == 48:
			var c *Rgba
			if num(i+1) == 2 {
				v := RGB(uint8(num(i+2)), uint8(num(i+3)), uint8(num(i+4)))
				c = &v
				i += 4
			} else if num(i+1) == 5 {
				c = ansi(num(i + 2))
				i += 2
			}
			if n == 38 {
				st.fg = c
			} else {
				st.bg = c
			}
		}
	}
}

func (st StyledText) Draw(t *Theme, area Rect, buf *Buffer) {
	for dy, line := range strings.Split(st.s, "\n") {
		if dy >= area.H {
			return
		}
		y := area.Y + dy
		var s sgrState
		col := 0
		for i := 0; i < len(line); {
			if line[i] == 0x1b {
				j := skipEscape(line, i)
				if i+1 < len(line) && line[i+1] == '[' && line[j-1] == 'm' {
					s.apply(line[i+2 : j-1])
				}
				i = j
				continue
			}
			r, size := decodeRune(line[i:])
			i += size
			if unicode.IsControl(r) {
				continue
			}
			cw := RuneWidth(r)
			if cw == 0 {
				if c := buf.At(area.X+col-1, y); c != nil && col > 0 {
					c.Symbol += string(r)
				}
				continue
			}
			if col+cw > area.W {
				break
			}
			x := area.X + col
			if s.bg != nil {
				Fill(t, buf, Rect{x, y, cw, 1}, *s.bg)
			}
			fg := t.Fg()
			if s.fg != nil {
				fg = *s.fg
			}
			Put(t, buf, x, y, string(r), InkOf(fg))
			if c := buf.At(x, y); c != nil {
				c.Attrs = s.attrs
			}
			if cw == 2 {
				if c := buf.At(x+1, y); c != nil {
					c.Symbol = " "
				}
			}
			col += cw
		}
	}
}

func decodeRune(s string) (rune, int) { return utf8.DecodeRuneInString(s) }
