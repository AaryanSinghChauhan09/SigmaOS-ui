import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "inputs"
  title: "Inputs"

  RowLayout {
    spacing: Style.spacing.controlGap
    TextField { placeholderText: "Search…"; Layout.preferredWidth: 220 }
    TextField { text: "Omarchy"; Layout.preferredWidth: 220 }
    TextField { text: "Hovered"; hasCursor: true; Layout.preferredWidth: 220 }
  }

  RowLayout {
    spacing: Style.spacing.xxl
    NumberField { label: "Gaps"; value: 10; from: 0; to: 40 }
    Dropdown {
      label: "Theme"
      value: "tokyo-night"
      options: [{ value: "tokyo-night", label: "Tokyo Night" }, { value: "nord", label: "Nord" }]
    }
  }
}
