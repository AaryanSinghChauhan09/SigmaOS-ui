package ui

import (
	"math"
	"strconv"
	"strings"

	tea "charm.land/bubbletea/v2"

	"omarchy-ui/tui/bubbletea/omarchy"
)

// One-row controls (spec/terminal.md, Controls): `[edge][pad] content [pad][edge]`, the edge cells
// reserved whenever any state shows a border, the fill across the whole width.

// ---------------------------------------------------------------------------------------------
// Button, IconButton, ButtonGroup

// Button is a label with an optional icon glyph. Borderless at rest unless Bordered; Selected
// fills and bolds; Cursor is the hover-cursor look (keyboard cursor, focus, pointer).
type Button struct {
	label, icon                string
	bordered, selected, cursor bool
	width                      int
	key                        string
}

func NewButton(label string) Button { return Button{label: label} }

func (b Button) Icon(glyph string) Button { b.icon = glyph; return b }
func (b Button) Bordered(on bool) Button  { b.bordered = on; return b }
func (b Button) Selected(on bool) Button  { b.selected = on; return b }
func (b Button) Cursor(on bool) Button    { b.cursor = on; return b }
func (b Button) Key(k string) Button      { b.key = k; return b }

// Width fixes the width (the label centers); by default label + 4.
func (b Button) Width(w int) Button { b.width = w; return b }

func (b Button) Size(*Theme) (int, int) {
	if b.width > 0 {
		return b.width, 1
	}
	n := 4 + width(b.label)
	if b.icon != "" {
		n += 2
	}
	return n, 1
}

func (b Button) Draw(t *Theme, area Rect, buf *Buffer) {
	l := LookOf(t, b.cursor, b.selected, false, b.bordered)
	control(t, buf, area.X, area.Y, area.W, l)
	content := b.label
	if b.icon != "" {
		content = b.icon + " " + b.label
	}
	omarchy.TextCenter(t, buf, area.X+2, area.Y, sub(area.W, 4), content, bold(l.Text, l.Bold))
	if b.key != "" {
		t.Hit(b.key, area)
	}
}

// IconButton is one glyph between edges: 5 columns.
type IconButton struct {
	glyph            string
	bordered, cursor bool
	key              string
}

func NewIconButton(glyph string) IconButton { return IconButton{glyph: glyph} }

func (b IconButton) Bordered(on bool) IconButton { b.bordered = on; return b }
func (b IconButton) Cursor(on bool) IconButton   { b.cursor = on; return b }
func (b IconButton) Key(k string) IconButton     { b.key = k; return b }

func (b IconButton) Size(*Theme) (int, int) { return 5, 1 }

func (b IconButton) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	var l Look
	if b.cursor {
		// PanelActionButton shows the hover border only when bordered.
		l = Look{Fill: c.Hover.Fill, Text: t.Fg()}
		if b.bordered {
			l.Border = borderOf(c.Hover.Border)
		}
	} else {
		l = LookOf(t, false, false, false, b.bordered)
	}
	control(t, buf, area.X, area.Y, area.W, l)
	omarchy.Put(t, buf, area.X+2, area.Y, b.glyph, l.Text)
	if b.key != "" {
		t.Hit(b.key, area)
	}
}

// ButtonGroup is bordered buttons one column apart, one selected: a segmented choice
// ([HandleButtonGroupKey] moves it). Buttons are keyed `<key>-<i>`.
type ButtonGroup struct {
	labels           []string
	selected, cursor int
	key              string
}

func NewButtonGroup(labels []string, selected int) ButtonGroup {
	return ButtonGroup{labels: labels, selected: selected, cursor: None}
}

func (g ButtonGroup) Cursor(i int) ButtonGroup { g.cursor = i; return g }
func (g ButtonGroup) Key(k string) ButtonGroup { g.key = k; return g }

// HandleButtonGroupKey: Left/h and Right/l move selected within count, stopping at the ends;
// Home/End jump. True when handled.
func HandleButtonGroupKey(selected *int, count int, msg tea.KeyPressMsg) bool {
	switch keyOf(msg) {
	case "left", "h":
		*selected = sub(*selected, 1)
	case "right", "l":
		*selected = min(*selected+1, sub(count, 1))
	case "home":
		*selected = 0
	case "end":
		*selected = sub(count, 1)
	default:
		return false
	}
	return true
}

func (g ButtonGroup) buttons() []Button {
	out := make([]Button, len(g.labels))
	for i, l := range g.labels {
		b := NewButton(l).Bordered(true).Selected(i == g.selected).Cursor(g.cursor == i)
		if g.key != "" {
			b = b.Key(g.key + "-" + strconv.Itoa(i))
		}
		out[i] = b
	}
	return out
}

func (g ButtonGroup) Size(t *Theme) (int, int) {
	w := 0
	bs := g.buttons()
	for _, b := range bs {
		bw, _ := b.Size(t)
		w += bw
	}
	return w + sub(len(bs), 1), 1
}

func (g ButtonGroup) Draw(t *Theme, area Rect, buf *Buffer) {
	x := area.X
	for _, b := range g.buttons() {
		w, _ := b.Size(t)
		b.Draw(t, rect(x, area.Y, w, 1).Intersect(area), buf)
		x += w + 1
	}
}

// ---------------------------------------------------------------------------------------------
// ToggleSwitch, Toggle

// ToggleSwitch is four cells of track with the knob `██` at the start (off) or the end (on). With
// the cursor its edges show (the ring), so it is 6 columns; Bare is the 4 alone.
type ToggleSwitch struct {
	on, cursor, bare bool
	key              string
}

func NewToggleSwitch(on bool) ToggleSwitch { return ToggleSwitch{on: on} }

func (s ToggleSwitch) Cursor(on bool) ToggleSwitch { s.cursor = on; return s }

// Bare is the track alone (inside a Toggle or a ListRow): no ring, 4 columns.
func (s ToggleSwitch) Bare() ToggleSwitch        { s.bare = true; return s }
func (s ToggleSwitch) Key(k string) ToggleSwitch { s.key = k; return s }

func switchTrack(t *Theme, buf *Buffer, x, y int, on bool) {
	c := &t.Shown().Controls
	f, knob, kx := c.Normal.Fill, t.Fg().Darker(1.25), x
	if on {
		f, knob, kx = c.Selected.Fill, c.Selected.Text, x+2
	}
	omarchy.Fill(t, buf, rect(x, y, 4, 1), f)
	omarchy.Put(t, buf, kx, y, "█", knob)
	omarchy.Put(t, buf, kx+1, y, "█", knob)
}

func (s ToggleSwitch) Size(*Theme) (int, int) {
	if s.bare {
		return 4, 1
	}
	return 6, 1
}

func (s ToggleSwitch) Draw(t *Theme, area Rect, buf *Buffer) {
	if s.bare {
		switchTrack(t, buf, area.X, area.Y, s.on)
	} else {
		var ring *Rgba
		if s.cursor {
			ring = borderOf(t.Shown().Controls.Hover.Border)
		}
		omarchy.Edges(t, buf, area.X, area.Y, 6, ring)
		switchTrack(t, buf, area.X+1, area.Y, s.on)
	}
	if s.key != "" {
		t.Hit(s.key, area)
	}
}

// Toggle is a labeled row with the switch at its end; the whole row is the control. Two rows with
// a description (title bold, description textDim under it), one without.
type Toggle struct {
	title, description string
	on, cursor         bool
	width              int
	key                string
}

func NewToggle(title string) Toggle { return Toggle{title: title, width: 45} }

func (g Toggle) Description(d string) Toggle { g.description = d; return g }
func (g Toggle) On(on bool) Toggle           { g.on = on; return g }
func (g Toggle) Cursor(on bool) Toggle       { g.cursor = on; return g }
func (g Toggle) Width(w int) Toggle          { g.width = w; return g }
func (g Toggle) Key(k string) Toggle         { g.key = k; return g }

func (g Toggle) Size(*Theme) (int, int) {
	if g.description == "" {
		return g.width, 1
	}
	return g.width, 2
}

func (g Toggle) Draw(t *Theme, area Rect, buf *Buffer) {
	l := LookOf(t, g.cursor, false, true, true)
	for dy := range area.H {
		control(t, buf, area.X, area.Y+dy, area.W, l)
	}
	words := sub(area.W, 4+4+1)
	omarchy.TextFit(t, buf, area.X+2, area.Y, g.title, words, bold(t.Fg(), true))
	if g.description != "" && area.H > 1 {
		omarchy.TextFit(t, buf, area.X+2, area.Y+1, g.description, words, t.Dim())
	}
	switchTrack(t, buf, sub(area.Right(), 6), area.Y, g.on)
	if g.key != "" {
		t.Hit(g.key, area)
	}
}

// ---------------------------------------------------------------------------------------------
// TextField, PasswordField, TextArea

// TextField is a single-line input: normal fill and border at rest, the hover look with the
// cursor, the caret (reverse video) at its index while focused. A bubbles/textinput model can
// hold the value and caret: NewTextField(in.Value()).Caret(in.Position()).
type TextField struct {
	text, placeholder string
	width             int
	cursor            bool
	caret             int
	key               string
}

func NewTextField(text string) TextField { return TextField{text: text, width: 28, caret: None} }

func (f TextField) Placeholder(p string) TextField { f.placeholder = p; return f }
func (f TextField) Width(w int) TextField          { f.width = w; return f }
func (f TextField) Cursor(on bool) TextField       { f.cursor = on; return f }

// Caret shows the caret before this character index (None: not focused).
func (f TextField) Caret(at int) TextField { f.caret = at; return f }
func (f TextField) Key(k string) TextField { f.key = k; return f }

func fieldText(t *Theme, buf *Buffer, x, y, w int, s, placeholder string, caret int, color Rgba) {
	if s == "" {
		omarchy.TextFit(t, buf, x, y, placeholder, w, t.Faint())
	} else {
		omarchy.TextFit(t, buf, x, y, s, w, color)
	}
	if caret >= 0 {
		r := []rune(s)
		cx := x + min(width(string(r[:min(caret, len(r))])), sub(w, 1))
		if c := buf.At(cx, y); c != nil {
			c.Attrs |= omarchy.Reverse
		}
	}
}

func (f TextField) Size(*Theme) (int, int) { return f.width, 1 }

func (f TextField) Draw(t *Theme, area Rect, buf *Buffer) {
	l := LookOf(t, f.cursor, false, true, true)
	control(t, buf, area.X, area.Y, area.W, l)
	fieldText(t, buf, area.X+2, area.Y, sub(area.W, 4), f.text, f.placeholder, f.caret, t.Fg())
	if f.key != "" {
		t.Hit(f.key, area)
	}
}

// PasswordField is a TextField for secrets: a lock before the text, dots for it (and TextField's
// caret while focused), an optional eye at the end, and with an error the urgent edges and the
// message on the row under it.
type PasswordField struct {
	text, placeholder, err       string
	width, caret                 int
	cursor, revealable, revealed bool
	key                          string
}

func NewPasswordField(text string) PasswordField {
	return PasswordField{text: text, width: 28, caret: None}
}

func (f PasswordField) Placeholder(p string) PasswordField { f.placeholder = p; return f }
func (f PasswordField) Width(w int) PasswordField          { f.width = w; return f }
func (f PasswordField) Cursor(on bool) PasswordField       { f.cursor = on; return f }
func (f PasswordField) Caret(at int) PasswordField         { f.caret = at; return f }
func (f PasswordField) Revealable(on bool) PasswordField   { f.revealable = on; return f }
func (f PasswordField) Revealed(on bool) PasswordField     { f.revealed = on; return f }
func (f PasswordField) Error(msg string) PasswordField     { f.err = msg; return f }
func (f PasswordField) Key(k string) PasswordField         { f.key = k; return f }

func (f PasswordField) Size(*Theme) (int, int) {
	if f.err == "" {
		return f.width, 1
	}
	return f.width, 2
}

func (f PasswordField) Draw(t *Theme, area Rect, buf *Buffer) {
	s := t.Shown()
	l := LookOf(t, f.cursor, false, true, true)
	erring := f.err != ""
	if erring {
		l.Border = some(s.Surfaces.Polkit.BorderError.Color)
	}
	control(t, buf, area.X, area.Y, area.W, l)
	glyph := t.Dim()
	if erring {
		glyph = s.Surfaces.Polkit.TextError
	}
	omarchy.Put(t, buf, area.X+2, area.Y, "", glyph)
	eye := 0
	if f.revealable {
		eye = 2
	}
	w := sub(area.W, 6+eye)
	shown := f.text
	if !f.revealed {
		shown = strings.Repeat("•", len([]rune(f.text)))
	}
	fieldText(t, buf, area.X+4, area.Y, w, shown, f.placeholder, f.caret, t.Fg())
	if f.revealable {
		g := "\U000f0208"
		if f.revealed {
			g = "\U000f0209"
		}
		omarchy.Put(t, buf, sub(area.Right(), 3), area.Y, g, t.Dim())
	}
	if erring && area.H > 1 {
		omarchy.TextFit(t, buf, area.X+2, area.Y+1, f.err, sub(area.W, 2), s.Surfaces.Polkit.TextError)
	}
	if f.key != "" {
		t.Hit(f.key, rect(area.X, area.Y, area.W, 1))
	}
}

// TextArea is wrapped multi-line text in TextField's frame (edges on every row), Rows(min, max)
// tall.
type TextArea struct {
	text, placeholder       string
	width, minRows, maxRows int
	cursor                  bool
}

func NewTextArea(text string) TextArea {
	return TextArea{text: text, width: 60, minRows: 3, maxRows: 6}
}

func (a TextArea) Placeholder(p string) TextArea { a.placeholder = p; return a }
func (a TextArea) Width(w int) TextArea          { a.width = w; return a }
func (a TextArea) Rows(lo, hi int) TextArea      { a.minRows, a.maxRows = lo, max(hi, lo); return a }
func (a TextArea) Cursor(on bool) TextArea       { a.cursor = on; return a }

func (a TextArea) lines() []string { return omarchy.WrapText(a.text, sub(a.width, 4)) }

func (a TextArea) Size(*Theme) (int, int) {
	return a.width, max(a.minRows, min(a.maxRows, len(a.lines())))
}

func (a TextArea) Draw(t *Theme, area Rect, buf *Buffer) {
	l := LookOf(t, a.cursor, false, true, true)
	for dy := range area.H {
		control(t, buf, area.X, area.Y+dy, area.W, l)
	}
	inner := sub(area.W, 4)
	if a.text == "" {
		omarchy.TextFit(t, buf, area.X+2, area.Y, a.placeholder, inner, t.Faint())
	}
	for i, line := range a.lines() {
		if i >= area.H {
			break
		}
		omarchy.TextFit(t, buf, area.X+2, area.Y+i, line, inner, t.Fg())
	}
}

// ---------------------------------------------------------------------------------------------
// NumberField, Dropdown

// NumberField is an integer stepper: an optional label row (textDim), then `−  value  +` in a
// field. Keyed `<key>`, its steppers `<key>-dec` and `<key>-inc`.
type NumberField struct {
	label         string
	value, lo, hi int64
	width         int
	cursor        bool
	key           string
}

func NewNumberField(value int64) NumberField {
	return NumberField{value: value, lo: math.MinInt64, hi: math.MaxInt64, width: 15}
}

func (f NumberField) Label(l string) NumberField     { f.label = l; return f }
func (f NumberField) Range(lo, hi int64) NumberField { f.lo, f.hi = lo, hi; return f }
func (f NumberField) Width(w int) NumberField        { f.width = w; return f }
func (f NumberField) Cursor(on bool) NumberField     { f.cursor = on; return f }
func (f NumberField) Key(k string) NumberField       { f.key = k; return f }

// HandleNumberFieldKey: Up/k/l/+ and Down/j/h/− step within the range. True when handled.
func HandleNumberFieldKey(value *int64, lo, hi int64, msg tea.KeyPressMsg) bool {
	switch keyOf(msg) {
	case "up", "right", "k", "l", "+":
		*value = min(*value+1, hi)
	case "down", "left", "j", "h", "-":
		*value = max(*value-1, lo)
	default:
		return false
	}
	return true
}

func (f NumberField) Size(*Theme) (int, int) {
	if f.label == "" {
		return f.width, 1
	}
	return f.width, 2
}

func (f NumberField) Draw(t *Theme, area Rect, buf *Buffer) {
	y := area.Y
	if f.label != "" {
		omarchy.TextFit(t, buf, area.X, y, f.label, area.W, t.Dim())
		y++
	}
	control(t, buf, area.X, y, area.W, LookOf(t, f.cursor, false, true, true))
	dec, inc := t.Faint(), t.Faint()
	if f.value > f.lo {
		dec = t.Fg()
	}
	if f.value < f.hi {
		inc = t.Fg()
	}
	omarchy.Put(t, buf, area.X+2, y, "−", dec)
	omarchy.Put(t, buf, sub(area.Right(), 3), y, "+", inc)
	omarchy.TextCenter(t, buf, area.X+4, y, sub(area.W, 8), strconv.FormatInt(f.value, 10), t.Fg())
	if f.key != "" {
		t.Hit(f.key, rect(area.X, y, area.W, 1))
		t.Hit(f.key+"-dec", rect(area.X+1, y, 2, 1))
		t.Hit(f.key+"-inc", rect(sub(area.Right(), 3), y, 2, 1))
	}
}

// Dropdown is a labeled single choice: a label row (bold textDim), then the trigger: value,
// chevron at the end. Its list ([DropdownList]) is drawn by the app over the page, under the
// trigger.
type Dropdown struct {
	label, value string
	width        int
	cursor       bool
	key          string
}

func NewDropdown(value string) Dropdown { return Dropdown{value: value, width: 30} }

func (d Dropdown) Label(l string) Dropdown { d.label = l; return d }
func (d Dropdown) Width(w int) Dropdown    { d.width = w; return d }
func (d Dropdown) Cursor(on bool) Dropdown { d.cursor = on; return d }
func (d Dropdown) Key(k string) Dropdown   { d.key = k; return d }

func trigger(t *Theme, buf *Buffer, x, y, w int, hot bool, s string, color Rgba) {
	control(t, buf, x, y, w, LookOf(t, hot, false, true, true))
	omarchy.TextFit(t, buf, x+2, y, s, sub(w, 6), color)
	omarchy.Put(t, buf, x+sub(w, 3), y, GlyphChevronDown, t.Fg().Darker(1.2))
}

func (d Dropdown) Size(*Theme) (int, int) {
	if d.label == "" {
		return d.width, 1
	}
	return d.width, 2
}

func (d Dropdown) Draw(t *Theme, area Rect, buf *Buffer) {
	y := area.Y
	if d.label != "" {
		omarchy.TextFit(t, buf, area.X, y, d.label, area.W, bold(t.Dim(), true))
		y++
	}
	trigger(t, buf, area.X, y, area.W, d.cursor, d.value, t.Fg())
	if d.key != "" {
		t.Hit(d.key, rect(area.X, y, area.W, 1))
	}
}

// DropdownList is a Dropdown's open list on the popups surface: a box in the popups border color,
// one row per option (keyed `<key>-<i>`), the highlighted row in the hover fill, the chosen one
// with an accent check.
type DropdownList struct {
	options             []string
	chosen, highlighted int
	width               int
	key                 string
}

func NewDropdownList(options []string, chosen int) DropdownList {
	return DropdownList{options: options, chosen: chosen, highlighted: None, width: 30}
}

func (d DropdownList) Highlighted(i int) DropdownList { d.highlighted = i; return d }
func (d DropdownList) Width(w int) DropdownList       { d.width = w; return d }
func (d DropdownList) Key(k string) DropdownList      { d.key = k; return d }

func (d DropdownList) Size(*Theme) (int, int) { return d.width, len(d.options) + 2 }

func (d DropdownList) Draw(t *Theme, area Rect, buf *Buffer) {
	omarchy.Clear(buf, area)
	s := t.Shown()
	inner := omarchy.Frame(t, buf, area, s.Surfaces.Popups.Border.Color, "", Ink{})
	for i, o := range d.options {
		y := inner.Y + i
		if y >= inner.Bottom() {
			break
		}
		if d.highlighted == i {
			omarchy.Fill(t, buf, rect(inner.X, y, inner.W, 1), s.Controls.Hover.Fill)
		}
		chosen := d.chosen == i
		omarchy.TextFit(t, buf, inner.X+1, y, o, sub(inner.W, 4), bold(t.Fg(), chosen))
		if chosen {
			omarchy.Put(t, buf, sub(inner.Right(), 2), y, GlyphCheck, bold(t.Accent(), true))
		}
		if d.key != "" {
			t.Hit(d.key+"-"+strconv.Itoa(i), rect(inner.X, y, inner.W, 1))
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Slider

// Slider is a thin track: played `━` in the foreground, the knob `●`, the rest `─` in track;
// notches `┼` on the rest and `╋` on the played part. Edges show with the cursor.
type Slider struct {
	value   float32
	notches int
	width   int
	cursor  bool
	key     string
}

func NewSlider(value float32) Slider { return Slider{value: max(0, min(1, value)), width: 45} }

// Notches is n evenly spaced notches (and steps), ends included.
func (s Slider) Notches(n int) Slider  { s.notches = n; return s }
func (s Slider) Width(w int) Slider    { s.width = w; return s }
func (s Slider) Cursor(on bool) Slider { s.cursor = on; return s }
func (s Slider) Key(k string) Slider   { s.key = k; return s }

// SliderValueAt is the value at a column of a slider drawn at area.
func SliderValueAt(area Rect, col int) float32 {
	track := max(sub(area.W, 2), 2)
	return max(0, min(1, float32(sub(col, area.X+1))/float32(track-1)))
}

// HandleSliderKey: Left/h and Right/l step by a notch (or 5%), Home/End jump. True when handled.
func HandleSliderKey(value *float32, notches int, msg tea.KeyPressMsg) bool {
	step := float32(0.05)
	if notches > 1 {
		step = 1 / float32(notches-1)
	}
	var v float32
	switch keyOf(msg) {
	case "left", "h":
		v = *value - step
	case "right", "l":
		v = *value + step
	case "home":
		v = 0
	case "end":
		v = 1
	default:
		return false
	}
	if notches > 1 {
		v = float32(math.Round(float64(v/step))) * step
	}
	*value = max(0, min(1, v))
	return true
}

// roundf rounds a float32 half away from zero, in float32 (Rust's f32::round).
func roundf(v float32) float32 { return float32(math.Round(float64(v))) }

func (s Slider) Size(*Theme) (int, int) { return s.width, 1 }

func (s Slider) Draw(t *Theme, area Rect, buf *Buffer) {
	var ring *Rgba
	if s.cursor {
		ring = borderOf(t.Shown().Controls.Hover.Border)
	}
	omarchy.Edges(t, buf, area.X, area.Y, area.W, ring)
	w := sub(area.W, 2)
	if w < 2 {
		return
	}
	knob := int(roundf(s.value * float32(w-1)))
	notch := func(i int) bool {
		if s.notches <= 1 {
			return false
		}
		for n := range s.notches {
			if int(roundf(float32(n)/float32(s.notches-1)*float32(w-1))) == i {
				return true
			}
		}
		return false
	}
	for i := range w {
		x := area.X + 1 + i
		if i < knob {
			sym := "━"
			if notch(i) {
				sym = "╋"
			}
			omarchy.Put(t, buf, x, area.Y, sym, t.Fg())
		} else if i > knob {
			sym := "─"
			if notch(i) {
				sym = "┼"
			}
			omarchy.Put(t, buf, x, area.Y, sym, t.Track())
		}
	}
	omarchy.Put(t, buf, area.X+1+knob, area.Y, "●", t.Fg())
	if s.key != "" {
		t.Hit(s.key, area)
	}
}

// ---------------------------------------------------------------------------------------------
// Checkbox, Radio, RadioGroup

// Checkbox is `[✓] Label` (a Radio: `(●) Label`): the box's brackets are its border, its three
// cells its fill; the mark is accent. A description goes on the row under the label.
type Checkbox struct {
	label, description                   string
	checked, exclusive, cursor, disabled bool
	key                                  string
}

func NewCheckbox(label string) Checkbox { return Checkbox{label: label} }

// NewRadio is a Checkbox drawn as a radio: `(●)`.
func NewRadio(label string) Checkbox { return Checkbox{label: label, exclusive: true} }

func (c Checkbox) Description(d string) Checkbox { c.description = d; return c }
func (c Checkbox) Checked(on bool) Checkbox      { c.checked = on; return c }
func (c Checkbox) Exclusive(on bool) Checkbox    { c.exclusive = on; return c }
func (c Checkbox) Cursor(on bool) Checkbox       { c.cursor = on; return c }
func (c Checkbox) Disabled(on bool) Checkbox     { c.disabled = on; return c }
func (c Checkbox) Key(k string) Checkbox         { c.key = k; return c }

func checkBox(t *Theme, buf *Buffer, x, y int, checked, exclusive, hot, disabled bool) {
	c := &t.Shown().Controls
	f, border := c.Normal.Fill, c.Normal.Border.Color
	if hot && !disabled {
		f, border = c.Hover.Fill, c.Hover.Border.Color
	} else if checked {
		f = c.Selected.Fill
	}
	if disabled {
		border, f = border.TimesAlpha(0.5), f.TimesAlpha(0.5)
	}
	omarchy.Fill(t, buf, rect(x, y, 3, 1), f)
	l, r := "[", "]"
	if exclusive {
		l, r = "(", ")"
	}
	omarchy.Put(t, buf, x, y, l, border)
	omarchy.Put(t, buf, x+2, y, r, border)
	if checked {
		mark := GlyphCheck
		if exclusive {
			mark = GlyphDot
		}
		a := t.Accent()
		if disabled {
			a = a.TimesAlpha(0.5)
		}
		omarchy.Put(t, buf, x+1, y, mark, bold(a, true))
	}
}

func (c Checkbox) Size(*Theme) (int, int) {
	w := 4 + max(width(c.label), width(c.description))
	if c.description == "" {
		return w, 1
	}
	return w, 2
}

func (c Checkbox) Draw(t *Theme, area Rect, buf *Buffer) {
	checkBox(t, buf, area.X, area.Y, c.checked, c.exclusive, c.cursor, c.disabled)
	words := sub(area.W, 4)
	fg, dim := t.Fg(), t.Dim()
	if c.disabled {
		fg, dim = t.Faint(), t.Faint()
	}
	omarchy.TextFit(t, buf, area.X+4, area.Y, c.label, words, fg)
	if c.description != "" && area.H > 1 {
		omarchy.TextFit(t, buf, area.X+4, area.Y+1, c.description, words, dim)
	}
	if c.key != "" && !c.disabled {
		t.Hit(c.key, area)
	}
}

// Option is a value and its label (RadioGroup, MultiSelect).
type Option struct{ Value, Label string }

// RadioGroup is radios stacked, one chosen ([HandleRadioGroupKey] moves it); radios are keyed
// `<key>-<value>`.
type RadioGroup struct {
	options []Option
	value   string
	cursor  int
	key     string
}

func NewRadioGroup(options []Option, value string) RadioGroup {
	return RadioGroup{options: options, value: value, cursor: None}
}

func (g RadioGroup) Cursor(i int) RadioGroup { g.cursor = i; return g }
func (g RadioGroup) Key(k string) RadioGroup { g.key = k; return g }

// HandleRadioGroupKey moves value among values (arrows, h/j/k/l), stopping at the ends. True
// when handled.
func HandleRadioGroupKey(value *string, values []string, msg tea.KeyPressMsg) bool {
	i := 0
	for j, v := range values {
		if v == *value {
			i = j
		}
	}
	switch keyOf(msg) {
	case "down", "right", "j", "l":
		i = min(i+1, sub(len(values), 1))
	case "up", "left", "k", "h":
		i = sub(i, 1)
	default:
		return false
	}
	*value = values[i]
	return true
}

func (g RadioGroup) Size(*Theme) (int, int) {
	w := 0
	for _, o := range g.options {
		w = max(w, 4+width(o.Label))
	}
	return w, len(g.options)
}

func (g RadioGroup) Draw(t *Theme, area Rect, buf *Buffer) {
	for i, o := range g.options {
		r := NewRadio(o.Label).Checked(o.Value == g.value).Cursor(g.cursor == i)
		if g.key != "" {
			r = r.Key(g.key + "-" + o.Value)
		}
		r.Draw(t, rect(area.X, area.Y+i, area.W, 1).Intersect(area), buf)
	}
}

// ---------------------------------------------------------------------------------------------
// ColorWell, SwatchStrip

// ColorWell is a bordered well with the color inset, `▏███▕`, and with the cursor a ring outside
// it: 7 columns.
type ColorWell struct {
	color  Rgba
	cursor bool
	key    string
}

func NewColorWell(c Rgba) ColorWell { return ColorWell{color: c} }

func (w ColorWell) Cursor(on bool) ColorWell { w.cursor = on; return w }
func (w ColorWell) Key(k string) ColorWell   { w.key = k; return w }

func (w ColorWell) Size(*Theme) (int, int) { return 7, 1 }

func (w ColorWell) Draw(t *Theme, area Rect, buf *Buffer) {
	c := &t.Shown().Controls
	if w.cursor {
		omarchy.Edges(t, buf, area.X, area.Y, 7, some(c.Hover.Border.Color))
	}
	omarchy.Edges(t, buf, area.X+1, area.Y, 5, borderOf(c.Normal.Border))
	for i := range 3 {
		omarchy.Put(t, buf, area.X+2+i, area.Y, "█", w.color)
	}
	if w.key != "" {
		t.Hit(w.key, area)
	}
}

// SwatchStrip is color chips `██` one column apart; under the chosen one `▔▔` in accent, under
// the cursor's in the hover border color (chosen wins). Two rows; chips keyed `<key>-<i>`.
type SwatchStrip struct {
	colors         []Rgba
	chosen, cursor int
	key            string
}

func NewSwatchStrip(colors []Rgba, chosen int) SwatchStrip {
	return SwatchStrip{colors: colors, chosen: chosen, cursor: None}
}

func (s SwatchStrip) Cursor(i int) SwatchStrip { s.cursor = i; return s }
func (s SwatchStrip) Key(k string) SwatchStrip { s.key = k; return s }

func (s SwatchStrip) Size(*Theme) (int, int) { return sub(len(s.colors)*3, 1), 2 }

func (s SwatchStrip) Draw(t *Theme, area Rect, buf *Buffer) {
	for i, c := range s.colors {
		x := area.X + i*3
		omarchy.Put(t, buf, x, area.Y, "█", c)
		omarchy.Put(t, buf, x+1, area.Y, "█", c)
		var mark *Rgba
		if s.chosen == i {
			mark = some(t.Accent())
		} else if s.cursor == i {
			mark = some(t.Shown().Controls.Hover.Border.Color)
		}
		if mark != nil {
			omarchy.Put(t, buf, x, area.Y+1, "▔", *mark)
			omarchy.Put(t, buf, x+1, area.Y+1, "▔", *mark)
		}
		if s.key != "" {
			t.Hit(s.key+"-"+strconv.Itoa(i), rect(x, area.Y, 2, 2))
		}
	}
}
