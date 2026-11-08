import QtQuick
import QtQuick.Controls as QQC
import QtTest
import Omarchy.Ui as O

// Checkbox, RadioGroup and MultiSelect, as the Rune gallery's forms_check_and_choose,
// multiselect_toggles_values and choosers_follow_keys_as_in_qt tests drive Rune's.
TestCase {
  id: testCase
  name: "Forms"
  width: 600
  height: 500
  visible: true
  when: windowShown

  Component {
    id: checkComponent
    O.Checkbox { id: box; label: "Show hidden files"; onToggled: box.checked = !box.checked }
  }

  Component {
    id: radiosComponent
    O.RadioGroup {
      value: "bottom"
      options: [{ value: "top", label: "Top" }, { value: "bottom", label: "Bottom" }, { value: "hidden", label: "Hidden" }]
    }
  }

  Component {
    id: multiComponent
    O.MultiSelect {
      width: 260
      label: "Notify for"
      options: ["Calendar", "Mail", "Messages", "Updates", "Battery"]
      values: ["Mail", "Updates"]
    }
  }

  SignalSpy { id: changed; signalName: "changed" }

  function test_checkbox_flips_on_click_and_keys_unless_disabled() {
    var box = createTemporaryObject(checkComponent, testCase)
    mouseClick(box)
    verify(box.checked)
    box.forceActiveFocus()
    keyClick(Qt.Key_Space)
    verify(!box.checked)
    keyClick(Qt.Key_Return)
    verify(box.checked)
    box.enabled = false
    mouseClick(box)
    verify(box.checked, "a disabled checkbox doesn't flip")
  }

  function test_radios_choose_on_click_and_keys() {
    var group = createTemporaryObject(radiosComponent, testCase)
    changed.clear()
    changed.target = group
    var hidden = findAll(group, function(i) { return i.label === "Hidden" })[0]
    mouseClick(hidden)
    compare(group.value, "hidden")
    compare(changed.signalArguments[0][0], "hidden")
    var top = findAll(group, function(i) { return i.label === "Top" })[0]
    mouseClick(top)
    compare(group.value, "top")
    group.forceActiveFocus()
    for (var step of [[Qt.Key_Down, "bottom"], ["j", "hidden"], [Qt.Key_Down, "hidden"], ["k", "bottom"], [Qt.Key_Up, "top"],
                      [Qt.Key_Up, "top"], [Qt.Key_Right, "bottom"], [Qt.Key_Left, "top"]]) {
      keyClick(step[0])
      compare(group.value, step[1], "radios after " + step[0])
    }
  }

  function test_multiselect_toggles_on_click() {
    var ms = createTemporaryObject(multiComponent, testCase)
    mouseClick(trigger(ms))
    searching()
    var row = optionRow("Calendar")
    mouseClick(row)
    verify(ms.values.indexOf("Calendar") >= 0)
    mouseClick(optionRow("Mail"))
    verify(ms.values.indexOf("Mail") < 0)
    compare(ms.values, ["Updates", "Calendar"])
  }

  function test_multiselect_keys() {
    var ms = createTemporaryObject(multiComponent, testCase)
    changed.clear()
    changed.target = ms
    trigger(ms).forceActiveFocus()
    keyClick("j")
    tryVerify(function() { return ms.popupOpen }, 1000, "j opens it from the trigger")
    searching()
    for (var c of "mes") keyClick(c)
    keyClick(Qt.Key_Return)
    compare(ms.values, ["Mail", "Updates", "Messages"], "Return toggles the first match")
    keyClick(Qt.Key_Backspace)
    keyClick(Qt.Key_Backspace)
    keyClick(Qt.Key_Backspace)
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Space)
    compare(ms.values, ["Mail", "Updates", "Messages", "Calendar"], "Down into the list, Space toggles")
    keyClick("j")
    keyClick(Qt.Key_Return)
    compare(ms.values, ["Updates", "Messages", "Calendar"], "j walks, Return toggles")
    compare(changed.count, 3)
    keyClick(Qt.Key_Escape)
    tryVerify(function() { return !ms.popupOpen }, 1000, "Escape closes it")
  }

  function trigger(ms) {
    return findAll(ms, function(i) { return i.activeFocusOnTab === true })[0]
  }

  // The popup is open and placed: its search field has taken the keys (after it moved).
  function searching() {
    tryVerify(function() { return findAll(QQC.Overlay.overlay, function(i) { return i.activeFocus && i.hasOwnProperty("placeholderText") }).length > 0 },
              1000, "the search field takes the keys")
  }

  // The open popup's row labelled `label`, laid out (its row positioner polished, which the
  // offscreen window may not have done yet when the popup opens).
  function optionRow(label) {
    var row = null
    tryVerify(function() {
      row = findAll(QQC.Overlay.overlay, function(i) { return i.text === label && i.visible })[0]
      return !!row
    }, 1000, "the " + label + " row")
    for (var p = row.parent; p; p = p.parent) if (isPolishScheduled(p)) verify(waitForPolish(p))
    return row
  }

  function findAll(item, pred) {
    if (!item) return []
    var out = pred(item) ? [item] : []
    for (var i = 0; i < item.children.length; i++) out = out.concat(findAll(item.children[i], pred))
    return out
  }
}
