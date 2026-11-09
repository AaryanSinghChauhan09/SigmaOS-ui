package ui

import (
	"strconv"
	"strings"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// SearchList: the type-ahead overlay (spec/terminal.md, Composed).

// SearchItem is one result: title, subtitle, glyph, value, detail.
type SearchItem struct{ Title, Subtitle, Glyph, Value, Detail string }

// SearchKey is what a key did to a SearchList.
type SearchKey int

const (
	SearchIgnored SearchKey = iota
	// SearchQuery: the query changed (filter again; the cursor is back on the first match).
	SearchQuery
	SearchMoved
	SearchActivate
	SearchDelete
	SearchClose
)

// SearchList is on the menu surface: a box in its border color, padded 2 × 1; the filter line
// (the query, or the placeholder in textDim), a blank row, then the body: ListRows under the
// cursor (keyed `<key>-row-<i>`) and, with the detail pane, two columns, a `│` rule in track,
// two columns and the pane (the cursor item's title bold over its detail in textDim, wrapped).
// No items: an overlay EmptyState centered in the body.
type SearchList struct {
	query          string
	items          []SearchItem
	cursor, offset int
	detail         bool
	placeholder    *string
	empty          *[2]string
	width, height  int
	key            string
}

func NewSearchList(key, query string) SearchList {
	return SearchList{key: key, query: query, width: 60, height: 22}
}

func (s SearchList) Items(items ...SearchItem) SearchList { s.items = items; return s }
func (s SearchList) Cursor(i int) SearchList              { s.cursor = i; return s }
func (s SearchList) Detail(on bool) SearchList            { s.detail = on; return s }
func (s SearchList) Placeholder(p string) SearchList      { s.placeholder = &p; return s }
func (s SearchList) SizeCells(w, h int) SearchList        { s.width, s.height = w, h; return s }

// Offset is the first body row shown.
func (s SearchList) Offset(rows int) SearchList { s.offset = rows; return s }

// Empty is what an empty query with no items shows.
func (s SearchList) Empty(glyph, text string) SearchList { s.empty = &[2]string{glyph, text}; return s }

// SearchMatches is the plain case-insensitive substring test.
func SearchMatches(text, query string) bool {
	return strings.Contains(strings.ToLower(text), strings.ToLower(query))
}

// HandleSearchListKey takes the keys with the filter line holding focus: printable keys and
// Space type, Backspace edits, Up, Down, Page Up/Down, Home/End move the cursor, Return
// activates (cursor.Index()), Delete asks to delete, Escape clears the query or, when empty, asks
// to close.
func HandleSearchListKey(query *string, cursor *CursorModel, msg tea.KeyPressMsg) SearchKey {
	if s := Printable(msg); s != "" {
		*query += s
		cursor.Set("", 0)
		return SearchQuery
	}
	switch keyOf(msg) {
	case "space":
		*query += " "
		cursor.Set("", 0)
		return SearchQuery
	case "backspace":
		if r := []rune(*query); len(r) > 0 {
			*query = string(r[:len(r)-1])
		}
		cursor.Set("", 0)
		return SearchQuery
	case "esc":
		if *query != "" {
			*query = ""
			cursor.Set("", 0)
			return SearchQuery
		}
		return SearchClose
	case "up", "down", "pgup", "pgdown", "home", "end":
		cursor.Active = true
		cursor.ApplyKey(msg)
		return SearchMoved
	case "enter":
		return SearchActivate
	case "delete":
		return SearchDelete
	}
	return SearchIgnored
}

func (s SearchList) Size(*Theme) (int, int) { return s.width, s.height }

func (s SearchList) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Clear(buf, area)
	m := &t.Shown().Surfaces.Menu
	inner := omarchy.Frame(t, buf, area, m.Border.Color, "", Ink{}).Inset(2, 1)
	if s.query == "" {
		p := t.String("common.search", "Search...")
		if s.placeholder != nil {
			p = *s.placeholder
		}
		omarchy.TextFit(t, buf, inner.X, inner.Y, p, inner.W, t.Dim())
	} else {
		omarchy.TextFit(t, buf, inner.X, inner.Y, s.query, inner.W, m.Text)
	}
	t.Hit(s.key+"-filter", rect(inner.X, inner.Y, inner.W, 1))
	body := rect(inner.X, inner.Y+2, inner.W, sub(inner.H, 2))
	if len(s.items) == 0 {
		glyph, text := "\U000f0349", ""
		if s.query != "" {
			text = strings.ReplaceAll(t.String("common.noMatchesFor", "No matches for “%1”"), "%1", s.query)
		} else if s.empty != nil {
			glyph, text = s.empty[0], s.empty[1]
		}
		e := NewEmptyState(text).Glyph(glyph).Overlay(true).Width(body.W)
		_, eh := e.Size(t)
		e.Draw(t, rect(body.X, body.Y+sub(body.H, eh)/2, body.W, eh), buf)
		return
	}
	listW := body.W
	if s.detail {
		listW = body.W * 55 / 100
	}
	list := NewScrollArea(listW).MaxRows(body.H).Offset(s.offset)
	for i, it := range s.items {
		list = list.Child(NewListRow(it.Title).Glyph(it.Glyph).Subtitle(it.Subtitle).Value(it.Value).Cursor(s.cursor == i).Width(listW).Key(s.key + "-row-" + strconv.Itoa(i)))
	}
	_, lh := list.Size(t)
	list.Draw(t, rect(body.X, body.Y, listW, lh), buf)
	if s.detail {
		rx := body.X + listW + 2
		omarchy.VLine(t, buf, rx, body.Y, body.H, "│", t.Track())
		px := rx + 3
		pw := sub(body.Right(), px)
		if s.cursor >= 0 && s.cursor < len(s.items) {
			it := s.items[s.cursor]
			omarchy.TextFit(t, buf, px, body.Y, it.Title, pw, bold(m.Text, true))
			for n, line := range omarchy.WrapText(it.Detail, pw) {
				y := body.Y + 1 + n
				if y >= body.Bottom() {
					break
				}
				omarchy.TextFit(t, buf, px, y, line, pw, t.Dim())
			}
		}
	}
}
