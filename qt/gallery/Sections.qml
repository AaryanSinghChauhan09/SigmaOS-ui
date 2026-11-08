import QtQuick
import QtQuick.Layouts
import Omarchy.Theme

// Every section of spec/gallery.md, in order. Each is sections/<name>.qml (a Section), loaded only
// when the gallery shows it, so `--section <name>` renders one alone. Adding a section: write the
// file and add its name here (and to the Rune gallery's `gallery!` list, under the same name).
ColumnLayout {
  id: root
  spacing: Style.spacing.huge * 2

  readonly property var order: [
    "buttons",
    "toggles",
    "inputs",
    "slider",
    "surfaces",
    "lists",
    "tiles",
    "progress",
    "messages",
    "tokens",
  ]

  Repeater {
    model: root.order
    delegate: Loader {
      required property string modelData
      active: gallery.section === "" || gallery.section === modelData
      visible: active
      Layout.fillWidth: true
      source: "sections/" + modelData + ".qml"
    }
  }
}
