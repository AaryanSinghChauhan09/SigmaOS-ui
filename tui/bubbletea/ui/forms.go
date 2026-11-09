package ui

import (
	"slices"
	"strconv"
	"strings"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// Setting rows and the multi-choice dropdown (spec/terminal.md, Forms). Checkbox, Radio,
// RadioGroup, PasswordField, TextArea, ColorWell and SwatchStrip are with the controls; Dialog
// with the surfaces.

// SettingRow is a row of a settings list: the label (and a textDim description under it) at the
// start, any control at the end of the first row. No frame; the cursor row takes the hover fill.
type SettingRow struct {
	label, description string
	control            Component
	cursor, disabled   bool
	width              int
	key                string
}

func NewSettingRow(label string) SettingRow { return SettingRow{label: label, width: 60} }

func (r SettingRow) Description(d string) SettingRow { r.description = d; return r }
func (r SettingRow) Control(c Component) SettingRow  { r.control = c; return r }
func (r SettingRow) Cursor(on bool) SettingRow       { r.cursor = on; return r }
func (r SettingRow) Disabled(on bool) SettingRow     { r.disabled = on; return r }
func (r SettingRow) Width(w int) SettingRow          { r.width = w; return r }
func (r SettingRow) Key(k string) SettingRow         { r.key = k; return r }

func (r SettingRow) Size(t *Theme) (int, int) {
	ch := 1
	if r.control != nil {
		_, ch = r.control.Size(t)
	}
	h := 1
	if r.description != "" {
		h = 2
	}
	return r.width, max(h, ch)
}

func (r SettingRow) Draw(t *Theme, area Rect, buf *Buffer) {
	if r.cursor && !r.disabled {
		omarchy.Fill(t, buf, area, t.Shown().Controls.Hover.Fill)
	}
	cw := 0
	if r.control != nil {
		cw, _ = r.control.Size(t)
	}
	words := sub(area.W, cw+3)
	fg, dim := t.Fg(), t.Dim()
	if r.disabled {
		fg, dim = t.Faint(), t.Faint()
	}
	omarchy.TextFit(t, buf, area.X+1, area.Y, r.label, words, fg)
	if r.description != "" {
		omarchy.TextFit(t, buf, area.X+1, area.Y+1, r.description, words, dim)
	}
	if r.control != nil {
		w, h := r.control.Size(t)
		r.control.Draw(t, rect(sub(area.Right(), w+1), area.Y, w, h), buf)
	}
	if r.key != "" {
		t.Hit(r.key, area)
	}
}

// MultiSelect is the searchable multi-choice dropdown: a label row (bold textDim), the trigger
// (the chosen labels joined by ", ", "N selected" past three, "None selected" in textFaint), and
// when open the popup under it: a box in the popups border color with the search row (󰍉 and the
// query or placeholder, in the focus look), a rule, then a checkbox row per option. The trigger
// is keyed `<key>`, the rows `<key>-<value>`.
type MultiSelect struct {
	label              string
	options            []Option
	values             []string
	open, cursor       bool
	highlighted, width int
	search, key        string
}

func NewMultiSelect(key string, options []Option, values []string) MultiSelect {
	return MultiSelect{key: key, options: options, values: values, highlighted: None, width: 33}
}

func (m MultiSelect) Label(l string) MultiSelect  { m.label = l; return m }
func (m MultiSelect) Open(on bool) MultiSelect    { m.open = on; return m }
func (m MultiSelect) Cursor(on bool) MultiSelect  { m.cursor = on; return m }
func (m MultiSelect) Search(q string) MultiSelect { m.search = q; return m }
func (m MultiSelect) Width(w int) MultiSelect     { m.width = w; return m }

// Highlighted is the row under the keyboard among those the search shows (None: the keys are in
// the search).
func (m MultiSelect) Highlighted(i int) MultiSelect { m.highlighted = i; return m }

func filterOptions(options []Option, search string) []Option {
	q := strings.ToLower(search)
	var out []Option
	for _, o := range options {
		if strings.Contains(strings.ToLower(o.Label), q) {
			out = append(out, o)
		}
	}
	return out
}

// Shown is the options the search shows (case-insensitive substring of the label).
func (m MultiSelect) Shown() []Option { return filterOptions(m.options, m.search) }

func (m MultiSelect) triggerText(t *Theme) (string, bool) {
	var chosen []string
	for _, o := range m.options {
		if slices.Contains(m.values, o.Value) {
			chosen = append(chosen, o.Label)
		}
	}
	switch n := len(chosen); {
	case n == 0:
		return t.String("common.noneSelected", "None selected"), false
	case n > 3:
		s := t.String("multiSelect.selectedCount", "%1 selected")
		return strings.ReplaceAll(strings.ReplaceAll(s, "%1", strconv.Itoa(n)), "{count}", strconv.Itoa(n)), true
	}
	return strings.Join(chosen, ", "), true
}

func (m MultiSelect) popupRows() int { return min(len(m.Shown()), 6) }

// MultiSelectState is the app's state for a MultiSelect's keys.
type MultiSelectState struct {
	Open   bool
	Search string
	// Row is the highlighted row among the shown options, None while the keys are in the search.
	Row int
}

// HandleMultiSelectKey takes the keys as the GUI bindings do. On the closed trigger: j, Down,
// Return or Space open. Open, in the search (Row None): typing filters, Return toggles the first
// match, Down goes into the list. In the list: j/k and Down/Up walk (Up off the first row goes
// back to the search), Return or Space toggles. Escape closes, clearing the search. It returns
// whether the key was used and the value to toggle ("" for none).
func HandleMultiSelectKey(s *MultiSelectState, options []Option, msg tea.KeyPressMsg) (bool, string) {
	shown := filterOptions(options, s.Search)
	k := keyOf(msg)
	if !s.Open {
		switch k {
		case "j", "down", "enter", "space":
			s.Open, s.Row = true, None
			return true, ""
		}
		return false, ""
	}
	if k == "esc" {
		s.Open, s.Search, s.Row = false, "", None
		return true, ""
	}
	if s.Row < 0 {
		switch k {
		case "enter":
			if len(shown) > 0 {
				return true, shown[0].Value
			}
			return true, ""
		case "down":
			if len(shown) > 0 {
				s.Row = 0
			} else {
				s.Row = None
			}
			return true, ""
		case "backspace":
			if r := []rune(s.Search); len(r) > 0 {
				s.Search = string(r[:len(r)-1])
			}
			return true, ""
		}
		if msg.Text != "" {
			s.Search += msg.Text
			return true, ""
		}
		return false, ""
	}
	switch k {
	case "down", "j":
		s.Row = min(s.Row+1, sub(len(shown), 1))
	case "up", "k":
		s.Row--
		if s.Row < 0 {
			s.Row = None
		}
	case "enter", "space":
		if s.Row < len(shown) {
			return true, shown[s.Row].Value
		}
	default:
		return false, ""
	}
	return true, ""
}

func (m MultiSelect) Size(*Theme) (int, int) {
	h := 1
	if m.label != "" {
		h++
	}
	if m.open {
		h += 4 + max(m.popupRows(), 1)
	}
	return m.width, h
}

func (m MultiSelect) Draw(t *Theme, area Rect, buf *Buffer) {
	y := area.Y
	if m.label != "" {
		omarchy.TextFit(t, buf, area.X, y, m.label, area.W, bold(t.Dim(), true))
		y++
	}
	s, chosen := m.triggerText(t)
	k := t.Faint()
	if chosen {
		k = t.Fg()
	}
	trigger(t, buf, area.X, y, area.W, m.cursor, s, k)
	t.Hit(m.key, rect(area.X, y, area.W, 1))
	if !m.open {
		return
	}
	rows := max(m.popupRows(), 1)
	pop := rect(area.X, y+1, area.W, rows+4).Intersect(area)
	omarchy.Clear(buf, pop)
	c := &t.Shown().Controls
	inner := omarchy.Frame(t, buf, pop, t.Shown().Surfaces.Popups.Border.Color, "", Ink{})
	// The search row holds the focus while open: the focus look.
	if m.highlighted < 0 {
		omarchy.Fill(t, buf, rect(inner.X, inner.Y, inner.W, 1), c.Focus.Fill)
	}
	omarchy.Put(t, buf, inner.X+1, inner.Y, "\U000f0349", t.Dim())
	if m.search == "" {
		omarchy.TextFit(t, buf, inner.X+3, inner.Y, t.String("common.search", "Search..."), sub(inner.W, 4), t.Faint())
	} else {
		omarchy.TextFit(t, buf, inner.X+3, inner.Y, m.search, sub(inner.W, 4), t.Fg())
	}
	omarchy.HLine(t, buf, inner.X, inner.Y+1, inner.W, "─", t.Fg().WithAlpha(0.10))
	shown := m.Shown()
	if len(shown) == 0 {
		omarchy.TextFit(t, buf, inner.X+1, inner.Y+2, t.String("common.noOptions", "No options"), sub(inner.W, 2), t.Dim())
	}
	for i, o := range shown {
		if i >= rows {
			break
		}
		ry := inner.Y + 2 + i
		if m.highlighted == i {
			omarchy.Fill(t, buf, rect(inner.X, ry, inner.W, 1), c.Hover.Fill)
		}
		on := slices.Contains(m.values, o.Value)
		checkBox(t, buf, inner.X+1, ry, on, false, false, false)
		omarchy.TextFit(t, buf, inner.X+5, ry, o.Label, sub(inner.W, 6), bold(t.Fg(), on))
		t.Hit(m.key+"-"+o.Value, rect(inner.X, ry, inner.W, 1))
	}
}
