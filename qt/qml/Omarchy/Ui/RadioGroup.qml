import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A column of Radios, one checked. `options` is a string[] or [{ value, label, description? }],
// as Dropdown takes. Picking one sets `value` and emits `changed(value)`.
//
// Keyboard: Tab focuses the group; Up/Down (j/k) move the choice, as native radio groups do.
// `cursorIndex` forces the hover-cursor look on one row, for panels that drive their own cursor.
Column {
  id: root

  property var options: []
  property string value: ""
  property int cursorIndex: -1
  property color foreground: Commons.Color.foreground
  property color accent: Commons.Color.accent

  signal changed(string value)

  function optionValue(o) { return (o && typeof o === "object") ? String(o.value) : String(o) }
  function optionLabel(o) { return (o && typeof o === "object") ? String(o.label !== undefined ? o.label : o.value) : String(o) }
  function optionDescription(o) { return (o && typeof o === "object" && o.description) ? String(o.description) : "" }
  function indexOfValue(v) {
    for (var i = 0; i < options.length; i++) if (optionValue(options[i]) === v) return i
    return -1
  }
  function choose(index) {
    if (index < 0 || index >= options.length) return
    var v = optionValue(options[index])
    if (v === value) return
    value = v
    changed(v)
  }

  spacing: Style.spacing.lg
  activeFocusOnTab: true

  Keys.onPressed: function(event) {
    var i = indexOfValue(value)
    if (event.key === Qt.Key_Down || event.key === Qt.Key_Right || event.text === "j") {
      choose(Math.min(options.length - 1, i + 1)); event.accepted = true
    } else if (event.key === Qt.Key_Up || event.key === Qt.Key_Left || event.text === "k") {
      choose(Math.max(0, i - 1)); event.accepted = true
    }
  }

  Repeater {
    model: root.options

    Radio {
      required property var modelData
      required property int index
      width: root.width > 0 ? root.width : implicitWidth
      label: root.optionLabel(modelData)
      description: root.optionDescription(modelData)
      checked: root.optionValue(modelData) === root.value
      hasCursor: root.cursorIndex === index || (root.activeFocus && checked)
      foreground: root.foreground
      accent: root.accent
      activeFocusOnTab: false
      onToggled: root.choose(index)
    }
  }
}
