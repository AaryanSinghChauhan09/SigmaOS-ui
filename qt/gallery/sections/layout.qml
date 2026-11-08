import QtQuick
import QtQuick.Controls as QQC
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "layout"
  title: "Layout"

  // The split view framed by a track-colored line, so its edges show.
  Item {
    Layout.preferredWidth: 720
    Layout.preferredHeight: 220

    SplitView {
      anchors.fill: parent

      Sidebar {
        QQC.SplitView.preferredWidth: 200
        QQC.SplitView.minimumWidth: 120
        sections: [
          { title: "Library", items: [
            { id: "all", label: "All files", glyph: "󰉋" },
            { id: "recent", label: "Recent", glyph: "󰋚" },
            { id: "starred", label: "Starred", glyph: "󰓎" },
            { id: "trash", label: "Trash", glyph: "󰩺" }
          ] },
          { title: "Places", items: [
            { id: "documents", label: "Documents", glyph: "󰈙" },
            { id: "downloads", label: "Downloads", glyph: "󰇚" }
          ] }
        ]
        selectedId: "recent"
      }

      Item {
        QQC.SplitView.fillWidth: true
        Text {
          anchors.centerIn: parent
          textFormat: Text.PlainText
          text: "No file selected"
          color: Commons.Color.textFaint
          font.family: Style.font.family
          font.pixelSize: Style.font.body
        }
      }
    }

    Rectangle {
      anchors.fill: parent
      color: "transparent"
      radius: Style.cornerRadius
      border.width: Style.spacing.hairline
      border.color: Commons.Color.track
    }
  }

  Calendar {
    year: 2026
    month: 11
    today: "2026-11-08"
    selected: "2026-11-14"
  }
}
