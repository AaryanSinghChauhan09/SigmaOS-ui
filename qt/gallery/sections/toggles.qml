import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "toggles"
  title: "Toggles"

  RowLayout {
    spacing: Style.spacing.xxl
    ToggleSwitch { checked: false }
    ToggleSwitch { checked: true }
    ToggleSwitch { checked: false; hasCursor: true }
    ToggleSwitch { checked: true; hasCursor: true }
  }

  Toggle {
    Layout.preferredWidth: 360
    label: "Wi-Fi"
    description: "Connected to Basecamp"
    checked: true
  }
  Toggle {
    Layout.preferredWidth: 360
    label: "Bluetooth"
    description: "Off"
    checked: false
    hasCursor: true
  }
}
