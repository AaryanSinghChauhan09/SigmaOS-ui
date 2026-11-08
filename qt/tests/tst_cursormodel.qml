import QtQuick
import QtTest
import Omarchy.Ui as O

// CursorModel's keys, case for case with the Rune CursorModel's unit tests
// (rune/omarchy-rune/src/ui/lists/cursor.rs): the first key only shows the cursor, rows cross
// sections and skip empty ones, pages stay in their section, grids move by columns, and the
// action keys and type-ahead report themselves.
TestCase {
  id: testCase
  name: "CursorModel"
  width: 200
  height: 200
  visible: true
  when: windowShown

  // A focused item that hands every key to `model`, as a panel's Keys.onPressed does.
  Component {
    id: panelComponent
    FocusScope {
      property O.CursorModel model: O.CursorModel {
        sections: [{ name: "header", count: 1 }, { name: "connected", count: 2 }, { name: "known", count: 0 }, { name: "discovered", count: 3 }]
      }
      width: 100
      height: 100
      focus: true
      Keys.onPressed: function(event) { model.handleKey(event) }
    }
  }

  SignalSpy { id: activated; signalName: "activated" }
  SignalSpy { id: deleted; signalName: "deleteRequested" }
  SignalSpy { id: closed; signalName: "closeRequested" }
  SignalSpy { id: typed; signalName: "textTyped" }

  function make(props) {
    var panel = createTemporaryObject(panelComponent, testCase)
    verify(panel)
    for (var k in props || {}) panel.model[k] = props[k]
    for (var spy of [activated, deleted, closed, typed]) {
      spy.clear()
      spy.target = panel.model
    }
    panel.forceActiveFocus()
    return panel.model
  }

  function at(c) { return [c.focusSection, c.selectedIndex] }

  function test_first_key_only_shows_the_cursor() {
    var c = make()
    verify(!c.isAt("header", 0))
    keyClick("j")
    verify(c.isAt("header", 0))
    keyClick("j")
    verify(c.isAt("connected", 0))
  }

  function test_rows_cross_sections_and_skip_empty_ones() {
    var c = make()
    c.set("connected", 1)
    keyClick(Qt.Key_Down)
    verify(c.isAt("discovered", 0), "skips the empty known section")
    keyClick("k")
    verify(c.isAt("connected", 1), "back to the previous section's last row")
    keyClick(Qt.Key_End)
    verify(c.isAt("discovered", 2))
    keyClick(Qt.Key_Down)
    verify(c.isAt("discovered", 2), "stops at the last row")
    keyClick(Qt.Key_Home)
    verify(c.isAt("header", 0))
    keyClick(Qt.Key_Up)
    verify(c.isAt("header", 0), "stops at the first row")
  }

  function test_tab_jumps_sections() {
    var c = make()
    c.set("header", 0)
    keyClick(Qt.Key_Tab)
    verify(c.isAt("connected", 0))
    keyClick(Qt.Key_Tab)
    verify(c.isAt("discovered", 0), "skips the empty section")
    keyClick(Qt.Key_Backtab)
    verify(c.isAt("connected", 0))
  }

  function test_pages_stay_in_their_section() {
    var c = make({ sections: [{ name: "a", count: 12 }, { name: "b", count: 4 }] })
    c.set("a", 0)
    keyClick(Qt.Key_PageDown)
    compare(at(c), ["a", 5])
    keyClick(Qt.Key_PageDown)
    keyClick(Qt.Key_PageDown)
    compare(at(c), ["a", 11])
    keyClick(Qt.Key_PageUp)
    compare(c.selectedIndex, 6)
  }

  function test_grids_move_by_columns() {
    var c = make({ sections: [{ name: "", count: 8 }], columns: 4 })
    c.set("", 1)
    keyClick("l")
    compare(c.selectedIndex, 2)
    keyClick(Qt.Key_Down)
    compare(c.selectedIndex, 6)
    keyClick(Qt.Key_Right)
    keyClick(Qt.Key_Right)
    compare(c.selectedIndex, 7, "stays on its line")
    keyClick("k")
    compare(c.selectedIndex, 3)
    keyClick("h")
    compare(c.selectedIndex, 2)

    var l = make({ sections: [{ name: "", count: 3 }] })
    l.set("", 1)
    keyClick(Qt.Key_Right)
    compare(l.selectedIndex, 1, "a list ignores column moves")
  }

  function test_short_last_line() {
    var c = make({ sections: [{ name: "", count: 6 }], columns: 4 })
    c.set("", 3)
    keyClick(Qt.Key_Down)
    compare(c.selectedIndex, 5, "lands on the short line's last tile")
  }

  function test_actions_and_type_ahead() {
    var c = make({ sections: [{ name: "", count: 3 }] })
    c.set("", 1)
    keyClick(Qt.Key_Return)
    keyClick(Qt.Key_Space)
    compare(activated.count, 2)
    compare(activated.signalArguments[0][1], 1)
    keyClick(Qt.Key_Escape)
    compare(closed.count, 1)
    keyClick("x")
    keyClick(Qt.Key_Delete)
    compare(deleted.count, 2)
    // (QtTest only types ASCII; the non-ASCII case is Rune's alone.)
    keyClick("q")
    keyClick("7")
    compare(typed.count, 2)
    compare(typed.signalArguments[0][0], "q")
    compare(typed.signalArguments[1][0], "7")
    keyClick("q", Qt.ControlModifier)
    compare(typed.count, 2, "Ctrl+Q is not text")
  }
}
