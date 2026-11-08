// A pure-QML Omarchy panel: no build, no C++. Needs the installed modules (qt/module, or the
// omarchy-ui Arch package) and runs with the qml tool:
//
//   QT_QUICK_CONTROLS_STYLE=Basic qml examples/qt-installed/main.qml
//
// Switch themes (or `omarchy font set`, or Hyprland's rounding) while it runs: it follows.

import QtQuick
import QtQuick.Layouts
import Omarchy.Theme as Theme
import Omarchy.Ui as O

Window {
  id: root
  width: 400
  height: panel.implicitHeight + 2 * Theme.Style.spacing.panelPadding
  visible: true
  title: "Network"
  color: Theme.Color.background

  property bool wifi: true

  O.Pane {
    id: panel
    anchors.fill: parent
    anchors.margins: Theme.Style.spacing.panelPadding
    bordered: true

    O.Heading { text: "Network"; subtitle: "Wi-Fi and saved networks"; level: "heading"; Layout.fillWidth: true }
    O.Toggle {
      label: "Wi-Fi"
      description: root.wifi ? "On" : "Off"
      checked: root.wifi
      onClicked: root.wifi = !root.wifi
      Layout.fillWidth: true
    }
    O.ListRow {
      glyph: String.fromCodePoint(0xF0928)
      title: "Basecamp"
      status: root.wifi ? "Connected" : "Saved"
      statusTone: root.wifi ? "success" : ""
      value: "5 GHz"
      current: root.wifi
      Layout.fillWidth: true
    }
    O.Button { text: "Forget network"; bordered: true }
  }
}
