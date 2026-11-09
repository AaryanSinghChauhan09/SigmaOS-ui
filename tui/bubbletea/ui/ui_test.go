package ui

import (
	"slices"
	"testing"

	tea "charm.land/bubbletea/v2"
)

func TestKeyLabelsAndCombos(t *testing.T) {
	if got := SplitCombo("Ctrl+Shift+P"); !slices.Equal(got, []string{"Ctrl", "Shift", "P"}) {
		t.Errorf("%v", got)
	}
	if got := SplitCombo("Ctrl++"); !slices.Equal(got, []string{"Ctrl", "+"}) {
		t.Errorf("the plus key: %v", got)
	}
	for in, want := range map[string]string{"enter": "⏎", "shift": "⇧", "k": "K", "PageUp": "PgUp", "F11": "F11"} {
		if KeyLabel(in) != want {
			t.Errorf("%s: %s", in, KeyLabel(in))
		}
	}
}

func TestDatesWalkTheCalendar(t *testing.T) {
	if NewDate(2026, 11, 8).Weekday() != 6 {
		t.Error("2026-11-08 is a Sunday")
	}
	if FirstCell(2026, 11) != NewDate(2026, 10, 26) {
		t.Error("the grid starts on the Monday on or before the 1st")
	}
	if NewDate(2026, 1, 31).AddMonths(1) != NewDate(2026, 2, 28) || NewDate(2026, 12, 31).AddDays(1) != NewDate(2027, 1, 1) {
		t.Error("date arithmetic")
	}
}

func TestCursorModelKeysAndGate(t *testing.T) {
	m := NewCursorModel(CursorSection{"a", 2}, CursorSection{"b", 0}, CursorSection{"c", 3})
	if m.ApplyKey(tea.KeyPressMsg{Code: 'j', Text: "j"}) != CursorMoved || m.Active != true || m.Index() != 0 {
		t.Fatal("the first key only wakes the cursor")
	}
	m.ApplyKey(tea.KeyPressMsg{Code: tea.KeyDown})
	m.ApplyKey(tea.KeyPressMsg{Code: tea.KeyDown})
	if m.Section() != "c" || m.Index() != 0 {
		t.Errorf("down past a section skips the empty one: %s %d", m.Section(), m.Index())
	}
	if m.ApplyKey(tea.KeyPressMsg{Code: tea.KeySpace, Text: " "}) != CursorActivate || m.ApplyKey(tea.KeyPressMsg{Code: 'x', Text: "x"}) != CursorDelete {
		t.Error("space activates, x deletes")
	}
	if m.ApplyKey(tea.KeyPressMsg{Code: 'q', Text: "q"}) != CursorText {
		t.Error("printable keys are text")
	}
	if m.PointerMoved(3, 3, "a", 1) || !m.PointerMoved(4, 3, "a", 1) || m.Section() != "a" {
		t.Error("the pointer's first sample primes the gate")
	}
}

func TestColorsRoundTrip(t *testing.T) {
	c, ok := HsvaFromHex("#7aa2f7", 0)
	if !ok || c.Hex() != "#7aa2f7" {
		t.Errorf("%v %s", ok, c.Hex())
	}
	c.A = 0.5
	if c.Hex() != "#7aa2f780" {
		t.Error(c.Hex())
	}
}

func TestPrintableExcludesSpaceAndModifiers(t *testing.T) {
	if Printable(tea.KeyPressMsg{Code: tea.KeySpace, Text: " "}) != "" || Printable(tea.KeyPressMsg{Code: 'a', Text: "a", Mod: tea.ModCtrl}) != "" || Printable(tea.KeyPressMsg{Code: 'a', Text: "a"}) != "a" {
		t.Error("printable")
	}
}
