import QtQuick
import QtTest
import Omarchy.Ui as O

// Menu, Tabs and IconRail, as the Rune gallery's menu_rows_run, menu_walks_with_vim_keys_and_arrows,
// tabs_and_rail_choose and choosers_follow_keys_as_in_qt tests drive Rune's.
TestCase {
  id: testCase
  name: "Navigation"
  width: 600
  height: 500
  visible: true
  when: windowShown

  // The navigation section's menu, held open: a disabled row and a submenu row among plain ones.
  Component {
    id: menuComponent
    O.Menu {
      id: menu
      held: true
      property var ran: []
      property bool sidebar: true
      O.MenuItem { text: "New"; iconText: ""; onTriggered: menu.ran.push(text) }
      O.MenuItem { text: "Open…"; onTriggered: menu.ran.push(text) }
      O.MenuItem { text: "Save"; onTriggered: menu.ran.push(text) }
      O.MenuItem { text: "Duplicate"; enabled: false; onTriggered: menu.ran.push(text) }
      O.MenuSeparator {}
      O.MenuItem { text: "Sidebar"; checkable: true; checked: menu.sidebar; onTriggered: menu.sidebar = !menu.sidebar }
      O.MenuItem {
        text: "Export"
        submenu: O.Menu {
          O.MenuItem { text: "PDF"; onTriggered: menu.ran.push(text) }
          O.MenuItem { text: "PNG"; onTriggered: menu.ran.push(text) }
        }
      }
    }
  }

  Component {
    id: tabsComponent
    O.Tabs { model: ["General", "Display", "Audio", "Network"]; currentIndex: 1 }
  }

  Component {
    id: railComponent
    O.IconRail {
      model: [
        { icon: "", label: "Files" }, { icon: "", label: "Search" }, { icon: "", label: "Source control" },
        { icon: "", label: "Debug" }, { icon: "", label: "Settings" }
      ]
    }
  }

  SignalSpy { id: activated; signalName: "activated" }

  function menu() {
    var m = createTemporaryObject(menuComponent, testCase)
    verify(m)
    m.forceActiveFocus()
    return m
  }

  function row(m, label) {
    return m.rows.filter(function(r) { return r.text === label })[0]
  }

  function test_menu_rows_run_on_click() {
    var m = menu()
    mouseClick(row(m, "New"))
    compare(m.ran, ["New"])
    mouseClick(row(m, "Sidebar"))
    verify(!m.sidebar, "a check row flips")
    mouseClick(row(m, "Duplicate"))
    compare(m.ran, ["New"], "a disabled row doesn't run")
  }

  function test_menu_walks_with_vim_keys_and_arrows() {
    var m = menu()
    keyClick("j")
    compare(m.currentItem.text, "New", "the first key lands on the first row")
    keyClick("j")
    keyClick(Qt.Key_Return)
    compare(m.ran, ["Open…"])
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Space)
    compare(m.ran, ["Open…", "Save"])
    keyClick("k")
    keyClick(Qt.Key_Enter)
    compare(m.ran, ["Open…", "Save", "Open…"])
    keyClick("j")
    keyClick("j")
    compare(m.currentItem.text, "Sidebar", "skips the disabled row")
    keyClick(Qt.Key_Return)
    verify(!m.sidebar)
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Down)
    compare(m.currentItem.text, "New", "wraps past the last row")
    keyClick(Qt.Key_Up)
    compare(m.currentItem.text, "Export", "and back past the first")
    keyClick(Qt.Key_Home)
    compare(m.currentItem.text, "New")
    keyClick(Qt.Key_End)
    compare(m.currentItem.text, "Export")
  }

  function test_menu_submenu_opens_right_and_closes_left() {
    var m = menu()
    keyClick(Qt.Key_End)
    keyClick("l")
    var sub = row(m, "Export").submenu
    verify(sub.opened, "l opens the submenu")
    verify(sub.activeFocus)
    compare(sub.currentItem.text, "PDF")
    keyClick("j")
    keyClick(Qt.Key_Return)
    compare(m.ran, ["PNG"])
    verify(!sub.opened, "running a row closes the submenu")
    verify(m.activeFocus, "and the held menu has the keys again")
    keyClick(Qt.Key_End)
    keyClick(Qt.Key_Right)
    verify(sub.opened)
    keyClick("h")
    verify(!sub.opened, "h goes back out")
    verify(m.activeFocus)
    keyClick(Qt.Key_Right)
    verify(sub.opened)
    keyClick(Qt.Key_Left)
    verify(!sub.opened, "so does Left")
    keyClick(Qt.Key_Right)
    keyClick(Qt.Key_Escape)
    verify(!sub.opened, "and Escape")
  }

  function test_menu_escape_closes_an_open_menu() {
    var m = menu()
    m.held = false
    m.open()
    verify(m.opened)
    keyClick(Qt.Key_Escape)
    verify(!m.opened)
  }

  function test_tabs_choose_on_click_and_keys() {
    var tabs = createTemporaryObject(tabsComponent, testCase)
    activated.clear()
    activated.target = tabs
    var labels = findAll(tabs, function(i) { return i.text === "Network" })
    mouseClick(labels[0])
    compare(tabs.currentIndex, 3)
    compare(activated.count, 1)
    tabs.forceActiveFocus()
    tabs.currentIndex = 1
    for (var step of [[Qt.Key_Right, 2], ["l", 3], [Qt.Key_Right, 3], ["h", 2], [Qt.Key_Left, 1], [Qt.Key_Home, 0], [Qt.Key_Left, 0],
                      [Qt.Key_End, 3], [Qt.Key_Up, 3], [Qt.Key_Down, 3]]) {
      keyClick(step[0])
      compare(tabs.currentIndex, step[1], "tabs after " + step[0])
    }
  }

  function test_rail_chooses_on_click_and_keys() {
    var rail = createTemporaryObject(railComponent, testCase)
    activated.clear()
    activated.target = rail
    mouseClick(rail, rail.width / 2, rail.height - rail.itemSize / 2 - 4)
    compare(rail.currentIndex, 4, "a press on the last button")
    compare(activated.count, 1)
    rail.forceActiveFocus()
    rail.currentIndex = 0
    for (var step of [[Qt.Key_Down, 1], ["j", 2], ["k", 1], [Qt.Key_Up, 0], [Qt.Key_Up, 0], [Qt.Key_End, 4], [Qt.Key_Down, 4],
                      [Qt.Key_Home, 0], [Qt.Key_Right, 0], [Qt.Key_Left, 0]]) {
      keyClick(step[0])
      compare(rail.currentIndex, step[1], "rail after " + step[0])
    }
  }

  function findAll(item, pred) {
    var out = pred(item) ? [item] : []
    for (var i = 0; i < item.children.length; i++) out = out.concat(findAll(item.children[i], pred))
    return out
  }
}
