package main

import (
	"path/filepath"
	"runtime"

	"charm.land/bubbles/v2/textinput"

	"omarchy-ui/tui/bubbletea/ui"
)

// State is everything a section's specimens show that the keys and the pointer can change. The
// initial values are the specimens spec/terminal.md (and spec/gallery.md) list.
type State struct {
	// focus is the interactive window's focused control in the current section ("" in
	// snapshots).
	focus   string
	picture *ui.AvatarImage

	// buttons
	align int
	ran   string
	// toggles
	switches        [4]bool
	wifi, bluetooth bool
	// inputs: Bubbles' textinput holds the text and the caret; the kit draws the field.
	search, name textinput.Model
	gaps         int64
	themeChoice  int
	dropdownOpen bool
	dropdownRow  int
	// slider
	level, stepped float32
	// surfaces
	networkOn, nightLight bool
	confirmSelected       int
	// lists
	rows       ui.CursorModel
	currentRow int
	rowsOffset int
	tileCursor int
	// menus
	menuCursor, contextCursor int
	sidebarShown, wordWrap    bool
	sortByName                bool
	// navigation
	tab, tabCursor, panel int
	// forms
	checks      [5]bool
	position    string
	password    textinput.Model
	revealed    bool
	nightToggle bool
	// pickers
	modules, notify []string
	notifyState     ui.MultiSelectState
	swatch          int
	swatchCursor    int
	discardSelected int
	// data
	tableSelected, tableCursor int
	treeExpanded               []string
	treeSelected, treeCursor   string
	// layout
	sidebarSelected string
	sidebarWidth    int
	month           [2]int
	daySelected     ui.Date
	dayCursor       *ui.Date
	// timeline
	playhead   float64
	clip       *ui.ClipRef
	scrub      float64
	dragCursor int
	// color
	color ui.Hsva
	// composed
	query        string
	searchCursor ui.CursorModel
	activated    int
	wifiRows     ui.CursorModel
	inspectorTab int
}

// picturePath is the sample picture (the same 96×96 file as the Qt and Rune galleries'), from
// the checkout this was built in.
func picturePath() string {
	_, file, _, _ := runtime.Caller(0)
	return filepath.Join(filepath.Dir(file), "..", "..", "..", "..", "qt", "gallery", "assets", "avatar.png")
}

func input(value string) textinput.Model {
	in := textinput.New()
	in.SetValue(value)
	in.Focus()
	return in
}

func initState() State {
	rows := ui.NewCursorList(9)
	rows.Set("", 1)
	searchCursor := ui.NewCursorList(4)
	searchCursor.Set("", 1)
	wifiRows := ui.NewCursorList(4)
	wifiRows.Set("", 1)
	color, _ := ui.HsvaFromHex("#7aa2f7", 0)
	return State{
		picture:         ui.LoadAvatarImage(picturePath()),
		align:           1,
		switches:        [4]bool{false, true, false, true},
		wifi:            true,
		search:          input(""),
		name:            input("Omarchy"),
		gaps:            10,
		level:           0.35,
		stepped:         0.75,
		networkOn:       true,
		nightLight:      true,
		confirmSelected: 1,
		rows:            rows,
		currentRow:      2,
		tileCursor:      2,
		menuCursor:      1,
		sidebarShown:    true,
		sortByName:      true,
		tab:             1,
		tabCursor:       2,
		panel:           1,
		checks:          [5]bool{false, true, false, false, true},
		position:        "bottom",
		password:        input("hunter22"),
		nightToggle:     true,
		modules:         []string{"clock", "battery", "network", "volume"},
		notify:          []string{"mail", "updates"},
		notifyState:     ui.MultiSelectState{Open: true, Row: ui.None},
		swatch:          2,
		swatchCursor:    ui.None,
		discardSelected: 1,
		tableSelected:   1,
		tableCursor:     3,
		treeExpanded:    []string{"root", "qt", "spec"},
		treeSelected:    "qt/table",
		treeCursor:      "readme",
		sidebarSelected: "recent",
		sidebarWidth:    25,
		month:           [2]int{2026, 11},
		daySelected:     ui.NewDate(2026, 11, 14),
		playhead:        12,
		clip:            &ui.ClipRef{Track: 0, Clip: 1},
		scrub:           42,
		dragCursor:      1,
		color:           color,
		query:           "term",
		searchCursor:    searchCursor,
		activated:       ui.None,
		wifiRows:        wifiRows,
	}
}

var themes = []string{"Tokyo Night", "Catppuccin", "Gruvbox", "Nord", "Rose Pine"}

var modules = []ui.Option{{Value: "clock", Label: "Clock"}, {Value: "battery", Label: "Battery"}, {Value: "network", Label: "Network"}, {Value: "volume", Label: "Volume"}, {Value: "bluetooth", Label: "Bluetooth"}}

var notifyOptions = []ui.Option{{Value: "calendar", Label: "Calendar"}, {Value: "mail", Label: "Mail"}, {Value: "messages", Label: "Messages"}, {Value: "updates", Label: "Updates"}, {Value: "battery", Label: "Battery"}}
