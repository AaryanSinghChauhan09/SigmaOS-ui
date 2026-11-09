package ui

import (
	"fmt"
	"slices"
	"strconv"
	"strings"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// Tables, trees, split panes, sidebars and the month grid (spec/terminal.md, Data). Rows share one
// ladder: the hover fill on the cursor row, else the selected fill (bold, selected text) on the
// selected one, else nothing.

func rowLook(t *Theme, cursor, selected bool) (Rgba, Ink) {
	c := &t.Shown().Controls
	f := omarchy.Transparent
	if cursor {
		f = c.Hover.Fill
	} else if selected {
		f = c.Selected.Fill
	}
	if selected {
		return f, bold(c.Selected.Text, true)
	}
	return f, ink(t.Fg())
}

// ---------------------------------------------------------------------------------------------
// Table

// TableColumn is a column: a title, a fixed width in cells (else it shares the slack),
// right-aligned or not.
type TableColumn struct {
	title string
	width int
	right bool
}

func NewTableColumn(title string) TableColumn { return TableColumn{title: title} }

func (c TableColumn) Width(w int) TableColumn { c.width = w; return c }
func (c TableColumn) Right() TableColumn      { c.right = true; return c }

// Table is a header of bold textDim titles (the sorted one with 󰅃/󰅀), a track rule under it,
// then one row per item, cells padded a column each side and elided with "…". Rows are keyed
// `<key>-<row>`, headers `<key>-col-<column>`.
type Table struct {
	columns                 []TableColumn
	rows                    [][]string
	sortCol                 int
	ascending               bool
	selected, cursor, width int
	key                     string
}

func NewTable(columns ...TableColumn) Table {
	return Table{columns: columns, sortCol: None, selected: None, cursor: None, width: 60}
}

func (b Table) Rows(rows [][]string) Table { b.rows = rows; return b }
func (b Table) Sort(column int, ascending bool) Table {
	b.sortCol, b.ascending = column, ascending
	return b
}
func (b Table) Selected(r int) Table { b.selected = r; return b }
func (b Table) Cursor(r int) Table   { b.cursor = r; return b }
func (b Table) Width(w int) Table    { b.width = w; return b }
func (b Table) Key(k string) Table   { b.key = k; return b }

// Widths are the column widths: fixed ones as given, the rest share floor((width − fixed) / flex).
func (b Table) Widths(total int) []int {
	fixed, flex := 0, 0
	for _, c := range b.columns {
		if c.width > 0 {
			fixed += c.width
		} else {
			flex++
		}
	}
	share := 0
	if flex > 0 {
		share = sub(total, fixed) / flex
	}
	out := make([]int, len(b.columns))
	for i, c := range b.columns {
		out[i] = share
		if c.width > 0 {
			out[i] = c.width
		}
	}
	return out
}

// HandleTableKey: Up/Down/k/j, Home/End, Page Up/Down move cursor among count rows. True when
// handled.
func HandleTableKey(cursor *int, count int, msg tea.KeyPressMsg) bool {
	last := sub(count, 1)
	switch keyOf(msg) {
	case "down", "j":
		*cursor = min(*cursor+1, last)
	case "up", "k":
		*cursor = sub(*cursor, 1)
	case "home":
		*cursor = 0
	case "end":
		*cursor = last
	case "pgdown":
		*cursor = min(*cursor+5, last)
	case "pgup":
		*cursor = sub(*cursor, 5)
	default:
		return false
	}
	return true
}

func (b Table) Size(*Theme) (int, int) { return b.width, 2 + len(b.rows) }

func (b Table) Draw(t *Theme, area Rect, buf *Buffer) {
	ws := b.Widths(area.W)
	x := area.X
	for i, c := range b.columns {
		w := ws[i]
		inner := sub(w, 2)
		hk := bold(t.Dim(), true)
		chevron := ""
		if b.sortCol == i {
			chevron = GlyphChevronDown
			if b.ascending {
				chevron = GlyphChevronUp
			}
		}
		room := inner
		if chevron != "" {
			room = sub(inner, 2)
		}
		tw := min(width(c.title), room)
		if c.right {
			start := x + 1 + inner - tw
			omarchy.TextFit(t, buf, start, area.Y, c.title, tw, hk)
			if chevron != "" {
				omarchy.Put(t, buf, sub(start, 2), area.Y, chevron, t.Dim())
			}
		} else {
			omarchy.TextFit(t, buf, x+1, area.Y, c.title, tw, hk)
			if chevron != "" {
				omarchy.Put(t, buf, x+1+tw+1, area.Y, chevron, t.Dim())
			}
		}
		if b.key != "" {
			t.Hit(b.key+"-col-"+strconv.Itoa(i), rect(x, area.Y, w, 1))
		}
		x += w
	}
	omarchy.HLine(t, buf, area.X, area.Y+1, area.W, "─", t.Track())
	for r, row := range b.rows {
		y := area.Y + 2 + r
		if y >= area.Bottom() {
			break
		}
		f, k := rowLook(t, b.cursor == r, b.selected == r)
		omarchy.Fill(t, buf, rect(area.X, y, area.W, 1), f)
		x := area.X
		for i, c := range b.columns {
			w := ws[i]
			s := ""
			if i < len(row) {
				s = row[i]
			}
			inner := sub(w, 2)
			if c.right {
				s = omarchy.Fit(s, inner)
				omarchy.Text(t, buf, x+1+inner-width(s), y, s, inner, k)
			} else {
				omarchy.TextFit(t, buf, x+1, y, s, inner, k)
			}
			x += w
		}
		if b.key != "" {
			t.Hit(b.key+"-"+strconv.Itoa(r), rect(area.X, y, area.W, 1))
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Tree

// TreeNode is a node: id, label, optional glyph, children.
type TreeNode struct {
	ID, Label, Glyph string
	Children         []TreeNode
}

// Node is a TreeNode.
func Node(id, label, glyph string, children ...TreeNode) TreeNode {
	return TreeNode{ID: id, Label: label, Glyph: glyph, Children: children}
}

// TreeRow is a visible row: its depth and node.
type TreeRow struct {
	Depth int
	Node  *TreeNode
}

// VisibleTree is the visible rows, depth first.
func VisibleTree(nodes []TreeNode, expanded []string) []TreeRow {
	var out []TreeRow
	var walk func(ns []TreeNode, depth int)
	walk = func(ns []TreeNode, depth int) {
		for i := range ns {
			n := &ns[i]
			out = append(out, TreeRow{depth, n})
			if len(n.Children) > 0 && slices.Contains(expanded, n.ID) {
				walk(n.Children, depth+1)
			}
		}
	}
	walk(nodes, 0)
	return out
}

// Tree is FileTree-style rows: an inset of 1 + 2 × depth, the chevron (󰅂 closed, 󰅀 open,
// textDim) and a space, the glyph (textDim; selected text when selected) and a space, the label.
// Rows are keyed `<key>-<id>`, chevrons `<key>-<id>-toggle`.
type Tree struct {
	nodes            []TreeNode
	expanded         []string
	selected, cursor string
	width            int
	key              string
}

func NewTree(nodes ...TreeNode) Tree { return Tree{nodes: nodes, width: 28} }

func (r Tree) Expanded(ids ...string) Tree { r.expanded = ids; return r }
func (r Tree) Selected(id string) Tree     { r.selected = id; return r }
func (r Tree) Cursor(id string) Tree       { r.cursor = id; return r }
func (r Tree) Width(w int) Tree            { r.width = w; return r }
func (r Tree) Key(k string) Tree           { r.key = k; return r }

func (r Tree) Size(*Theme) (int, int) { return r.width, len(VisibleTree(r.nodes, r.expanded)) }

func (r Tree) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	for i, row := range VisibleTree(r.nodes, r.expanded) {
		y := area.Y + i
		if y >= area.Bottom() {
			break
		}
		n := row.Node
		selected := r.selected != "" && r.selected == n.ID
		f, k := rowLook(t, r.cursor != "" && r.cursor == n.ID, selected)
		omarchy.Fill(t, buf, rect(area.X, y, area.W, 1), f)
		x := area.X + 1 + 2*row.Depth
		if r.key != "" {
			t.Hit(r.key+"-"+n.ID, rect(area.X, y, area.W, 1))
		}
		if len(n.Children) > 0 {
			chevron := GlyphChevronRight
			if slices.Contains(r.expanded, n.ID) {
				chevron = GlyphChevronDown
			}
			omarchy.Put(t, buf, x, y, chevron, t.Dim())
			if r.key != "" {
				t.Hit(r.key+"-"+n.ID+"-toggle", rect(x, y, 1, 1))
			}
		}
		x += 2
		if n.Glyph != "" {
			g := t.Dim()
			if selected {
				g = c.Selected.Text
			}
			omarchy.Put(t, buf, x, y, n.Glyph, g)
			x += 2
		}
		omarchy.TextFit(t, buf, x, y, n.Label, sub(area.Right(), x+1), k)
	}
}

// ---------------------------------------------------------------------------------------------
// SplitView

// SplitPane is a pane of a SplitView: content, and a size in cells (0: it fills).
type SplitPane struct {
	content   Component
	size, min int
	hidden    bool
}

func NewSplitPane(content Component) SplitPane { return SplitPane{content: content} }

func (p SplitPane) SizeCells(s int) SplitPane { p.size = s; return p }
func (p SplitPane) Min(m int) SplitPane       { p.min = m; return p }
func (p SplitPane) Visible(on bool) SplitPane { p.hidden = !on; return p }

// SplitView is panes side by side with a one-column handle between each pair (keyed
// `split-handle-<i>`): `│` in track, in the hover fill while hot.
type SplitView struct {
	panes         []SplitPane
	cursor        int
	width, height int
}

func NewSplitView(width, height int) SplitView {
	return SplitView{cursor: None, width: width, height: height}
}

func (v SplitView) Pane(p ...SplitPane) SplitView {
	v.panes = append(append([]SplitPane(nil), v.panes...), p...)
	return v
}

// Cursor is the handle (0-based) drawn hot.
func (v SplitView) Cursor(handle int) SplitView { v.cursor = handle; return v }

func (v SplitView) Size(*Theme) (int, int) { return v.width, v.height }

func (v SplitView) Draw(t *Theme, area Rect, buf *Buffer) {
	var panes []SplitPane
	for _, p := range v.panes {
		if !p.hidden {
			panes = append(panes, p)
		}
	}
	handles := sub(len(panes), 1)
	fixed, flex := 0, 0
	for _, p := range panes {
		if p.size > 0 {
			fixed += max(p.size, p.min)
		} else {
			flex++
		}
	}
	share := 0
	if flex > 0 {
		share = sub(area.W, fixed+handles) / flex
	}
	x := area.X
	for i, p := range panes {
		w := share
		if p.size > 0 {
			w = max(p.size, p.min)
		}
		p.content.Draw(t, rect(x, area.Y, w, area.H).Intersect(area), buf)
		x += w
		if i+1 < len(panes) {
			hr := rect(x, area.Y, 1, area.H)
			if v.cursor == i {
				omarchy.Fill(t, buf, hr, t.Shown().Controls.Hover.Fill)
			}
			omarchy.VLine(t, buf, x, area.Y, area.H, "│", t.Track())
			t.Hit("split-handle-"+strconv.Itoa(i), hr)
			x++
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Sidebar

// SidebarItem is a row of a Sidebar.
type SidebarItem struct{ ID, Label, Glyph string }

// SidebarSection is a titled group of rows.
type SidebarSection struct {
	Title string
	Items []SidebarItem
}

// Sidebar is sections a row apart: a SectionHeader inset a column, then rows of ` G  Label`
// (glyph textDim, selected text when selected; keyed `<key>-<id>`). Collapsed: the glyphs alone,
// a 5-column rail.
type Sidebar struct {
	sections         []SidebarSection
	selected, cursor string
	collapsed        bool
	width            int
	key              string
}

// SidebarRail is a collapsed Sidebar's width.
const SidebarRail = 5

func NewSidebar(sections ...SidebarSection) Sidebar { return Sidebar{sections: sections, width: 25} }

func (s Sidebar) Selected(id string) Sidebar { s.selected = id; return s }
func (s Sidebar) Cursor(id string) Sidebar   { s.cursor = id; return s }
func (s Sidebar) Collapsed(on bool) Sidebar  { s.collapsed = on; return s }
func (s Sidebar) Width(w int) Sidebar        { s.width = w; return s }
func (s Sidebar) Key(k string) Sidebar       { s.key = k; return s }

func (s Sidebar) Size(*Theme) (int, int) {
	n, rows := len(s.sections), 0
	for _, sec := range s.sections {
		rows += len(sec.Items)
	}
	headers, w := n, s.width
	if s.collapsed {
		headers, w = 0, SidebarRail
	}
	return w, rows + headers + sub(n, 1)
}

func (s Sidebar) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	y := area.Y
	for si, sec := range s.sections {
		if si > 0 {
			y++
		}
		if !s.collapsed {
			omarchy.TextFit(t, buf, area.X+1, y, sec.Title, sub(area.W, 2), bold(t.Dim(), true))
			y++
		}
		for _, it := range sec.Items {
			if y >= area.Bottom() {
				return
			}
			selected := s.selected != "" && s.selected == it.ID
			f, k := rowLook(t, s.cursor != "" && s.cursor == it.ID, selected)
			omarchy.Fill(t, buf, rect(area.X, y, area.W, 1), f)
			gx := area.X + 1
			if s.collapsed {
				gx = area.X + 2
			}
			g := t.Dim()
			if selected {
				g = c.Selected.Text
			}
			omarchy.Put(t, buf, gx, y, it.Glyph, g)
			if !s.collapsed {
				omarchy.TextFit(t, buf, area.X+4, y, it.Label, sub(area.W, 5), k)
			}
			if s.key != "" {
				t.Hit(s.key+"-"+it.ID, rect(area.X, y, area.W, 1))
			}
			y++
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Calendar

// Date is a calendar date.
type Date struct{ Year, Month, Day int }

func NewDate(y, m, d int) Date { return Date{y, m, d} }

func DaysInMonth(year, month int) int {
	switch month {
	case 1, 3, 5, 7, 8, 10, 12:
		return 31
	case 4, 6, 9, 11:
		return 30
	}
	if (year%4 == 0 && year%100 != 0) || year%400 == 0 {
		return 29
	}
	return 28
}

func floorDiv(a, b int) int {
	q := a / b
	if (a%b != 0) && ((a < 0) != (b < 0)) {
		q--
	}
	return q
}

func remEuclid(a, b int) int { return ((a % b) + b) % b }

// ordinal is days since 1970-01-01 (Howard Hinnant's days_from_civil).
func (d Date) ordinal() int {
	y := d.Year
	if d.Month <= 2 {
		y--
	}
	m, dd := d.Month, d.Day
	era := y
	if y < 0 {
		era = y - 399
	}
	era /= 400
	yoe := y - era*400
	mm := m + 9
	if m > 2 {
		mm = m - 3
	}
	doy := (153*mm+2)/5 + dd - 1
	doe := yoe*365 + yoe/4 - yoe/100 + doy
	return era*146097 + doe - 719468
}

func dateFromOrdinal(z int) Date {
	z += 719468
	era := z
	if z < 0 {
		era = z - 146096
	}
	era /= 146097
	doe := z - era*146097
	yoe := (doe - doe/1460 + doe/36524 - doe/146096) / 365
	doy := doe - (365*yoe + yoe/4 - yoe/100)
	mp := (5*doy + 2) / 153
	d := doy - (153*mp+2)/5 + 1
	m := mp - 9
	if mp < 10 {
		m = mp + 3
	}
	y := yoe + era*400
	if m <= 2 {
		y++
	}
	return Date{y, m, d}
}

// Weekday is 0 for Monday … 6 for Sunday.
func (d Date) Weekday() int       { return remEuclid(d.ordinal()+3, 7) }
func (d Date) AddDays(n int) Date { return dateFromOrdinal(d.ordinal() + n) }
func (d Date) AddMonths(n int) Date {
	m0 := d.Year*12 + d.Month - 1 + n
	y, m := floorDiv(m0, 12), remEuclid(m0, 12)+1
	return Date{y, m, min(d.Day, DaysInMonth(y, m))}
}
func (d Date) ISO() string { return fmt.Sprintf("%04d-%02d-%02d", d.Year, d.Month, d.Day) }

var months = [12]string{"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"}
var weekdays = [7]string{"MO", "TU", "WE", "TH", "FR", "SA", "SU"}

// Calendar is a month from Monday: a header row (󰅁 at the start, MONTH YEAR in textDim centered,
// 󰅂 at the end), the weekday row (MO … SU bold textDim), six week rows of 4-column day cells:
// today's edges in accent and bold, the selected day the selected fill (bold, selected text), the
// cursor day the hover fill; weekends textDim, days outside the month textFaint. 28 × 8. Days are
// keyed `<key>-YYYY-MM-DD`, the steppers `<key>-prev` and `<key>-next`.
type Calendar struct {
	year, month             int
	today, selected, cursor *Date
	key                     string
}

func NewCalendar(year, month int) Calendar { return Calendar{year: year, month: month} }

func (c Calendar) Today(d Date) Calendar    { c.today = &d; return c }
func (c Calendar) Selected(d Date) Calendar { c.selected = &d; return c }
func (c Calendar) Cursor(d *Date) Calendar  { c.cursor = d; return c }
func (c Calendar) Key(k string) Calendar    { c.key = k; return c }

// FirstCell is the Monday on or before the 1st.
func FirstCell(year, month int) Date {
	first := Date{year, month, 1}
	return first.AddDays(-first.Weekday())
}

// HandleCalendarKey: arrows (and h/j/k/l) move cursor by a day or a week, Page Up/Down by a
// month; the shown month follows the cursor. True when handled.
func HandleCalendarKey(cursor *Date, year, month *int, msg tea.KeyPressMsg) bool {
	switch keyOf(msg) {
	case "left", "h":
		*cursor = cursor.AddDays(-1)
	case "right", "l":
		*cursor = cursor.AddDays(1)
	case "up", "k":
		*cursor = cursor.AddDays(-7)
	case "down", "j":
		*cursor = cursor.AddDays(7)
	case "pgup":
		*cursor = cursor.AddMonths(-1)
	case "pgdown":
		*cursor = cursor.AddMonths(1)
	default:
		return false
	}
	*year, *month = cursor.Year, cursor.Month
	return true
}

func (c Calendar) Size(*Theme) (int, int) { return 28, 8 }

func eqDate(p *Date, d Date) bool { return p != nil && *p == d }

func (c Calendar) Draw(t *Theme, area Rect, buf *Buffer) {
	ctl := &t.Shown().Controls
	omarchy.Put(t, buf, area.X+1, area.Y, GlyphChevronLeft, t.Fg())
	omarchy.Put(t, buf, area.X+26, area.Y, GlyphChevronRight, t.Fg())
	title := strings.ToUpper(months[min(max(c.month, 1), 12)-1]) + " " + strconv.Itoa(c.year)
	omarchy.TextCenter(t, buf, area.X+3, area.Y, 22, title, t.Dim())
	if c.key != "" {
		t.Hit(c.key+"-prev", rect(area.X, area.Y, 3, 1))
		t.Hit(c.key+"-next", rect(area.X+25, area.Y, 3, 1))
	}
	for i, w := range weekdays {
		omarchy.Text(t, buf, area.X+4*i+1, area.Y+1, w, 2, bold(t.Dim(), true))
	}
	start := FirstCell(c.year, c.month)
	for n := range 42 {
		d := start.AddDays(n)
		col, row := n%7, n/7
		x, y := area.X+col*4, area.Y+2+row
		if y >= area.Bottom() {
			break
		}
		cell := rect(x, y, 4, 1)
		selected, today := eqDate(c.selected, d), eqDate(c.today, d)
		if eqDate(c.cursor, d) {
			omarchy.Fill(t, buf, cell, ctl.Hover.Fill)
		} else if selected {
			omarchy.Fill(t, buf, cell, ctl.Selected.Fill)
		}
		if today {
			omarchy.Edges(t, buf, x, y, 4, some(t.Accent()))
		}
		var k Ink
		switch {
		case selected:
			k = bold(ctl.Selected.Text, true)
		case d.Month != c.month:
			k = ink(t.Faint())
		case col >= 5:
			k = bold(t.Dim(), today)
		default:
			k = bold(t.Fg(), today)
		}
		omarchy.Text(t, buf, x+1, y, fmt.Sprintf("%2d", d.Day), 2, k)
		if c.key != "" {
			t.Hit(c.key+"-"+d.ISO(), cell)
		}
	}
}
