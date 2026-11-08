import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "surfaces"
  title: "Surfaces"

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
}
