import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "messages"
  title: "Messages"

  RowLayout {
    spacing: Style.spacing.huge

    Item {
      implicitWidth: 350
      implicitHeight: 150
      EmptyState {
        anchors.centerIn: parent
        width: parent.width
        glyph: "\u{f05aa}"
        title: "No networks found"
        hint: "Turn on Wi-Fi to scan"
        actionText: "Scan"
      }
    }

    Item {
      implicitWidth: 350
      implicitHeight: 150
      EmptyState {
        anchors.centerIn: parent
        width: parent.width
        size: "overlay"
        glyph: "\u{f014c}"
        title: "Clipboard is empty"
        hint: "Copy something to see it here"
      }
    }
  }

  Banner { tone: "info"; text: "Omarchy 3.2 is available"; actionText: "Update"; dismissible: true; Layout.fillWidth: true }
  Banner { tone: "success"; text: "Backup finished"; dismissible: true; Layout.fillWidth: true }
  Banner { tone: "warning"; text: "Battery is low (12%)"; dismissible: true; Layout.fillWidth: true }
  Banner { tone: "error"; text: "Could not reach the update server"; dismissible: true; Layout.fillWidth: true }

  // A toast as it draws, held open (the app owns when it shows and the timer).
  Toast {
    glyph: "\u{f012c}"
    title: "Export finished"
    text: "omarchy-demo.mp4 · 42 MB"
    actionText: "Open"
    countdown: 0.6
  }
}
