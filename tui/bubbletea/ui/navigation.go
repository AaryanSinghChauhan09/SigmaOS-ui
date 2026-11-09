package ui

import (
	"strconv"
	"strings"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// Menus, keys and the chrome apps move around with (spec/terminal.md, Navigation).

// ---------------------------------------------------------------------------------------------
// Keycap, KeyCombo

// KeyLabel is how a key name is drawn on a cap (the same table in every binding).
func KeyLabel(key string) string {
	k := strings.TrimSpace(key)
	switch strings.ToLower(k) {
	case "super", "meta", "win", "cmd", "logo":
		return "Super"
	case "ctrl", "control":
		return "Ctrl"
	case "alt", "option":
		return "Alt"
	case "shift":
		return "⇧"
	case "enter", "return":
		return "⏎"
	case "backspace":
		return "⌫"
	case "esc", "escape":
		return "Esc"
	case "delete", "del":
		return "Del"
	case "up":
		return "↑"
	case "down":
		return "↓"
	case "left":
		return "←"
	case "right":
		return "→"
	case "pageup":
		return "PgUp"
	case "pagedown":
		return "PgDn"
	case "tab":
		return "Tab"
	case "space":
		return "Space"
	case "home":
		return "Home"
	case "end":
		return "End"
	}
	if len([]rune(k)) == 1 {
		return strings.ToUpper(k)
	}
	return k
}

// SplitCombo is "Ctrl+Shift+S" as its keys; a lone `+` is the plus key ("Ctrl++").
func SplitCombo(combo string) []string {
	var out []string
	var cur []rune
	chars := []rune(combo)
	for i := 0; i < len(chars); i++ {
		c := chars[i]
		switch {
		case c == '+' && len(cur) > 0:
			out = append(out, string(cur))
			cur = nil
		case c == '+':
			out = append(out, "+")
			cur = nil
			if i+1 < len(chars) && chars[i+1] == '+' {
				i++
			}
		default:
			cur = append(cur, c)
		}
	}
	if len(cur) > 0 {
		out = append(out, string(cur))
	}
	return out
}

// Keycap is one key: ` Label ` in the normal fill between the normal border's edges.
type Keycap string

func (k Keycap) Size(*Theme) (int, int) { return width(KeyLabel(string(k))) + 4, 1 }

func (k Keycap) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	control(t, buf, area.X, area.Y, area.W, Look{Fill: c.Normal.Fill, Border: some(c.Normal.Border.Color), Text: t.Fg()})
	omarchy.Text(t, buf, area.X+2, area.Y, KeyLabel(string(k)), sub(area.W, 4), t.Fg())
}

// KeyCombo is a combination, "Ctrl+Shift+P", drawn as Keycaps side by side.
type KeyCombo string

func (k KeyCombo) caps() []Keycap {
	var out []Keycap
	for _, s := range SplitCombo(string(k)) {
		out = append(out, Keycap(s))
	}
	return out
}

func (k KeyCombo) Size(t *Theme) (int, int) {
	w := 0
	for _, c := range k.caps() {
		cw, _ := c.Size(t)
		w += cw
	}
	return w, 1
}

func (k KeyCombo) Draw(t *Theme, area Rect, buf *Buffer) {
	x := area.X
	for _, c := range k.caps() {
		w, _ := c.Size(t)
		c.Draw(t, rect(x, area.Y, w, 1).Intersect(area), buf)
		x += w
	}
}

// ---------------------------------------------------------------------------------------------
// Menu

// MenuItem is one row of a [Menu].
type MenuItem struct {
	label, icon, shortcut                  string
	check, radio                           *bool
	submenu, disabled, destructive, cursor bool
	key                                    string
}

func NewMenuItem(label string) MenuItem { return MenuItem{label: label} }

func (i MenuItem) Icon(g string) MenuItem       { i.icon = g; return i }
func (i MenuItem) Shortcut(s string) MenuItem   { i.shortcut = s; return i }
func (i MenuItem) Submenu(on bool) MenuItem     { i.submenu = on; return i }
func (i MenuItem) Disabled(on bool) MenuItem    { i.disabled = on; return i }
func (i MenuItem) Destructive(on bool) MenuItem { i.destructive = on; return i }
func (i MenuItem) Cursor(on bool) MenuItem      { i.cursor = on; return i }
func (i MenuItem) Key(k string) MenuItem        { i.key = k; return i }

// Check makes a checkable row, checked or not.
func (i MenuItem) Check(on bool) MenuItem { i.check = &on; return i }

// Radio makes an exclusive row, chosen or not.
func (i MenuItem) Radio(on bool) MenuItem { i.radio = &on; return i }

func (i MenuItem) IsDisabled() bool { return i.disabled }

func (i MenuItem) mark() string {
	switch {
	case i.check != nil && *i.check:
		return GlyphCheck
	case i.radio != nil && *i.radio:
		return GlyphDot
	case i.submenu:
		return GlyphSubmenu
	}
	return ""
}

func (i MenuItem) hasMarkSlot() bool { return i.check != nil || i.radio != nil || i.submenu }

type entryKind int

const (
	entryItem entryKind = iota
	entrySeparator
	entrySection
)

type menuEntry struct {
	kind  entryKind
	item  MenuItem
	label string
}

// Menu is a menu on the menu surface: a box in the menu border color, one row per item
// (`[glyph ]label ··· hint [mark]`), the cursor row in menu.selectedBackground with
// menu.selectedText; separators are track rules, sections SectionHeaders. A ContextMenu is the
// same component, placed at the pointer by the app.
type Menu struct {
	entries  []menuEntry
	minWidth int
}

// ContextMenu opens at a point rather than under a trigger: the same component.
type ContextMenu = Menu

func NewMenu() Menu { return Menu{minWidth: 20} }

func (m Menu) add(e menuEntry) Menu {
	m.entries = append(append([]menuEntry(nil), m.entries...), e)
	return m
}

func (m Menu) Item(items ...MenuItem) Menu {
	for _, i := range items {
		m = m.add(menuEntry{kind: entryItem, item: i})
	}
	return m
}
func (m Menu) Separator() Menu           { return m.add(menuEntry{kind: entrySeparator}) }
func (m Menu) Section(label string) Menu { return m.add(menuEntry{kind: entrySection, label: label}) }
func (m Menu) MinWidth(w int) Menu       { m.minWidth = w; return m }

func (m Menu) columns() (icons, marks bool) {
	for _, e := range m.entries {
		if e.kind == entryItem {
			icons = icons || e.item.icon != ""
			marks = marks || e.item.hasMarkSlot()
		}
	}
	return
}

func (m Menu) rowWidth(i MenuItem) int {
	icons, marks := m.columns()
	w := 2 + width(i.label)
	if icons {
		w += 2
	}
	if i.shortcut != "" {
		w += 2 + width(i.shortcut)
	}
	if marks {
		w += 2
	}
	return w
}

// HandleMenuKey moves a menu's cursor (an index among its items, disabled ones skipped):
// Up/Down and k/j wrap, Home/End jump. enabled[i] says whether item i takes the cursor. True
// when handled.
func HandleMenuKey(cursor *int, enabled []bool, msg tea.KeyPressMsg) bool {
	n := len(enabled)
	any := false
	for _, e := range enabled {
		any = any || e
	}
	if n == 0 || !any {
		return false
	}
	step := func(from, d int) int {
		i := from
		for range n {
			i = ((i+d)%n + n) % n
			if enabled[i] {
				return i
			}
		}
		return from
	}
	switch keyOf(msg) {
	case "down", "j":
		*cursor = step(*cursor, 1)
	case "up", "k":
		*cursor = step(*cursor, -1)
	case "home":
		*cursor = step(n-1, 1)
	case "end":
		*cursor = step(0, -1)
	default:
		return false
	}
	return true
}

func (m Menu) Size(*Theme) (int, int) {
	w := m.minWidth
	for _, e := range m.entries {
		if e.kind == entryItem {
			w = max(w, m.rowWidth(e.item))
		}
	}
	return w + 2, len(m.entries) + 2
}

func (m Menu) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Clear(buf, area)
	s := &t.Shown().Surfaces.Menu
	inner := omarchy.Frame(t, buf, area, s.Border.Color, "", Ink{})
	icons, marks := m.columns()
	for n, e := range m.entries {
		y := inner.Y + n
		if y >= inner.Bottom() {
			break
		}
		switch e.kind {
		case entrySeparator:
			omarchy.HLine(t, buf, inner.X, y, inner.W, "─", t.Track())
		case entrySection:
			omarchy.TextFit(t, buf, inner.X+1, y, e.label, sub(inner.W, 2), bold(t.Dim(), true))
		default:
			i := e.item
			row := rect(inner.X, y, inner.W, 1)
			hot := i.cursor && !i.disabled
			if hot {
				omarchy.Fill(t, buf, row, s.SelectedBackground)
			}
			k := s.Text
			switch {
			case i.disabled:
				k = t.Faint()
			case i.destructive:
				k = t.Urgent()
			case hot:
				k = s.SelectedText
			}
			x := row.X + 1
			if icons {
				if i.icon != "" {
					omarchy.Put(t, buf, x, y, i.icon, k)
				}
				x += 2
			}
			markW := 0
			if marks {
				markW = 2
			}
			right := sub(row.Right(), 1+markW)
			hintW := 0
			if i.shortcut != "" {
				hintW = width(i.shortcut) + 2
			}
			omarchy.TextFit(t, buf, x, y, i.label, sub(right, x+hintW), k)
			if i.shortcut != "" {
				hk := t.Dim()
				if i.disabled {
					hk = t.Faint()
				}
				omarchy.TextRight(t, buf, right, y, i.shortcut, hk)
			}
			if mark := i.mark(); mark != "" {
				mk := bold(t.Accent(), true)
				if i.disabled {
					mk = ink(t.Faint())
				} else if i.submenu && i.check == nil && i.radio == nil {
					mk = ink(t.Dim())
				}
				omarchy.Put(t, buf, sub(row.Right(), 2), y, mark, mk)
			}
			if i.key != "" && !i.disabled {
				t.Hit(i.key, row)
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------
// ShortcutSheet

// Shortcut is a key combination and what it does.
type Shortcut struct{ Combo, Description string }

type shortcutGroup struct {
	title string
	rows  []Shortcut
}

// ShortcutSheet is grouped shortcuts in Columns columns (4 apart): each group a SectionHeader,
// then rows of a KeyCombo column and the description two columns after it.
type ShortcutSheet struct {
	groups  []shortcutGroup
	columns int
}

func NewShortcutSheet() ShortcutSheet { return ShortcutSheet{columns: 2} }

func (s ShortcutSheet) Group(title string, rows ...Shortcut) ShortcutSheet {
	s.groups = append(append([]shortcutGroup(nil), s.groups...), shortcutGroup{title, rows})
	return s
}
func (s ShortcutSheet) Columns(n int) ShortcutSheet { s.columns = max(n, 1); return s }

func groupSize(t *Theme, rows []Shortcut) (w, h, comboW int) {
	desc := 0
	for _, r := range rows {
		cw, _ := KeyCombo(r.Combo).Size(t)
		comboW = max(comboW, cw)
		desc = max(desc, width(r.Description))
	}
	return comboW + 2 + desc, 1 + len(rows), comboW
}

func (s ShortcutSheet) layout() [][]int {
	cols := make([][]int, s.columns)
	for i := range s.groups {
		cols[i%s.columns] = append(cols[i%s.columns], i)
	}
	return cols
}

func (s ShortcutSheet) colW(t *Theme, col []int) int {
	cw := 0
	for _, g := range col {
		w, _, _ := groupSize(t, s.groups[g].rows)
		cw = max(cw, max(w, width(s.groups[g].title)))
	}
	return cw
}

func (s ShortcutSheet) Size(t *Theme) (int, int) {
	w, h := 0, 0
	for ci, col := range s.layout() {
		ch := sub(len(col), 1)
		for _, g := range col {
			_, gh, _ := groupSize(t, s.groups[g].rows)
			ch += gh
		}
		w += s.colW(t, col)
		if ci > 0 {
			w += 4
		}
		h = max(h, ch)
	}
	return w, h
}

func (s ShortcutSheet) Draw(t *Theme, area Rect, buf *Buffer) {
	x := area.X
	for _, col := range s.layout() {
		cw := s.colW(t, col)
		y := area.Y
		for _, g := range col {
			grp := s.groups[g]
			_, gh, comboW := groupSize(t, grp.rows)
			omarchy.TextFit(t, buf, x, y, grp.title, cw, bold(t.Dim(), true))
			for i, r := range grp.rows {
				ry := y + 1 + i
				kc := KeyCombo(r.Combo)
				kw, _ := kc.Size(t)
				kc.Draw(t, rect(x, ry, kw, 1).Intersect(area), buf)
				omarchy.TextFit(t, buf, x+comboW+2, ry, r.Description, sub(cw, comboW+2), t.Fg())
			}
			y += gh + 1
		}
		x += cw + 4
	}
}

// ---------------------------------------------------------------------------------------------
// Toolbar

// ToolbarSeparator is a track rule `│` with a column clear each side: it splits a toolbar's
// groups.
type ToolbarSeparator struct{}

func (ToolbarSeparator) Size(*Theme) (int, int) { return 3, 1 }
func (ToolbarSeparator) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Put(t, buf, area.X+1, area.Y, "│", t.Track())
}

// Toolbar is three slots on one row: leading at the left, center centered on the bar, trailing
// at the right, then the overflow IconButton ("toolbar-overflow"); a track rule along the bottom
// row when divided.
type Toolbar struct {
	leading, center, trailing           []Component
	overflow, overflowCursor, undivided bool
	width                               int
}

func NewToolbar(width int) Toolbar { return Toolbar{width: width} }

func (b Toolbar) Leading(k ...Component) Toolbar { b.leading = appendKids(b.leading, k...); return b }
func (b Toolbar) Center(k ...Component) Toolbar  { b.center = appendKids(b.center, k...); return b }
func (b Toolbar) Trailing(k ...Component) Toolbar {
	b.trailing = appendKids(b.trailing, k...)
	return b
}
func (b Toolbar) Overflow(on bool) Toolbar       { b.overflow = on; return b }
func (b Toolbar) OverflowCursor(on bool) Toolbar { b.overflowCursor = on; return b }
func (b Toolbar) Divided(on bool) Toolbar        { b.undivided = !on; return b }

func slotW(t *Theme, ds []Component) int {
	w := 0
	for _, d := range ds {
		dw, _ := d.Size(t)
		w += dw
	}
	return w
}

func (b Toolbar) height(t *Theme) int {
	h, any := 0, false
	for _, ds := range [][]Component{b.leading, b.center, b.trailing} {
		for _, d := range ds {
			_, dh := d.Size(t)
			h, any = max(h, dh), true
		}
	}
	if !any {
		return 1
	}
	return h
}

func (b Toolbar) Size(t *Theme) (int, int) {
	h := b.height(t)
	if !b.undivided {
		h++
	}
	return b.width, h
}

func (b Toolbar) Draw(t *Theme, area Rect, buf *Buffer) {
	h := b.height(t)
	place := func(x int, ds []Component) {
		for _, d := range ds {
			w, dh := d.Size(t)
			d.Draw(t, rect(x, area.Y+(h-dh)/2, w, dh).Intersect(area), buf)
			x += w
		}
	}
	place(area.X, b.leading)
	place(area.X+sub(area.W, slotW(t, b.center))/2, b.center)
	ow := 0
	if b.overflow {
		ow = 5
	}
	place(sub(area.Right(), slotW(t, b.trailing)+ow), b.trailing)
	if b.overflow {
		NewIconButton("").Cursor(b.overflowCursor).Key("toolbar-overflow").Draw(t, rect(area.Right()-5, area.Y+(h-1)/2, 5, 1), buf)
	}
	if !b.undivided {
		omarchy.HLine(t, buf, area.X, area.Y+h, area.W, "─", t.Track())
	}
}

// ---------------------------------------------------------------------------------------------
// Tabs, IconRail

// Tabs are text tabs ` Label ` (keyed `tab-<i>`) over a track rule the control's width; the
// selected tab bold in controls.selected.text with its span of the rule `━` in accent. Two rows.
type Tabs struct {
	labels          []string
	current, cursor int
	width           int
}

func NewTabs(labels []string, current int) Tabs {
	return Tabs{labels: labels, current: current, cursor: None}
}

func (b Tabs) Cursor(i int) Tabs { b.cursor = i; return b }

// Width is the rule's width (else the tabs').
func (b Tabs) Width(w int) Tabs { b.width = w; return b }

// HandleTabsKey: Left/h, Right/l move current within count, Home/End jump. True when handled.
func HandleTabsKey(current *int, count int, msg tea.KeyPressMsg) bool {
	return HandleButtonGroupKey(current, count, msg)
}

func (b Tabs) Size(*Theme) (int, int) {
	if b.width > 0 {
		return b.width, 2
	}
	w := 0
	for _, l := range b.labels {
		w += width(l) + 2
	}
	return w, 2
}

func (b Tabs) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	omarchy.HLine(t, buf, area.X, area.Y+1, area.W, "─", t.Track())
	x := area.X
	for i, l := range b.labels {
		w := width(l) + 2
		r := rect(x, area.Y, w, 1).Intersect(area)
		selected, hot := i == b.current, b.cursor == i
		if hot {
			omarchy.Fill(t, buf, r, c.Hover.Fill)
		}
		k := ink(t.Dim())
		if selected {
			k = bold(c.Selected.Text, true)
		} else if hot {
			k = ink(t.Fg())
		}
		omarchy.Text(t, buf, x+1, area.Y, l, sub(w, 2), k)
		if selected {
			omarchy.HLine(t, buf, x, area.Y+1, min(w, sub(area.Right(), x)), "━", t.Accent())
		}
		t.Hit("tab-"+strconv.Itoa(i), r)
		x += w
	}
}

// RailItem is a rail entry: a glyph and its label.
type RailItem struct{ Glyph, Label string }

// IconRail is a vertical tab rail: one row per item ` G ` (keyed `rail-<i>`), the selected glyph
// accent with a `▎` accent bar at the rail's leading edge, the cursor item in the hover fill, a
// track rule `│` down its trailing edge. Five columns.
type IconRail struct {
	items           []RailItem
	current, cursor int
	height          int
}

func NewIconRail(items []RailItem, current int) IconRail {
	return IconRail{items: items, current: current, cursor: None}
}

func (r IconRail) Cursor(i int) IconRail { r.cursor = i; return r }

// Height is the rule's height (else one row per item).
func (r IconRail) Height(h int) IconRail { r.height = h; return r }

// HandleIconRailKey: Up/k, Down/j move current, Home/End jump. True when handled.
func HandleIconRailKey(current *int, count int, msg tea.KeyPressMsg) bool {
	switch keyOf(msg) {
	case "up", "k":
		*current = sub(*current, 1)
	case "down", "j":
		*current = min(*current+1, sub(count, 1))
	case "home":
		*current = 0
	case "end":
		*current = sub(count, 1)
	default:
		return false
	}
	return true
}

func (r IconRail) Size(*Theme) (int, int) {
	if r.height > 0 {
		return 5, r.height
	}
	return 5, len(r.items)
}

func (r IconRail) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	omarchy.VLine(t, buf, area.X+4, area.Y, area.H, "│", t.Track())
	for i, it := range r.items {
		y := area.Y + i
		if y >= area.Bottom() {
			break
		}
		hot := r.cursor == i
		if hot {
			omarchy.Fill(t, buf, rect(area.X+1, y, 3, 1), c.Hover.Fill)
		}
		selected := i == r.current
		if selected {
			omarchy.Put(t, buf, area.X, y, "▎", t.Accent())
		}
		k := t.Dim()
		if selected {
			k = t.Accent()
		} else if hot {
			k = t.Fg()
		}
		omarchy.Put(t, buf, area.X+2, y, it.Glyph, k)
		t.Hit("rail-"+strconv.Itoa(i), rect(area.X, y, 4, 1))
	}
}
