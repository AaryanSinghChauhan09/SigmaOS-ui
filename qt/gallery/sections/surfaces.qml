import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  id: section
  name: "surfaces"
  title: "Surfaces"

  function g(cp) { return String.fromCodePoint(cp) }

  RowLayout {
    spacing: Style.spacing.huge
    Layout.alignment: Qt.AlignTop

    Card {
      title: "Network"
      Layout.preferredWidth: 300
      Layout.alignment: Qt.AlignTop

      Toggle { label: "Wi-Fi"; description: "Basecamp"; checked: true; Layout.fillWidth: true }
      PanelSeparator { Layout.fillWidth: true }
      Column {
        Layout.fillWidth: true
        PopupMenuRow { text: "Basecamp"; checked: true }
        PopupMenuRow { text: "Basecamp Guest"; highlighted: true }
        PopupMenuRow { text: "Coffee Shop" }
      }
    }

    ColumnLayout {
      spacing: Style.spacing.huge
      Layout.alignment: Qt.AlignTop

      // A tooltip as it draws, held open (the real one shows on hover).
      BorderSurface {
        color: Commons.Color.tooltip.background
        radius: Style.cornerRadius
        borderSpec: Border.surfaceSpec("tooltip", "border", Commons.Color.tooltip.border, Math.max(1, Style.normalBorderWidth))
        implicitWidth: tip.implicitWidth + contentLeftInset + contentRightInset
        implicitHeight: tip.implicitHeight + contentTopInset + contentBottomInset
        leftPadding: Style.spacing.controlPaddingX
        rightPadding: Style.spacing.controlPaddingX
        topPadding: Style.spacing.controlPaddingY
        bottomPadding: Style.spacing.controlPaddingY
        Text {
          id: tip
          x: parent.contentLeftInset
          y: parent.contentTopInset
          text: "Restart Wi-Fi"
          color: Commons.Color.tooltip.text
          font.family: Style.font.family
          font.pixelSize: Style.font.bodySmall
        }
      }

      Item {
        implicitWidth: 340
        implicitHeight: 170
        ConfirmDialog {
          anchors.fill: parent
          opened: true
          message: "Remove the Coffee Shop network?"
        }
      }
    }
  }

  // Panes: an app's side panel with a rule against its neighbour, and a bordered settings group.
  RowLayout {
    spacing: Style.spacing.huge
    Layout.alignment: Qt.AlignTop

    Pane {
      title: "Layers"
      subtitle: "3 items"
      divider: "left"
      actions: [ IconButton { iconText: section.g(0xF067) } ]
      Layout.preferredWidth: 240
      Layout.alignment: Qt.AlignTop

      Column {
        Layout.fillWidth: true
        PopupMenuRow { text: "Background" }
        PopupMenuRow { text: "Title"; checked: true }
        PopupMenuRow { text: "Logo" }
      }
    }

    Pane {
      title: "Night light"
      subtitle: "Sunset to sunrise"
      bordered: true
      actions: [ ToggleSwitch { checked: true } ]
      Layout.preferredWidth: 240
      Layout.alignment: Qt.AlignTop

      SettingRow {
        Layout.fillWidth: true
        label: "Strength"
        ValueSlider { width: 100; value: 0.6 }
      }
    }
  }

  // Headings: the three levels, a trailing action, and a toolbar's centered title.
  RowLayout {
    spacing: Style.spacing.huge
    Layout.alignment: Qt.AlignTop

    ColumnLayout {
      spacing: Style.spacing.huge
      Layout.preferredWidth: 240
      Layout.fillWidth: false
      Layout.alignment: Qt.AlignTop

      Heading { text: "Settings"; subtitle: "Omarchy"; level: "display"; Layout.fillWidth: true }
      Heading { text: "Appearance"; subtitle: "Theme, font and rounding"; level: "heading"; Layout.fillWidth: true }
    }

    ColumnLayout {
      spacing: Style.spacing.huge
      Layout.preferredWidth: 240
      Layout.fillWidth: false
      Layout.alignment: Qt.AlignTop

      Heading {
        text: "Frame"
        Layout.fillWidth: true
        IconButton { iconText: section.g(0xF141) }
      }
      Heading { text: "notes.md"; subtitle: "~/Documents"; centered: true; Layout.fillWidth: true }
    }
  }
}
