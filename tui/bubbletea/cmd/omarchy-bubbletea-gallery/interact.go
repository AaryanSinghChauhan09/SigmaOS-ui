package main

import (
	"math"
	"slices"
	"strconv"
	"strings"

	"charm.land/bubbles/v2/textinput"
	tea "charm.land/bubbletea/v2"

	o "omarchy-ui/tui/bubbletea/omarchy"
	"omarchy-ui/tui/bubbletea/ui"
)

// What the keys, clicks and the pointer do in the interactive gallery: each section's focusable
// controls (Tab walks them), the keys the focused one takes, and what a click on a keyed part
// does. The behavior tests drive these the way the GUI galleries' tests drive theirs.

// focusables are a section's focusable controls, in Tab order.
func focusables(section string) []string {
	switch section {
	case "buttons":
		return []string{"plain", "bordered", "icon", "power", "align"}
	case "toggles":
		return []string{"switch-0", "switch-1", "wifi"}
	case "inputs":
		return []string{"search", "name", "gaps", "theme"}
	case "slider":
		return []string{"level", "stepped"}
	case "surfaces":
		return []string{"network", "confirm", "add-layer", "night-light"}
	case "lists":
		return []string{"rows"}
	case "tiles":
		return []string{"tile"}
	case "messages":
		return []string{"scan"}
	case "menus":
		return []string{"menu", "context"}
	case "navigation":
		return []string{"tabs", "rail"}
	case "forms":
		return []string{"night", "check-0", "check-1", "check-4", "radio", "password"}
	case "pickers":
		return []string{"notify", "swatch", "discard"}
	case "data":
		return []string{"table", "tree"}
	case "layout":
		return []string{"sidebar", "handle", "cal"}
	case "timeline":
		return []string{"timeline", "scrubber", "drag"}
	case "color":
		return []string{"picker"}
	case "composed":
		return []string{"networks", "searchlist"}
	}
	return nil
}

// typing is true while the focused control takes text (q types instead of quitting).
func (s *State) typing() bool {
	switch s.focus {
	case "search", "name", "password", "searchlist":
		return true
	}
	return false
}

func activate(k tea.KeyPressMsg) bool { return k.String() == "enter" || k.String() == "space" }

func flip(b *bool) bool { *b = !*b; return true }

func toggle(values []string, v string) []string {
	if i := slices.Index(values, v); i >= 0 {
		return slices.Delete(slices.Clone(values), i, i+1)
	}
	return append(slices.Clone(values), v)
}

func (s *State) runMenu(i int) {
	switch i {
	case 4:
		s.sidebarShown = !s.sidebarShown
	case 5:
		s.wordWrap = !s.wordWrap
	default:
		s.ran = menuItems[i]
	}
}

func (s *State) runContext(i int) {
	switch i {
	case 3:
		s.sortByName = true
	case 4:
		s.sortByName = false
	default:
		s.ran = contextItems[i]
	}
}

// edit hands a key to one of the text fields (Bubbles' textinput).
func edit(in *textinput.Model, k tea.KeyPressMsg) bool {
	before, pos := in.Value(), in.Position()
	*in, _ = in.Update(k)
	return in.Value() != before || in.Position() != pos || k.Text != ""
}

// key gives the focused control a key; true when it took it.
func (s *State) key(k tea.KeyPressMsg) bool {
	f := s.focus
	if f == "" {
		return false
	}
	name := k.String()
	switch f {
	case "plain", "bordered", "icon", "power", "add-layer", "scan":
		if activate(k) {
			s.ran = f
			return true
		}
		return false
	case "align":
		return ui.HandleButtonGroupKey(&s.align, 3, k)
	case "switch-0", "switch-1":
		if activate(k) {
			i, _ := strconv.Atoi(f[7:])
			return flip(&s.switches[i])
		}
		return false
	case "wifi", "network", "night-light", "night":
		if !activate(k) {
			return false
		}
		switch f {
		case "wifi":
			return flip(&s.wifi)
		case "network":
			return flip(&s.networkOn)
		case "night-light":
			return flip(&s.nightLight)
		}
		return flip(&s.nightToggle)
	case "search":
		return edit(&s.search, k)
	case "name":
		return edit(&s.name, k)
	case "gaps":
		return ui.HandleNumberFieldKey(&s.gaps, 0, 40, k)
	case "theme":
		if !s.dropdownOpen {
			if activate(k) || name == "down" {
				s.dropdownOpen, s.dropdownRow = true, s.themeChoice
				return true
			}
			return false
		}
		switch name {
		case "down", "j":
			s.dropdownRow = min(s.dropdownRow+1, len(themes)-1)
		case "up", "k":
			s.dropdownRow = max(s.dropdownRow-1, 0)
		case "enter", "space":
			s.themeChoice, s.dropdownOpen = s.dropdownRow, false
		case "esc":
			s.dropdownOpen = false
		default:
			return false
		}
		return true
	case "level":
		return ui.HandleSliderKey(&s.level, 0, k)
	case "stepped":
		return ui.HandleSliderKey(&s.stepped, 5, k)
	case "confirm":
		return ui.HandleDialogKey(&s.confirmSelected, k)
	case "rows":
		switch s.rows.ApplyKey(k) {
		case ui.CursorMoved:
			s.rowsOffset = ui.EnsureVisible(s.rowsOffset, s.rows.Index(), 1, 7)
			return true
		case ui.CursorActivate:
			if s.rows.Index() != 3 {
				s.currentRow = s.rows.Index()
			}
			return true
		case ui.CursorDelete:
			s.ran = "forget"
			return true
		}
		return false
	case "tile":
		i := max(s.tileCursor, 0)
		switch name {
		case "right", "l":
			if i%4 >= 3 {
				return false
			}
			i = min(i+1, 7)
		case "left", "h":
			if i%4 == 0 {
				return false
			}
			i--
		case "down", "j":
			i = min(i+4, 7)
		case "up", "k":
			i = max(i-4, 0)
		default:
			return false
		}
		s.tileCursor = i
		return true
	case "menu":
		if activate(k) {
			s.runMenu(s.menuCursor)
			return true
		}
		return ui.HandleMenuKey(&s.menuCursor, []bool{true, true, true, false, true, true, true}, k)
	case "context":
		if activate(k) {
			s.runContext(s.contextCursor)
			return true
		}
		return ui.HandleMenuKey(&s.contextCursor, []bool{true, true, true, true, true, true}, k)
	case "tabs":
		r := ui.HandleTabsKey(&s.tab, 4, k)
		s.tabCursor = s.tab
		return r
	case "rail":
		return ui.HandleIconRailKey(&s.panel, len(rail), k)
	case "check-0", "check-1", "check-4":
		if activate(k) {
			i, _ := strconv.Atoi(f[6:])
			return flip(&s.checks[i])
		}
		return false
	case "radio":
		return ui.HandleRadioGroupKey(&s.position, []string{"top", "bottom", "hidden"}, k)
	case "password":
		if name == "f2" {
			return flip(&s.revealed)
		}
		return edit(&s.password, k)
	case "notify":
		handled, v := ui.HandleMultiSelectKey(&s.notifyState, notifyOptions, k)
		if v != "" {
			s.notify = toggle(s.notify, v)
		}
		return handled
	case "swatch":
		i := s.swatchCursor
		if i < 0 {
			i = s.swatch
		}
		switch name {
		case "left", "h":
			s.swatchCursor = max(i-1, 0)
		case "right", "l":
			s.swatchCursor = min(i+1, 7)
		case "enter", "space":
			s.swatch = i
		default:
			return false
		}
		return true
	case "discard":
		return ui.HandleDialogKey(&s.discardSelected, k)
	case "table":
		if activate(k) {
			s.tableSelected = s.tableCursor
			return true
		}
		return ui.HandleTableKey(&s.tableCursor, len(tableRows), k)
	case "tree":
		return s.treeKey(k)
	case "sidebar":
		var ids []string
		for _, sec := range sidebarSections() {
			for _, it := range sec.Items {
				ids = append(ids, it.ID)
			}
		}
		i := max(slices.Index(ids, s.sidebarSelected), 0)
		switch name {
		case "down", "j":
			i = min(i+1, len(ids)-1)
		case "up", "k":
			i = max(i-1, 0)
		default:
			return false
		}
		s.sidebarSelected = ids[i]
		return true
	case "handle":
		switch name {
		case "left", "h":
			s.sidebarWidth = max(s.sidebarWidth-1, 15)
		case "right", "l":
			s.sidebarWidth = min(s.sidebarWidth+1, 40)
		default:
			return false
		}
		return true
	case "cal":
		c := s.daySelected
		if s.dayCursor != nil {
			c = *s.dayCursor
		}
		if activate(k) {
			s.daySelected = c
			return true
		}
		y, m := s.month[0], s.month[1]
		if ui.HandleCalendarKey(&c, &y, &m, k) {
			s.dayCursor = &c
			s.month = [2]int{y, m}
			return true
		}
		return false
	case "timeline":
		return ui.HandleTimelineKey(&s.playhead, 30, 90, k)
	case "scrubber":
		return ui.HandleScrubberKey(&s.scrub, 130, k)
	case "drag":
		switch name {
		case "down", "j":
			s.dragCursor = min(s.dragCursor+1, 3)
		case "up", "k":
			s.dragCursor = max(s.dragCursor-1, 0)
		default:
			return false
		}
		return true
	case "picker":
		return ui.HandleColorPickerKey(&s.color, k)
	case "searchlist":
		switch ui.HandleSearchListKey(&s.query, &s.searchCursor, k) {
		case ui.SearchQuery:
			s.searchCursor.SetCount("", len(matching(s.query)))
			return true
		case ui.SearchActivate:
			s.activated = s.searchCursor.Index()
			return true
		case ui.SearchIgnored:
			return false
		}
		return true
	case "networks":
		r := s.wifiRows.ApplyKey(k)
		return r == ui.CursorMoved || r == ui.CursorActivate
	}
	return false
}

func (s *State) treeKey(k tea.KeyPressMsg) bool {
	nodes := treeNodes()
	rows := ui.VisibleTree(nodes, s.treeExpanded)
	i := 0
	for j, r := range rows {
		if r.Node.ID == s.treeCursor {
			i = j
		}
	}
	node := rows[i].Node
	open := slices.Contains(s.treeExpanded, node.ID)
	switch k.String() {
	case "down", "j":
		s.treeCursor = rows[min(i+1, len(rows)-1)].Node.ID
	case "up", "k":
		s.treeCursor = rows[max(i-1, 0)].Node.ID
	case "right", "l":
		if len(node.Children) == 0 || open {
			return false
		}
		s.treeExpanded = append(slices.Clone(s.treeExpanded), node.ID)
	case "left", "h":
		if !open {
			return false
		}
		s.treeExpanded = slices.DeleteFunc(slices.Clone(s.treeExpanded), func(x string) bool { return x == node.ID })
	case "enter", "space":
		s.treeSelected = node.ID
	default:
		return false
	}
	return true
}

// focusOf is the focusable a clicked key belongs to, if any.
func focusOf(section, key string) string {
	base := key
	prefixed := []struct{ prefix, focus string }{
		{"align-", "align"}, {"row-", "rows"}, {"tile-", "tile"}, {"menu-", "menu"}, {"context-", "context"},
		{"tab-", "tabs"}, {"rail-", "rail"}, {"radio-", "radio"}, {"notify", "notify"},
	}
	for _, p := range prefixed {
		if strings.HasPrefix(key, p.prefix) {
			base = p.focus
			goto found
		}
	}
	switch {
	case strings.HasPrefix(key, "swatch-") && section == "pickers":
		base = "swatch"
	case strings.HasPrefix(key, "discard-"):
		base = "discard"
	case strings.HasPrefix(key, "confirm-"):
		base = "confirm"
	case strings.HasPrefix(key, "table-"):
		base = "table"
	case strings.HasPrefix(key, "tree-"):
		base = "tree"
	case strings.HasPrefix(key, "side-"):
		base = "sidebar"
	case strings.HasPrefix(key, "split-handle"):
		base = "handle"
	case strings.HasPrefix(key, "cal-"):
		base = "cal"
	case strings.HasPrefix(key, "clip-") || key == "timeline-ruler":
		base = "timeline"
	case strings.HasPrefix(key, "drag-"):
		base = "drag"
	case strings.HasPrefix(key, "search-"):
		base = "searchlist"
	case strings.HasPrefix(key, "wifi-"):
		base = "networks"
	case strings.HasPrefix(key, "gaps"):
		base = "gaps"
	case strings.HasPrefix(key, "swatch-") || key == "picker-square":
		base = "picker"
	}
found:
	if slices.Contains(focusables(section), base) {
		return base
	}
	return ""
}

// click is a click on a keyed part, at a column; rect is where that part was drawn. True when it
// did something.
func (s *State) click(t *o.Theme, section, key string, col int, rect o.Rect) bool {
	n := func(prefix string) int {
		v, err := strconv.Atoi(strings.TrimPrefix(key, prefix))
		if err != nil {
			return 0
		}
		return v
	}
	has := func(p string) bool { return strings.HasPrefix(key, p) }
	switch {
	case key == "plain" || key == "bordered" || key == "icon" || key == "power" || key == "add-layer":
		s.ran = "button"
	case key == "empty-action":
		s.ran = "scan"
	case key == "wifi" && section == "toggles":
		s.wifi = !s.wifi
	case key == "bluetooth":
		s.bluetooth = !s.bluetooth
	case key == "network":
		s.networkOn = !s.networkOn
	case key == "night-light":
		s.nightLight = !s.nightLight
	case key == "night":
		s.nightToggle = !s.nightToggle
	case key == "gaps-dec":
		s.gaps = max(s.gaps-1, 0)
	case key == "gaps-inc":
		s.gaps = min(s.gaps+1, 40)
	case key == "theme":
		s.dropdownOpen = !s.dropdownOpen
		s.dropdownRow = s.themeChoice
	case key == "level":
		s.level = ui.SliderValueAt(rect, col)
	case key == "stepped":
		s.stepped = float32(math.Round(float64(ui.SliderValueAt(rect, col)*4))) / 4
	case key == "confirm-cancel":
		s.confirmSelected = 0
	case key == "confirm-accept":
		s.confirmSelected = 1
	case key == "discard-cancel":
		s.discardSelected = 0
	case key == "discard-accept":
		s.discardSelected = 1
	case key == "password" || key == "modules" || key == "picker-square":
	case key == "notify":
		s.notifyState.Open = !s.notifyState.Open
	case key == "timeline-ruler":
		s.playhead = ui.TimelineTimeAt(col-rect.X, 30, rect.W)
	case key == "scrubber":
		s.scrub = math.Max(0, math.Min(130, float64(max(col-rect.X, 0))/float64(max(rect.W-1, 1))*130))
	case key == "cal-prev" || key == "cal-next":
		d := 1
		if key == "cal-prev" {
			d = -1
		}
		m := ui.NewDate(s.month[0], s.month[1], 1).AddMonths(d)
		s.month = [2]int{m.Year, m.Month}
	case has("theme-list-"):
		s.themeChoice, s.dropdownOpen = n("theme-list-"), false
	case has("align-"):
		s.align = n("align-")
	case has("switch-"):
		i := n("switch-")
		s.switches[i] = !s.switches[i]
	case has("row-") && strings.HasSuffix(key, "-action"):
		s.ran = "forget"
	case has("row-"):
		i := n("row-")
		s.rows.Set("", i)
		s.currentRow = i
	case has("tile-"):
		s.tileCursor = n("tile-")
	case has("menu-"):
		if i := slices.Index(menuKeys, key); i >= 0 {
			s.menuCursor = i
			s.runMenu(i)
		}
	case has("context-"):
		if i := slices.Index(contextKeys, key); i >= 0 {
			s.contextCursor = i
			s.runContext(i)
		}
	case has("tab-"):
		s.tab = n("tab-")
		s.tabCursor = s.tab
	case has("rail-"):
		s.panel = n("rail-")
	case has("check-"):
		if i := n("check-"); i != 3 {
			s.checks[i] = !s.checks[i]
		}
	case has("radio-"):
		s.position = strings.TrimPrefix(key, "radio-")
	case has("notify-"):
		s.notify = toggle(s.notify, strings.TrimPrefix(key, "notify-"))
	case has("modules-"):
		s.modules = toggle(s.modules, strings.TrimPrefix(key, "modules-"))
	case has("swatch-") && section == "pickers":
		s.swatch = n("swatch-")
		s.swatchCursor = s.swatch
	case has("swatch-"):
		// The color picker's palette: pick the swatch, keep the alpha.
		colors := ui.PaletteColors(t, []string{"red", "orange", "yellow", "green", "cyan", "blue", "magenta", "foreground"})
		if i := n("swatch-"); i < len(colors) {
			if h, ok := ui.HsvaFromHex(colors[i].Hex()[:7], s.color.H); ok {
				h.A = s.color.A
				s.color = h
			}
		}
	case has("table-col-"):
	case has("table-"):
		r := n("table-")
		s.tableCursor, s.tableSelected = r, r
	case has("tree-") && strings.HasSuffix(key, "-toggle"):
		id := strings.TrimSuffix(strings.TrimPrefix(key, "tree-"), "-toggle")
		if slices.Contains(s.treeExpanded, id) {
			s.treeExpanded = slices.DeleteFunc(slices.Clone(s.treeExpanded), func(x string) bool { return x == id })
		} else {
			s.treeExpanded = append(slices.Clone(s.treeExpanded), id)
		}
	case has("tree-"):
		s.treeSelected = strings.TrimPrefix(key, "tree-")
		s.treeCursor = s.treeSelected
	case has("side-"):
		s.sidebarSelected = strings.TrimPrefix(key, "side-")
	case has("cal-"):
		p := strings.Split(strings.TrimPrefix(key, "cal-"), "-")
		if len(p) == 3 {
			y, _ := strconv.Atoi(p[0])
			m, _ := strconv.Atoi(p[1])
			d, _ := strconv.Atoi(p[2])
			s.daySelected = ui.NewDate(y, m, d)
			s.month = [2]int{y, m}
		}
	case has("clip-"):
		p := strings.Split(strings.TrimPrefix(key, "clip-"), "-")
		if len(p) == 2 {
			a, _ := strconv.Atoi(p[0])
			b, _ := strconv.Atoi(p[1])
			s.clip = &ui.ClipRef{Track: a, Clip: b}
		}
	case has("drag-"):
		s.dragCursor = n("drag-")
	case has("search-row-"):
		s.activated = n("search-row-")
	case has("wifi-"):
		s.wifiRows.Set("", n("wifi-"))
	default:
		return false
	}
	return true
}

// hover is the pointer over a keyed part: list cursors follow it (through CursorModel's gate).
func (s *State) hover(key string, col, row int) bool {
	n := func(prefix string) (int, bool) {
		if !strings.HasPrefix(key, prefix) {
			return 0, false
		}
		v, err := strconv.Atoi(strings.TrimPrefix(key, prefix))
		return v, err == nil
	}
	if i, ok := n("row-"); ok && i != 3 {
		return s.rows.PointerMoved(col, row, "", i)
	}
	if i, ok := n("wifi-"); ok {
		return s.wifiRows.PointerMoved(col, row, "", i)
	}
	if i, ok := n("search-row-"); ok {
		return s.searchCursor.PointerMoved(col, row, "", i)
	}
	if i, ok := n("tile-"); ok {
		changed := s.tileCursor != i
		s.tileCursor = i
		return changed
	}
	return false
}
