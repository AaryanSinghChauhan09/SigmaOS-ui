import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "progress"
  title: "Progress"

  RowLayout {
    spacing: Style.spacing.huge
    Meter { value: 0.2; markers: [0.5, 0.8]; Layout.preferredWidth: 220 }
    Meter { value: 0.65; markers: [0.5, 0.8]; Layout.preferredWidth: 220 }
    Meter { value: 0.92; alarming: true; markers: [0.5, 0.8]; Layout.preferredWidth: 220 }
  }

  Meter { value: 0.65; label: "Battery"; valueText: "65%"; Layout.preferredWidth: 360 }

  RowLayout {
    spacing: Style.spacing.huge
    ProgressBar { value: 0.4; Layout.preferredWidth: 340 }
    ProgressBar { indeterminate: true; Layout.preferredWidth: 340 }
  }

  RowLayout {
    spacing: Style.spacing.huge
    Spinner {}
    Spinner { size: Style.font.iconLarge }
    Spinner { pulse: true }
    Spinner { pulse: true; size: Style.font.iconLarge }
  }
}
