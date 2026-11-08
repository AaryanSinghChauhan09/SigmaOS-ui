import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "forms"
  title: "Forms"

  Column {
    Layout.preferredWidth: 480

    SettingRow {
      width: parent.width
      label: "Night light"
      description: "Warm the screen after sunset"
      ToggleSwitch { checked: true }
    }
    SettingRow {
      width: parent.width
      label: "Theme"
      description: "Applies to every app"
      Dropdown {
        width: 180
        showLabel: false
        value: "tokyo-night"
        options: [{ value: "tokyo-night", label: "Tokyo Night" }, { value: "nord", label: "Nord" }]
      }
    }
    SettingRow {
      width: parent.width
      label: "Gaps"
      description: "Space between windows"
      NumberField { value: 10; from: 0; to: 40 }
    }
  }

  RowLayout {
    spacing: Style.spacing.huge * 2

    Column {
      Layout.alignment: Qt.AlignTop
      spacing: Style.spacing.lg
      Checkbox { label: "Show seconds" }
      Checkbox { label: "Show date"; checked: true }
      Checkbox { label: "Show weekday"; hasCursor: true }
      Checkbox { label: "Show week number"; enabled: false }
      Checkbox { label: "Twelve-hour clock"; description: "Show AM and PM"; checked: true }
    }

    RadioGroup {
      Layout.alignment: Qt.AlignTop
      value: "bottom"
      options: [{ value: "top", label: "Top" }, { value: "bottom", label: "Bottom" }, { value: "hidden", label: "Hidden" }]
    }
  }

  RowLayout {
    spacing: Style.spacing.controlGap

    PasswordField {
      Layout.preferredWidth: 220
      Layout.alignment: Qt.AlignTop
      placeholderText: "Password"
    }
    PasswordField {
      Layout.preferredWidth: 220
      Layout.alignment: Qt.AlignTop
      text: "hunter22"
      revealable: true
    }
    PasswordField {
      Layout.preferredWidth: 220
      Layout.alignment: Qt.AlignTop
      text: "letmein"
      error: "Wrong password, try again"
    }
  }

  TextArea {
    Layout.preferredWidth: 480
    text: "Back up your dotfiles\nbefore switching themes,\nthen run omarchy update."
  }
}
