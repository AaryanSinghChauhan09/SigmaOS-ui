import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "navigation"
  title: "Navigation"

  // Three groups split by separators, a centered title, a trailing button and the overflow.
  Toolbar {
    Layout.fillWidth: true
    IconButton { iconText: "" }
    IconButton { iconText: ""; hasCursor: true }
    IconButton { iconText: "" }
    ToolbarSeparator {}
    IconButton { iconText: "" }
    IconButton { iconText: "" }
    ToolbarSeparator {}
    IconButton { iconText: "" }
    IconButton { iconText: "" }
    center: [
      Text {
        textFormat: Text.PlainText
        text: "notes.md"
        color: Commons.Color.textDim
        font.family: Style.font.family
        font.pixelSize: Style.font.body
      }
    ]
    trailing: [ Button { text: "Share"; bordered: true } ]
    overflow: true
  }

  Tabs {
    Layout.fillWidth: true
    model: ["General", "Display", "Audio", "Network"]
    currentIndex: 1
    cursorIndex: 2
  }

  RowLayout {
    spacing: 0
    IconRail {
      id: rail
      model: [
        { icon: "", label: "Files" },
        { icon: "", label: "Search" },
        { icon: "", label: "Source control" },
        { icon: "", label: "Debug" },
        { icon: "", label: "Settings" },
      ]
      currentIndex: 1
    }
    Rectangle {
      implicitWidth: Style.space(320)
      implicitHeight: rail.implicitHeight
      color: "transparent"
      Text {
        anchors.centerIn: parent
        textFormat: Text.PlainText
        text: "Search"
        color: Commons.Color.textDim
        font.family: Style.font.family
        font.pixelSize: Style.font.body
      }
    }
  }
}
