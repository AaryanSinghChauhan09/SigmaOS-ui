package omarchy

// Rect is an area of cells: columns x … x+W−1, rows y … y+H−1.
type Rect struct{ X, Y, W, H int }

// R is a Rect.
func R(x, y, w, h int) Rect { return Rect{x, y, max(w, 0), max(h, 0)} }

func (r Rect) Right() int  { return r.X + r.W }
func (r Rect) Bottom() int { return r.Y + r.H }
func (r Rect) Empty() bool { return r.W <= 0 || r.H <= 0 }

// Contains is true when the cell (x, y) is inside.
func (r Rect) Contains(x, y int) bool { return x >= r.X && x < r.Right() && y >= r.Y && y < r.Bottom() }

// Intersect is the overlap of two areas (empty, at the later corner, when they don't meet).
func (r Rect) Intersect(o Rect) Rect {
	x1, y1 := max(r.X, o.X), max(r.Y, o.Y)
	x2, y2 := min(r.Right(), o.Right()), min(r.Bottom(), o.Bottom())
	return Rect{x1, y1, max(0, x2-x1), max(0, y2-y1)}
}

// Inset is r shrunk by dx columns and dy rows on every side.
func (r Rect) Inset(dx, dy int) Rect {
	return Rect{r.X + min(dx, r.W/2), r.Y + min(dy, r.H/2), max(0, r.W-2*dx), max(0, r.H-2*dy)}
}

// ColorKind says what a cell's color is.
type ColorKind uint8

const (
	// DefaultColor is the terminal's own foreground or background (SGR 39 / 49).
	DefaultColor ColorKind = iota
	// RGBColor is a 24-bit color.
	RGBColor
	// IndexedColor is an ANSI (0–15) or xterm-256 (16–255) index.
	IndexedColor
)

// Color is a cell's color: the default, 24-bit, or an index (below truecolor, after Finish).
type Color struct {
	Kind    ColorKind
	R, G, B uint8
	Index   uint8
}

// CellRGB is a 24-bit cell color.
func CellRGB(c Rgba) Color { return Color{Kind: RGBColor, R: c.R, G: c.G, B: c.B} }

// Indexed is an indexed cell color.
func Indexed(n uint8) Color { return Color{Kind: IndexedColor, Index: n} }

// Attrs are a cell's attributes.
type Attrs uint8

const (
	Bold Attrs = 1 << iota
	Faint
	Italic
	Underline
	Reverse
	Crossed
)

// Cell is one cell of the grid: a symbol (a character and any marks on it; a wide character's
// second column holds a blank), colors and attributes.
type Cell struct {
	Symbol string
	Fg, Bg Color
	Attrs  Attrs
}

var blankCell = Cell{Symbol: " "}

// Buffer is a frame of cells over an area: what components draw into. Painting composites over
// what is already there (the kit's compositing needs the cells, which a string can't carry).
type Buffer struct {
	Area  Rect
	Cells []Cell
}

// NewBuffer is a blank buffer over area: spaces on the default colors.
func NewBuffer(area Rect) *Buffer {
	area.W, area.H = max(area.W, 0), max(area.H, 0)
	b := &Buffer{Area: area, Cells: make([]Cell, area.W*area.H)}
	for i := range b.Cells {
		b.Cells[i] = blankCell
	}
	return b
}

// At is the cell at (x, y), or nil outside the area.
func (b *Buffer) At(x, y int) *Cell {
	if !b.Area.Contains(x, y) {
		return nil
	}
	return &b.Cells[(y-b.Area.Y)*b.Area.W+(x-b.Area.X)]
}

// Reset blanks a cell.
func (c *Cell) Reset() { *c = blankCell }
