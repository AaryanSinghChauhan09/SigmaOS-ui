import QtQuick
import QtQuick.Layouts
import QtTest
import Omarchy.Theme
import Omarchy.Ui as O

// A Pane's header actions and the switches, as the Rune gallery's pane_actions_run and
// toggles_toggle tests drive Rune's: the action ends the header at the pane's right edge and
// runs, and a switch asks to flip on a press and on Space.
TestCase {
  id: testCase
  name: "Surfaces"
  width: 600
  height: 400
  visible: true
  when: windowShown

  Component {
    id: paneComponent
    O.Pane {
      id: pane
      property bool nightLight: true
      width: 240
      title: "Night light"
      subtitle: "Sunset to sunrise"
      bordered: true
      actions: [ O.ToggleSwitch { objectName: "night-light"; checked: pane.nightLight; onToggled: pane.nightLight = !pane.nightLight } ]
      O.SettingRow {
        Layout.fillWidth: true
        label: "Strength"
        O.ValueSlider { width: 100; value: 0.6 }
      }
    }
  }

  Component {
    id: toggleComponent
    O.Toggle { id: toggle; label: "Wi-Fi"; description: "Connected"; onClicked: toggle.checked = !toggle.checked }
  }

  function test_pane_action_ends_the_header_and_runs() {
    var pane = createTemporaryObject(paneComponent, testCase)
    var sw = findObject(pane, "night-light")
    verify(sw)
    var right = sw.mapToItem(pane, sw.width, 0).x
    var inner = pane.width - pane.contentPadding
    verify(right <= inner + 0.5 && right > inner - 40, "the action ends the header, not the title (" + right + " vs " + inner + ")")
    var title = findAll(pane, function(i) { return i.text === "Night light" })[0]
    verify(title.mapToItem(pane, 0, 0).x < sw.mapToItem(pane, 0, 0).x, "the title comes first")
    verify(pane.nightLight)
    mouseClick(sw)
    verify(!pane.nightLight)
  }

  function test_toggle_row_flips_on_click_and_space() {
    var toggle = createTemporaryObject(toggleComponent, testCase)
    mouseClick(toggle)
    verify(toggle.checked, "the whole row is the control")
    toggle.forceActiveFocus()
    keyClick(Qt.Key_Space)
    verify(!toggle.checked)
  }

  function findObject(item, name) {
    return findAll(item, function(i) { return i.objectName === name })[0]
  }

  function findAll(item, pred) {
    var out = pred(item) ? [item] : []
    for (var i = 0; i < item.children.length; i++) out = out.concat(findAll(item.children[i], pred))
    return out
  }
}
