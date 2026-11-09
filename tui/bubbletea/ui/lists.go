package ui

import (
	"bytes"
	"image"
	"image/draw"
	"image/png"
	"os"
	"strconv"
	"strings"
	"unicode"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// Rows, the keyboard cursor that walks them, the scroll view they sit in, and the small pieces
// rows and grids are made of (spec/terminal.md, Lists).

// ---------------------------------------------------------------------------------------------
// CursorModel

// CursorSection is a named run of rows.
type CursorSection struct {
	Name  string
	Count int
}

// CursorKey is what a key did to a CursorModel.
type CursorKey int

const (
	CursorIgnored CursorKey = iota
	CursorMoved
	CursorActivate
	CursorClose
	CursorDelete
	CursorText
)

type gate struct{ col, row int }

// CursorModel is the keyboard cursor every panel re-implements: a section and a row within it,
// and whether it shows. The same keys as the GUI bindings' (spec/components/lists.md); the
// pointer moves it only when it reaches another cell.
type CursorModel struct {
	sections      []CursorSection
	section       int
	index         int
	Active        bool
	Columns       int
	Page          int
	gate          *gate
	initialSample bool
}

// NewCursorModel is a cursor over named sections.
func NewCursorModel(sections ...CursorSection) CursorModel {
	m := CursorModel{sections: sections, Columns: 1, Page: 5}
	m.settle()
	return m
}

// NewCursorList is a cursor over one unnamed section of count rows.
func NewCursorList(count int) CursorModel { return NewCursorModel(CursorSection{"", count}) }

// WithColumns lays the rows out in n columns (a grid).
func (m CursorModel) WithColumns(n int) CursorModel { m.Columns = max(n, 1); return m }

func (m *CursorModel) Section() string {
	if m.section < len(m.sections) {
		return m.sections[m.section].Name
	}
	return ""
}
func (m *CursorModel) Index() int { return m.index }

// IsAt is true when the cursor shows on row index of section.
func (m *CursorModel) IsAt(section string, index int) bool {
	return m.Active && m.Section() == section && m.index == index
}

// In is the cursor's row in section, or None.
func (m *CursorModel) In(section string) int {
	if m.Active && m.Section() == section {
		return m.index
	}
	return None
}

func (m *CursorModel) SetCount(section string, count int) {
	for i := range m.sections {
		if m.sections[i].Name == section {
			m.sections[i].Count = count
			break
		}
	}
	m.settle()
}

func (m *CursorModel) Set(section string, index int) {
	for i, s := range m.sections {
		if s.Name == section {
			m.section, m.index, m.Active = i, index, true
			m.settle()
			return
		}
	}
}

func (m *CursorModel) Reset() { m.Active, m.gate, m.initialSample = false, nil, false }

func (m *CursorModel) count() int {
	if m.section < len(m.sections) {
		return m.sections[m.section].Count
	}
	return 0
}

func (m *CursorModel) wake() bool {
	m.ResetPointer()
	if m.Active {
		return true
	}
	m.Active = true
	m.settle()
	return false
}

func (m *CursorModel) neighbour(dir int) int {
	for i := m.section + dir; i >= 0 && i < len(m.sections); i += dir {
		if m.sections[i].Count > 0 {
			return i
		}
	}
	return None
}

func (m *CursorModel) settle() {
	if m.count() == 0 {
		s := m.neighbour(1)
		if s == None {
			s = m.neighbour(-1)
		}
		if s != None {
			m.section, m.index = s, 0
		}
	}
	m.index = min(m.index, sub(m.count(), 1))
}

func sign(v int) int {
	switch {
	case v > 0:
		return 1
	case v < 0:
		return -1
	}
	return 0
}

// MoveBy moves dx columns and dy rows (into the next section past the ends).
func (m *CursorModel) MoveBy(dx, dy int) bool {
	if !m.wake() {
		return true
	}
	bs, bi := m.section, m.index
	if dx != 0 && m.Columns > 1 {
		col := m.index % m.Columns
		if to := col + dx; to >= 0 && to < m.Columns {
			if next := m.index + dx; next >= 0 && next < m.count() {
				m.index = next
			}
		}
	}
	if dy != 0 {
		step := m.Columns
		n := dy
		if n < 0 {
			n = -n
		}
		for range n {
			next := m.index + step*sign(dy)
			if next >= 0 && next < m.count() {
				m.index = next
			} else if dy > 0 {
				if s := m.neighbour(1); s != None {
					m.section = s
					m.index = min(m.index%m.Columns, sub(m.count(), 1))
				} else if m.Columns > 1 && m.index/m.Columns < sub(m.count(), 1)/m.Columns {
					m.index = m.count() - 1
				}
			} else if s := m.neighbour(-1); s != None {
				m.section = s
				count := m.count()
				lastLine := (sub(count, 1) / m.Columns) * m.Columns
				m.index = min(lastLine+m.index%m.Columns, sub(count, 1))
			}
		}
	}
	return m.section != bs || m.index != bi
}

func (m *CursorModel) MoveSection(delta int) bool {
	if !m.wake() {
		return true
	}
	if s := m.neighbour(sign(delta)); s != None {
		m.section, m.index = s, 0
		return true
	}
	return false
}

func (m *CursorModel) PageBy(delta int) bool {
	if !m.wake() {
		return true
	}
	before := m.index
	last := sub(m.count(), 1)
	step := max(m.Page, 1) * m.Columns
	m.index = max(0, min(last, m.index+step*sign(delta)))
	return m.index != before
}

func (m *CursorModel) Home() bool {
	m.Active = true
	bs, bi := m.section, m.index
	for i, s := range m.sections {
		if s.Count > 0 {
			m.section, m.index = i, 0
			break
		}
	}
	return m.section != bs || m.index != bi
}

func (m *CursorModel) End() bool {
	m.Active = true
	bs, bi := m.section, m.index
	for i := len(m.sections) - 1; i >= 0; i-- {
		if m.sections[i].Count > 0 {
			m.section, m.index = i, m.sections[i].Count-1
			break
		}
	}
	return m.section != bs || m.index != bi
}

// ApplyKey: Down/j, Up/k, Right/l, Left/h, Page Up/Down, Home/End, Tab/Shift+Tab (sections),
// Return and Space (activate), Escape (close), Delete and x (delete), any other printable key
// (text: [Printable] has it).
func (m *CursorModel) ApplyKey(msg tea.KeyPressMsg) CursorKey {
	moved := func(bool) CursorKey { return CursorMoved }
	switch keyOf(msg) {
	case "down", "j":
		return moved(m.MoveBy(0, 1))
	case "up", "k":
		return moved(m.MoveBy(0, -1))
	case "right", "l":
		return moved(m.MoveBy(1, 0))
	case "left", "h":
		return moved(m.MoveBy(-1, 0))
	case "pgdown":
		return moved(m.PageBy(1))
	case "pgup":
		return moved(m.PageBy(-1))
	case "home":
		return moved(m.Home())
	case "end":
		return moved(m.End())
	case "enter", "space":
		return CursorActivate
	case "esc":
		return CursorClose
	case "delete", "x", "X":
		return CursorDelete
	case "tab":
		return moved(m.MoveSection(1))
	case "shift+tab":
		return moved(m.MoveSection(-1))
	}
	if Printable(msg) != "" {
		return CursorText
	}
	return CursorIgnored
}

// PointerMoved is a pointer sample over row index of section at a cell: it moves the cursor only
// when the pointer reached another cell since the last accepted sample (a keyboard move resets
// the gate; the next sample only primes it).
func (m *CursorModel) PointerMoved(col, row int, section string, index int) bool {
	var moved bool
	if m.gate != nil {
		moved = *m.gate != gate{col, row}
	} else {
		moved = m.initialSample
	}
	if m.gate == nil || moved {
		m.gate = &gate{col, row}
	}
	m.initialSample = false
	if moved {
		m.Set(section, index)
	}
	return moved
}

func (m *CursorModel) ResetPointer()       { m.gate, m.initialSample = nil, false }
func (m *CursorModel) AllowInitialSample() { m.gate, m.initialSample = nil, true }

// ---------------------------------------------------------------------------------------------
// ListRow

// ListRow is one row of a keyboard-and-mouse list, one line:
// `[edge][pad][glyph ]Title  subtitle ··· value [trailing] [end][pad][edge]`. The cursor row takes
// the hover fill and edges, the current row the selected fill. Keyed `<key>`, its action
// `<key>-action`.
type ListRow struct {
	title, glyph, subtitle, value      string
	glyphColor                         *Rgba
	leading, trailing, expansion       Component
	status                             *toned
	trailingGlyph                      *toned
	action                             *rowAction
	actionCursor, cursor, current, off bool
	width                              int
	key                                string
}

type toned struct {
	text string
	tone Tone
}

type rowAction struct {
	glyph  string
	urgent bool
}

func NewListRow(title string) ListRow { return ListRow{title: title, width: 53} }

func (r ListRow) Glyph(g string) ListRow       { r.glyph = g; return r }
func (r ListRow) GlyphColor(c Rgba) ListRow    { r.glyphColor = &c; return r }
func (r ListRow) Subtitle(s string) ListRow    { r.subtitle = s; return r }
func (r ListRow) Value(v string) ListRow       { r.value = v; return r }
func (r ListRow) Trailing(c Component) ListRow { r.trailing = c; return r }
func (r ListRow) Cursor(on bool) ListRow       { r.cursor = on; return r }
func (r ListRow) Current(on bool) ListRow      { r.current = on; return r }
func (r ListRow) Disabled(on bool) ListRow     { r.off = on; return r }
func (r ListRow) Width(w int) ListRow          { r.width = w; return r }
func (r ListRow) Key(k string) ListRow         { r.key = k; return r }
func (r ListRow) ActionCursor(on bool) ListRow { r.actionCursor = on; return r }

// Leading is any leading item instead of a glyph (a small Avatar: 2 columns).
func (r ListRow) Leading(c Component) ListRow { r.leading = c; return r }

// Status replaces the subtitle with a toned status.
func (r ListRow) Status(s string, tone Tone) ListRow { r.status = &toned{s, tone}; return r }

func (r ListRow) TrailingGlyph(g string, tone Tone) ListRow {
	r.trailingGlyph = &toned{g, tone}
	return r
}

// Action is revealed in the end slot under the cursor (urgent: destructive).
func (r ListRow) Action(g string, urgent bool) ListRow { r.action = &rowAction{g, urgent}; return r }

// Expansion opens the row: any item under the main line, inside the row's fill.
func (r ListRow) Expansion(c Component) ListRow { r.expansion = c; return r }

func (r ListRow) Size(t *Theme) (int, int) {
	h := 1
	if r.expansion != nil {
		_, eh := r.expansion.Size(t)
		h += eh
	}
	return r.width, h
}

func (r ListRow) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	hot := r.cursor && !r.off
	f, border := omarchy.Transparent, (*Rgba)(nil)
	if hot {
		f, border = c.Hover.Fill, borderOf(c.Hover.Border)
	} else if r.current && !r.off {
		f, border = c.Selected.Fill, borderOf(c.Selected.Border)
	}
	// The whole row first, so the action and anything keyed in the row lie over it.
	if r.key != "" && !r.off {
		t.Hit(r.key, rect(area.X, area.Y, area.W, 1))
	}
	omarchy.Fill(t, buf, area, f)
	for dy := range area.H {
		omarchy.Edges(t, buf, area.X, area.Y+dy, area.W, border)
	}
	y := area.Y
	or := func(c Rgba) Rgba {
		if r.off {
			return t.Faint()
		}
		return c
	}
	x := area.X + 2
	if r.leading != nil {
		lw, _ := r.leading.Size(t)
		r.leading.Draw(t, rect(x, y, lw, 1), buf)
		x += lw + 1
	} else if r.glyph != "" {
		gc := t.Fg()
		if r.glyphColor != nil {
			gc = *r.glyphColor
		}
		omarchy.Put(t, buf, x, y, r.glyph, or(gc))
		x += 3
	}
	// The end: [pad][edge] at the right, the end slot before them, then trailing, value.
	right := sub(area.Right(), 2)
	showAction := r.action != nil && !r.off && (r.cursor || r.actionCursor)
	if r.trailingGlyph != nil || r.action != nil {
		right = sub(right, 1)
		ex := right
		if showAction {
			k := t.Dim()
			if r.action.urgent {
				k = t.Urgent()
			}
			if r.actionCursor {
				omarchy.Fill(t, buf, rect(ex, y, 1, 1), k.WithAlpha(float64(c.Hover.Fill.AlphaF())))
			}
			omarchy.Put(t, buf, ex, y, r.action.glyph, k)
			if r.key != "" {
				t.Hit(r.key+"-action", rect(ex, y, 1, 1))
			}
		} else if r.trailingGlyph != nil {
			omarchy.Put(t, buf, ex, y, r.trailingGlyph.text, or(r.trailingGlyph.tone.Color(t, t.Dim())))
		}
		right = sub(right, 1)
	}
	if r.trailing != nil {
		tw, _ := r.trailing.Size(t)
		right = sub(right, tw)
		r.trailing.Draw(t, rect(right, y, tw, 1), buf)
		right = sub(right, 1)
	}
	if r.value != "" {
		vw := width(r.value)
		omarchy.TextRight(t, buf, right, y, r.value, or(t.Dim()))
		right = sub(right, vw+1)
	}
	avail := sub(right, x)
	tw := omarchy.TextFit(t, buf, x, y, r.title, avail, bold(or(t.Fg()), true))
	second, secondInk := "", omarchy.Transparent
	if r.status != nil {
		second, secondInk = r.status.text, or(r.status.tone.Color(t, t.Dim()))
	} else if r.subtitle != "" {
		second, secondInk = r.subtitle, or(t.Dim())
	}
	if second != "" && avail > tw+2 {
		omarchy.TextFit(t, buf, x+tw+2, y, second, avail-tw-2, secondInk)
	}
	if r.expansion != nil {
		_, eh := r.expansion.Size(t)
		r.expansion.Draw(t, rect(area.X+2, y+1, sub(area.W, 4), eh), buf)
	}
}

// ---------------------------------------------------------------------------------------------
// ScrollArea

// ScrollArea is a vertical scroll view capped at MaxRows, the bar over its last column while the
// content overflows: a `│` groove in track, a `┃` thumb in the normal border color.
type ScrollArea struct {
	children                    []Component
	maxRows, offset, gap, width int
}

func NewScrollArea(width int) ScrollArea { return ScrollArea{maxRows: 1 << 30, width: width} }

func (s ScrollArea) MaxRows(n int) ScrollArea { s.maxRows = n; return s }
func (s ScrollArea) Gap(g int) ScrollArea     { s.gap = g; return s }
func (s ScrollArea) Child(k ...Component) ScrollArea {
	s.children = appendKids(s.children, k...)
	return s
}

// Offset is the first content row shown.
func (s ScrollArea) Offset(rows int) ScrollArea { s.offset = rows; return s }

func (s ScrollArea) contentH(t *Theme) int {
	h := 0
	for _, c := range s.children {
		_, ch := c.Size(t)
		h += ch
	}
	return h + sub(len(s.children), 1)*s.gap
}

// EnsureVisible is the offset that shows rows [top, top+h) of the content in a max-row
// viewport, moving as little as possible from offset.
func EnsureVisible(offset, top, h, maxRows int) int {
	switch {
	case top < offset:
		return top
	case top+h > offset+maxRows:
		return sub(top+h, maxRows)
	}
	return offset
}

func (s ScrollArea) Size(t *Theme) (int, int) { return s.width, min(s.contentH(t), s.maxRows) }

func (s ScrollArea) Draw(t *Theme, area Rect, buf *Buffer) {
	total := s.contentH(t)
	view := area.H
	offset := min(s.offset, sub(total, view))
	// Draw the whole content into a scratch buffer, then copy the window.
	scratchArea := rect(area.X, 0, area.W, max(total, 1))
	scratch := omarchy.NewBuffer(scratchArea)
	// What's under the area shows through the scratch (fills composite over it).
	for y := 0; y < min(total, view+offset); y++ {
		if y >= offset {
			for x := area.X; x < area.Right(); x++ {
				src, dst := buf.At(x, area.Y+y-offset), scratch.At(x, y)
				if src != nil && dst != nil {
					*dst = *src
				}
			}
		}
	}
	y := 0
	hits := t.HitsLen()
	for _, c := range s.children {
		cw, ch := c.Size(t)
		c.Draw(t, rect(area.X, y, min(cw, area.W), ch).Intersect(scratchArea), scratch)
		y += ch + s.gap
	}
	// Keys drawn in the scratch buffer land where they show, or nowhere when scrolled away.
	shown := rect(area.X, offset, area.W, view)
	t.RemapHits(hits, func(r Rect) (Rect, bool) {
		r = r.Intersect(shown)
		if r.H <= 0 {
			return r, false
		}
		r.Y = r.Y - offset + area.Y
		return r, true
	})
	for dy := range min(view, total) {
		for x := area.X; x < area.Right(); x++ {
			src, dst := scratch.At(x, offset+dy), buf.At(x, area.Y+dy)
			if src != nil && dst != nil {
				*dst = *src
			}
		}
	}
	if total > view && view > 0 {
		bx := area.Right() - 1
		thumb := max((view*view+total/2)/total, 1)
		room := view - thumb
		maxOff := total - view
		ty := (offset*room + maxOff/2) / maxOff
		for dy := range view {
			if dy >= ty && dy < ty+thumb {
				omarchy.Put(t, buf, bx, area.Y+dy, "┃", t.Shown().Controls.Normal.Border.Color)
			} else {
				omarchy.Put(t, buf, bx, area.Y+dy, "│", t.Track())
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// KeyValue, Hero

type pair struct {
	label, value string
	tone         Tone
}

// KeyValue is dim labels beside their values, Columns pairs to a line: label column, one space,
// value column, three columns between pairs.
type KeyValue struct {
	pairs   []pair
	columns int
}

func NewKeyValue() KeyValue { return KeyValue{columns: 1} }

func (k KeyValue) Pair(label, value string) KeyValue { return k.PairToned(label, value, Neutral) }
func (k KeyValue) PairToned(label, value string, tone Tone) KeyValue {
	k.pairs = append(append([]pair(nil), k.pairs...), pair{label, value, tone})
	return k
}
func (k KeyValue) Columns(n int) KeyValue { k.columns = max(n, 1); return k }

func (k KeyValue) widths() [][2]int {
	out := make([][2]int, k.columns)
	for i, p := range k.pairs {
		c := i % k.columns
		out[c][0] = max(out[c][0], width(p.label))
		out[c][1] = max(out[c][1], width(p.value))
	}
	return out
}

func (k KeyValue) Size(*Theme) (int, int) {
	w := 0
	for _, c := range k.widths() {
		w += c[0] + 1 + c[1]
	}
	w += 3 * (k.columns - 1)
	return w, (len(k.pairs) + k.columns - 1) / k.columns
}

func (k KeyValue) Draw(t *Theme, area Rect, buf *Buffer) {
	ws := k.widths()
	for i, p := range k.pairs {
		line, col := i/k.columns, i%k.columns
		x := area.X
		for _, w := range ws[:col] {
			x += w[0] + 1 + w[1] + 3
		}
		y := area.Y + line
		omarchy.Text(t, buf, x, y, p.label, ws[col][0], t.Dim())
		omarchy.Text(t, buf, x+ws[col][0]+1, y, p.value, ws[col][1], p.tone.Color(t, t.Fg()))
	}
}

// Hero is a panel's head: the glyph, then the bold title over an uppercased bold status line
// (textDim, urgent on error), the trailing control at the end of the first row.
type Hero struct {
	glyph, title, status string
	err                  bool
	glyphOpacity         float64
	trailing             Component
	width                int
}

func NewHero(glyph, title string) Hero {
	return Hero{glyph: glyph, title: title, glyphOpacity: 1, width: 53}
}

func (h Hero) Status(s string) Hero        { h.status = s; return h }
func (h Hero) Error(on bool) Hero          { h.err = on; return h }
func (h Hero) GlyphOpacity(o float64) Hero { h.glyphOpacity = o; return h }
func (h Hero) Trailing(c Component) Hero   { h.trailing = c; return h }
func (h Hero) Width(w int) Hero            { h.width = w; return h }

func (h Hero) Size(*Theme) (int, int) { return h.width, 2 }

func (h Hero) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Put(t, buf, area.X, area.Y, h.glyph, t.Fg().WithAlpha(h.glyphOpacity))
	tw := 0
	if h.trailing != nil {
		w, _ := h.trailing.Size(t)
		tw = w + 2
	}
	words := sub(area.W, 3+tw)
	omarchy.TextFit(t, buf, area.X+3, area.Y, h.title, words, bold(t.Fg(), true))
	k := t.Dim()
	if h.err {
		k = t.Urgent()
	}
	omarchy.TextFit(t, buf, area.X+3, area.Y+1, strings.ToUpper(h.status), words, bold(k, true))
	if h.trailing != nil {
		w, th := h.trailing.Size(t)
		h.trailing.Draw(t, rect(sub(area.Right(), w), area.Y, w, th), buf)
	}
}

// ---------------------------------------------------------------------------------------------
// Badge, StatusDot

// Badge is a count or short label, ` 3 ` bold. Neutral: the normal fill; toned: the tone at the
// selected fill's alpha, text in the tone; filled: the tone solid, text in the background.
type Badge struct {
	label  string
	tone   Tone
	filled bool
}

func NewBadge(label string) Badge { return Badge{label: label} }

func (b Badge) Tone(t Tone) Badge    { b.tone = t; return b }
func (b Badge) Filled(on bool) Badge { b.filled = on; return b }

func (b Badge) Size(*Theme) (int, int) { return width(b.label) + 2, 1 }

func (b Badge) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	tone := b.tone.Color(t, t.Fg())
	var bg, k Rgba
	switch {
	case b.filled:
		bg, k = tone, t.Bg()
	case b.tone == Neutral:
		bg, k = c.Normal.Fill, t.Fg()
	default:
		bg, k = tone.WithAlpha(float64(c.Selected.Fill.AlphaF())), tone
	}
	omarchy.Fill(t, buf, area, bg)
	omarchy.Text(t, buf, area.X+1, area.Y, b.label, sub(area.W, 2), bold(k, true))
}

// StatusDot is a dot `●` in its tone.
type StatusDot Tone

func (StatusDot) Size(*Theme) (int, int) { return 1, 1 }
func (d StatusDot) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Put(t, buf, area.X, area.Y, "●", Tone(d).Color(t, t.Fg()))
}

// ---------------------------------------------------------------------------------------------
// Avatar

// AvatarSize is an avatar's size in cells: small 2×1 (a ListRow's leading slot), medium 4×2,
// large 6×3.
type AvatarSize int

const (
	AvatarMedium AvatarSize = iota
	AvatarSmall
	AvatarLarge
)

func (s AvatarSize) Cells() (int, int) {
	switch s {
	case AvatarSmall:
		return 2, 1
	case AvatarLarge:
		return 6, 3
	}
	return 4, 2
}

// AvatarImage is a decoded picture (8-bit straight RGBA), drawn in half-block pixels.
type AvatarImage struct {
	Width, Height int
	RGBA          []uint8
}

// LoadAvatarImage decodes a PNG file; nil when it can't be read.
func LoadAvatarImage(path string) *AvatarImage {
	b, err := os.ReadFile(path)
	if err != nil {
		return nil
	}
	return DecodeAvatarImage(b)
}

// DecodeAvatarImage decodes PNG bytes; nil when they can't be read.
func DecodeAvatarImage(b []byte) *AvatarImage {
	img, err := png.Decode(bytes.NewReader(b))
	if err != nil {
		return nil
	}
	bounds := img.Bounds()
	w, h := bounds.Dx(), bounds.Dy()
	out := &AvatarImage{Width: w, Height: h, RGBA: make([]uint8, w*h*4)}
	if n, ok := img.(*image.NRGBA); ok {
		for y := range h {
			copy(out.RGBA[y*w*4:(y+1)*w*4], n.Pix[y*n.Stride:y*n.Stride+w*4])
		}
		return out
	}
	// Straight alpha, 8 bits per channel.
	n := image.NewNRGBA(image.Rect(0, 0, w, h))
	draw.Draw(n, n.Bounds(), img, bounds.Min, draw.Src)
	for y := range h {
		copy(out.RGBA[y*w*4:(y+1)*w*4], n.Pix[y*n.Stride:y*n.Stride+w*4])
	}
	return out
}

// Sample is the w × h box-filtered pixel (i, j): the source box [floor(i·W/w),
// floor((i+1)·W/w)) (and the same down), alpha-weighted color round(Σc·a / Σa), alpha
// round(Σa / n).
func (img *AvatarImage) Sample(w, h, i, j int) Rgba {
	x0, x1 := i*img.Width/w, max((i+1)*img.Width/w, i*img.Width/w+1)
	y0, y1 := j*img.Height/h, max((j+1)*img.Height/h, j*img.Height/h+1)
	var r, g, b, a, n uint64
	for y := y0; y < min(y1, img.Height); y++ {
		for x := x0; x < min(x1, img.Width); x++ {
			p := (y*img.Width + x) * 4
			pa := uint64(img.RGBA[p+3])
			r += uint64(img.RGBA[p]) * pa
			g += uint64(img.RGBA[p+1]) * pa
			b += uint64(img.RGBA[p+2]) * pa
			a += pa
			n++
		}
	}
	if a == 0 || n == 0 {
		return omarchy.Transparent
	}
	ch := func(s uint64) uint8 { return uint8((s*2 + a) / (2 * a)) }
	return Rgba{R: ch(r), G: ch(g), B: ch(b), A: uint8((a*2 + n) / (2 * n))}
}

// Avatar is a person's or an app's picture with fallbacks: the image (in half-block pixels, two
// per cell), a glyph, the initial. Icons by freedesktop name don't exist in a terminal: an
// Avatar with only an Icon name draws its glyph or its initial (spec/terminal.md, Icons).
type Avatar struct {
	name, glyph string
	image       *AvatarImage
	size        AvatarSize
	round       bool
	status      *Tone
}

func NewAvatar(name string) Avatar { return Avatar{name: name} }

func (a Avatar) Image(img *AvatarImage) Avatar { a.image = img; return a }

// Icon is a freedesktop icon name: accepted for parity, not drawn (no icons in a terminal).
func (a Avatar) Icon(string) Avatar         { return a }
func (a Avatar) Glyph(g string) Avatar      { a.glyph = g; return a }
func (a Avatar) SizeAs(s AvatarSize) Avatar { a.size = s; return a }
func (a Avatar) Round(on bool) Avatar       { a.round = on; return a }
func (a Avatar) Status(tone Tone) Avatar    { a.status = &tone; return a }

// Initial is the first letter or digit of name, upper-cased.
func Initial(name string) string {
	for _, r := range name {
		if unicode.IsLetter(r) || unicode.IsDigit(r) || unicode.IsNumber(r) {
			return strings.ToUpper(string(r))
		}
	}
	return ""
}

func (a Avatar) Size(*Theme) (int, int) { return a.size.Cells() }

func (a Avatar) Draw(t *Theme, area Rect, buf *Buffer) {
	w, h := a.size.Cells()
	area.W, area.H = min(w, area.W), min(h, area.H)
	if a.image != nil {
		pw, ph := w, h*2
		for j := range h {
			for i := range w {
				px := func(pj int) Rgba {
					if a.round {
						// Outside the inscribed circle (pixel centers, in pixel units): clear.
						cx, cy := float64(i)+0.5-float64(pw)/2, float64(pj)+0.5-float64(ph)/2
						r := float64(min(pw, ph)) / 2
						if cx*cx+cy*cy > r*r {
							return omarchy.Transparent
						}
					}
					return a.image.Sample(pw, ph, i, pj)
				}
				top, bottom := px(j*2), px(j*2+1)
				x, y := area.X+i, area.Y+j
				omarchy.Fill(t, buf, rect(x, y, 1, 1), bottom)
				omarchy.Put(t, buf, x, y, "▀", omarchy.Over(top, t.Bg()))
			}
		}
	} else {
		omarchy.Fill(t, buf, area, t.Shown().Controls.Selected.Fill)
		mark, b := a.glyph, false
		if mark == "" {
			mark, b = Initial(a.name), true
		}
		omarchy.Put(t, buf, area.X+(w-1)/2, area.Y+(h-1)/2, mark, bold(t.Fg(), b))
	}
	if a.status != nil {
		omarchy.Put(t, buf, area.Right()-1, area.Bottom()-1, "●", a.status.Color(t, t.Fg()))
	}
}

// ---------------------------------------------------------------------------------------------
// TileGrid

// Tile is a glyph or an Avatar over a label of up to two lines.
type Tile struct {
	label, glyph string
	avatar       *Avatar
}

func NewTile(label string) Tile { return Tile{label: label} }

func (t Tile) Glyph(g string) Tile { t.glyph = g; return t }
func (t Tile) Avatar(a Avatar) Tile {
	a = a.SizeAs(AvatarMedium)
	t.avatar = &a
	return t
}

// TileGrid is tiles 11 columns × 4 rows, no gap: an icon box of two rows (the glyph on its second
// row, or a medium Avatar filling it), then two label rows, centered, wrapped and elided. The
// cursor tile takes the hover fill and edges, the current one the selected fill. Tiles are keyed
// `<key>-<i>`.
type TileGrid struct {
	tiles                    []Tile
	columns, cursor, current int
	key                      string
}

// TileW and TileH are a tile's size in cells.
const TileW, TileH = 11, 4

func NewTileGrid(tiles ...Tile) TileGrid {
	return TileGrid{tiles: tiles, columns: 4, cursor: None, current: None}
}

func (g TileGrid) Columns(n int) TileGrid { g.columns = max(n, 1); return g }
func (g TileGrid) Cursor(i int) TileGrid  { g.cursor = i; return g }
func (g TileGrid) Current(i int) TileGrid { g.current = i; return g }
func (g TileGrid) Key(k string) TileGrid  { g.key = k; return g }

func (g TileGrid) Size(*Theme) (int, int) {
	cols := min(g.columns, max(len(g.tiles), 1))
	return cols * TileW, (len(g.tiles) + g.columns - 1) / g.columns * TileH
}

func (g TileGrid) Draw(t *Theme, area Rect, buf *Buffer) {
	for i, tile := range g.tiles {
		c, r := i%g.columns, i/g.columns
		cell := rect(area.X+c*TileW, area.Y+r*TileH, TileW, TileH).Intersect(area)
		if cell.H < TileH {
			continue
		}
		l := LookOf(t, g.cursor == i, g.current == i, false, false)
		omarchy.Fill(t, buf, cell, l.Fill)
		for dy := range TileH {
			omarchy.Edges(t, buf, cell.X, cell.Y+dy, TileW, l.Border)
		}
		if tile.avatar != nil {
			aw, ah := tile.avatar.Size(t)
			tile.avatar.Draw(t, rect(cell.X+(TileW-aw)/2, cell.Y, aw, ah), buf)
		} else {
			omarchy.TextCenter(t, buf, cell.X+1, cell.Y+1, TileW-2, tile.glyph, t.Fg())
		}
		lines := omarchy.WrapText(tile.label, TileW-2)
		for n, line := range lines {
			if n >= 2 {
				break
			}
			if n == 1 && len(lines) > 2 {
				line = omarchy.Fit(line+" "+strings.Join(lines[2:], " "), TileW-2)
			}
			omarchy.TextCenter(t, buf, cell.X+1, cell.Y+2+n, TileW-2, line, bold(t.Fg(), l.Bold))
		}
		if g.key != "" {
			t.Hit(g.key+"-"+strconv.Itoa(i), cell)
		}
	}
}
