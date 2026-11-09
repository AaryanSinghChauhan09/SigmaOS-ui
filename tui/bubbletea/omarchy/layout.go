package omarchy

// Component is anything the kit draws: it knows its natural size in cells and draws itself into
// an area of a [Buffer]. Every ui component is one (value types, built like Lip Gloss styles);
// [Theme.Render] turns one into a string for a View or a lipgloss.JoinVertical.
type Component interface {
	// Size is the natural size: columns, rows.
	Size(t *Theme) (w, h int)
	// Draw draws into area (normally the natural size).
	Draw(t *Theme, area Rect, buf *Buffer)
}

// Render draws c at its natural size into a fresh buffer, finishes it (the page background left
// to the terminal, colors mapped to the terminal's depth) and returns it as a styled string: a
// View's content, or a piece for lipgloss.JoinHorizontal. Hits are recorded in c's own
// coordinates (render the root at the screen's top-left for clicks to land).
func (t *Theme) Render(c Component) string {
	w, h := c.Size(t)
	return t.RenderSize(c, w, h)
}

// RenderSize is [Theme.Render] into a w × h area (a full-screen root: the window's size).
func (t *Theme) RenderSize(c Component, w, h int) string {
	area := R(0, 0, w, h)
	buf := NewBuffer(area)
	c.Draw(t, area, buf)
	t.Finish(buf)
	return ANSI(buf)
}

// Col stacks children top to bottom, gap rows apart, each at its natural size (or the column's
// width, [Col.Stretch]).
type Col struct {
	gap, width int
	items      []colItem
}

type colItem struct {
	c       Component
	stretch bool
}

// Column is a [Col] with gap rows between children.
func Column(gap int, children ...Component) Col {
	c := Col{gap: gap}
	for _, ch := range children {
		c.items = append(c.items, colItem{c: ch})
	}
	return c
}

// Child adds a child.
func (c Col) Child(ch ...Component) Col {
	items := append([]colItem(nil), c.items...)
	for _, x := range ch {
		items = append(items, colItem{c: x})
	}
	c.items = items
	return c
}

// Stretch adds a child given the column's whole width.
func (c Col) Stretch(ch Component) Col {
	c.items = append(append([]colItem(nil), c.items...), colItem{c: ch, stretch: true})
	return c
}

// Width fixes the column's width (else the widest child's).
func (c Col) Width(w int) Col { c.width = w; return c }

func (c Col) Size(t *Theme) (int, int) {
	w, h := 0, 0
	for i, it := range c.items {
		cw, ch := it.c.Size(t)
		w = max(w, cw)
		h += ch
		if i > 0 {
			h += c.gap
		}
	}
	if c.width > 0 {
		w = c.width
	}
	return w, h
}

func (c Col) Draw(t *Theme, area Rect, buf *Buffer) {
	y := area.Y
	for _, it := range c.items {
		cw, ch := it.c.Size(t)
		w := min(cw, area.W)
		if it.stretch {
			w = area.W
		}
		r := Rect{area.X, y, w, ch}.Intersect(area)
		if r.H > 0 {
			r.H = min(ch, max(0, area.Bottom()-y))
			it.c.Draw(t, r, buf)
		}
		y += ch + c.gap
	}
}

// Row places children left to right, gap columns apart, top-aligned (or centered) at their
// natural sizes.
type Row struct {
	gap    int
	center bool
	items  []Component
}

// Line is a [Row] with gap columns between children.
func Line(gap int, children ...Component) Row { return Row{gap: gap, items: children} }

// Child adds a child.
func (r Row) Child(ch ...Component) Row {
	r.items = append(append([]Component(nil), r.items...), ch...)
	return r
}

// Center centers shorter children on the row's height.
func (r Row) Center() Row { r.center = true; return r }

func (r Row) Size(t *Theme) (int, int) {
	w, h := 0, 0
	for i, it := range r.items {
		cw, ch := it.Size(t)
		w += cw
		if i > 0 {
			w += r.gap
		}
		h = max(h, ch)
	}
	return w, h
}

func (r Row) Draw(t *Theme, area Rect, buf *Buffer) {
	_, h := r.Size(t)
	x := area.X
	for _, it := range r.items {
		cw, ch := it.Size(t)
		dy := 0
		if r.center {
			dy = max(0, h-ch) / 2
		}
		a := Rect{x, area.Y + dy, cw, ch}.Intersect(area)
		if a.W > 0 && a.H > 0 {
			it.Draw(t, a, buf)
		}
		x += cw + r.gap
	}
}

// Boxed holds one child in a fixed w × h box: at the top left ([Sized]) or centered
// ([Centered]).
type Boxed struct {
	w, h   int
	center bool
	inner  Component
}

// Sized is inner in a w × h box, at the top left.
func Sized(w, h int, inner Component) Boxed { return Boxed{w: w, h: h, inner: inner} }

// Centered is inner centered in a w × h box.
func Centered(w, h int, inner Component) Boxed { return Boxed{w: w, h: h, center: true, inner: inner} }

func (b Boxed) Size(*Theme) (int, int) { return b.w, b.h }

func (b Boxed) Draw(t *Theme, area Rect, buf *Buffer) {
	cw, ch := b.inner.Size(t)
	cw, ch = min(cw, area.W), min(ch, area.H)
	dx, dy := 0, 0
	if b.center {
		dx, dy = (area.W-cw)/2, (area.H-ch)/2
	}
	b.inner.Draw(t, Rect{area.X + dx, area.Y + dy, cw, ch}, buf)
}

// Filled gives inner the whole w × h box (it stretches to fill).
type Filled struct {
	w, h  int
	inner Component
}

// FillBox is inner given the whole w × h box.
func FillBox(w, h int, inner Component) Filled { return Filled{w, h, inner} }

func (f Filled) Size(*Theme) (int, int)                { return f.w, f.h }
func (f Filled) Draw(t *Theme, area Rect, buf *Buffer) { f.inner.Draw(t, area, buf) }

// Blank is empty space.
type Blank struct{ W, H int }

func (b Blank) Size(*Theme) (int, int)   { return b.W, b.H }
func (Blank) Draw(*Theme, Rect, *Buffer) {}

// Label is one line of text in an ink picked from the theme at draw time (so it fades), elided
// to the area.
type Label struct {
	text string
	ink  func(*Theme) Ink
}

// NewLabel is a [Label].
func NewLabel(text string, ink func(*Theme) Ink) Label { return Label{text, ink} }

func (l Label) Size(*Theme) (int, int) { return Width(l.text), 1 }
func (l Label) Draw(t *Theme, area Rect, buf *Buffer) {
	TextFit(t, buf, area.X, area.Y, l.text, area.W, l.ink(t))
}

// Drawn is anything drawn by a function, at a fixed size: an app's own drawing in the kit's
// terms (Fill, Put, Text, Frame) with the theme's colors.
type Drawn struct {
	w, h int
	f    func(t *Theme, area Rect, buf *Buffer)
}

// Custom is a [Drawn] component: f draws it into its area, w × h.
func Custom(w, h int, f func(t *Theme, area Rect, buf *Buffer)) Drawn { return Drawn{w, h, f} }

func (c Drawn) Size(*Theme) (int, int)                { return c.w, c.h }
func (c Drawn) Draw(t *Theme, area Rect, buf *Buffer) { c.f(t, area, buf) }
