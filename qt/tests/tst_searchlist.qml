import QtQuick
import QtTest
import Omarchy.Theme
import Omarchy.Ui as O

// SearchList's keys, as its header documents them and the Rune gallery's
// search_list_filters_moves_and_activates test drives them: typing filters (j, k and Space are
// text), Up/Down and the paging keys move the cursor, Return activates the cursor row, Delete
// asks to delete it, Escape clears the query and then asks to close.
TestCase {
  id: testCase
  name: "SearchList"
  width: 600
  height: 500
  visible: true
  when: windowShown

  // The composed section's four results; the caller filters, as SearchList asks.
  readonly property var all: [
    { title: "Terminal", subtitle: "App", glyph: "", detail: "Open a new terminal window." },
    { title: "Terminal font", subtitle: "Style", glyph: "", detail: "Pick the monospace font every terminal uses." },
    { title: "Terminal theme", subtitle: "Style", glyph: "", detail: "Match the terminal colors to the desktop theme." },
    { title: "Intermediate CA", subtitle: "Certificate", glyph: "", detail: "Trust a certificate authority for this machine." }
  ]

  Component {
    id: listComponent
    O.SearchList {
      id: list
      width: 360
      height: 300
      showDetail: true
      items: testCase.all.filter(function(i) { return list.matches(i.title, list.query) })
    }
  }

  SignalSpy { id: activated; signalName: "activated" }
  SignalSpy { id: deleted; signalName: "deleteRequested" }
  SignalSpy { id: closed; signalName: "closeRequested" }

  function make(query) {
    var list = createTemporaryObject(listComponent, testCase, { query: query || "" })
    verify(list)
    for (var spy of [activated, deleted, closed]) {
      spy.clear()
      spy.target = list
    }
    list.forceActiveFocus()
    verify(list.filterInput.activeFocus, "the filter line holds focus")
    return list
  }

  function type(text) {
    for (var c of text) keyClick(c)
  }

  function titles(list) {
    return list.items.map(function(i) { return i.title })
  }

  function test_typing_filters() {
    var list = make()
    compare(list.items.length, 4)
    type("font")
    compare(list.query, "font")
    compare(titles(list), ["Terminal font"])
    keyClick(Qt.Key_Backspace)
    compare(list.query, "fon")
  }

  function test_vim_keys_and_space_are_text() {
    var list = make()
    type("jk")
    compare(list.query, "jk", "j and k type, they don't move")
    compare(list.cursorIndex, -1, "nothing matches jk")
    keyClick(Qt.Key_Backspace)
    keyClick(Qt.Key_Backspace)
    type("term")
    keyClick(Qt.Key_Space)
    compare(list.query, "term ")
    compare(list.items.length, 0)
    keyClick(Qt.Key_Backspace)
    type("x")
    compare(list.query, "termx", "x is text here, not delete")
    compare(deleted.count, 0)
  }

  function test_arrows_move_the_cursor() {
    var list = make("term")
    compare(list.cursorIndex, 0, "the cursor shows on the first match from the start")
    keyClick(Qt.Key_Down)
    compare(list.cursorIndex, 1)
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Down)
    compare(list.cursorIndex, 3)
    keyClick(Qt.Key_Down)
    compare(list.cursorIndex, 3, "it stops at the last row")
    keyClick(Qt.Key_Up)
    compare(list.cursorIndex, 2)
    keyClick(Qt.Key_Home)
    compare(list.cursorIndex, 0)
    keyClick(Qt.Key_End)
    compare(list.cursorIndex, 3)
    keyClick(Qt.Key_PageUp)
    compare(list.cursorIndex, 0)
    keyClick(Qt.Key_PageDown)
    compare(list.cursorIndex, 3)
    compare(list.currentItem.title, "Intermediate CA", "the detail pane follows the cursor")
  }

  function test_enter_activates_the_cursor_row() {
    var list = make("term")
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Return)
    compare(activated.count, 1)
    compare(activated.signalArguments[0][0], 2)
    compare(list.items[2].title, "Terminal theme")
    keyClick(Qt.Key_Enter)
    compare(activated.count, 2, "keypad Enter too")
  }

  function test_new_query_starts_at_first_match() {
    var list = make("term")
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Down)
    compare(list.cursorIndex, 2)
    type("i")
    compare(list.query, "termi")
    compare(list.cursorIndex, 0)
  }

  function test_delete_asks_to_delete_the_cursor_row() {
    var list = make("term")
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Delete)
    compare(deleted.count, 1)
    compare(deleted.signalArguments[0][0], 1)
  }

  function test_escape_clears_then_closes() {
    var list = make("termix")
    compare(list.items.length, 0)
    keyClick(Qt.Key_Escape)
    compare(list.query, "")
    compare(list.items.length, 4, "an empty query lists everything")
    compare(closed.count, 0)
    keyClick(Qt.Key_Escape)
    compare(closed.count, 1)
  }

  function test_nothing_matches_shows_the_empty_state() {
    var list = make("zzz")
    compare(list.items.length, 0)
    compare(list.cursorIndex, -1)
    keyClick(Qt.Key_Return)
    compare(activated.count, 0, "Return with no rows does nothing")
    var expected = I18n.t("common.noMatchesFor", "").arg("zzz")
    verify(expected.indexOf("zzz") >= 0, "the catalog has a no-matches line with the query")
    verify(findText(list, expected), "the empty state reads " + expected)
  }

  function test_click_activates_the_row() {
    var list = make("term")
    var row = findText(list, "Terminal font")
    verify(row)
    mouseClick(row)
    compare(activated.count, 1)
    compare(activated.signalArguments[0][0], 1)
    compare(list.cursorIndex, 1)
  }

  // The visible Text item showing exactly `text` under `item`, or null.
  function findText(item, text) {
    if (item.visible === false) return null
    if (item.text === text && item.hasOwnProperty("textFormat")) return item
    for (var i = 0; i < item.children.length; i++) {
      var hit = findText(item.children[i], text)
      if (hit) return hit
    }
    return null
  }
}
