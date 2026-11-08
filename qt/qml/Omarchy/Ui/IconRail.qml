import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A vertical rail of icon tabs, as an app's side panel switcher: square buttons sm apart, the
// selected glyph in the accent with an accent bar at the rail's leading edge that slides between
// them, a 1px track rule down the trailing edge. Under the pointer or the keyboard cursor a button
// takes the hover fill; the rest are dim.
//
//   IconRail {
//     model: [{ icon: "", label: "Files" }, { icon: "", label: "Search" }]
//     onActivated: (i) => panel = i
//   }
//
// Keyboard (focusable): Up/Down or k/j move the selection, Home/End jump.
FocusScope {
  id: root

  property var model: []
  property int currentIndex: 0
  property int cursorIndex: -1

  signal activated(int index)

  function select(i) {
    var n = repeater.count
    if (n === 0) return
    i = Math.max(0, Math.min(n - 1, i))
    if (i === currentIndex) return
    currentIndex = i
    activated(i)
  }

  readonly property real itemSize: Style.spacing.controlHeight + Style.spacing.sm * 2
  readonly property real _pad: Style.spacing.sm

  activeFocusOnTab: true
  implicitWidth: itemSize + _pad * 2 + 1
  implicitHeight: column.implicitHeight + _pad * 2

  Keys.onPressed: function(event) {
    if (event.key === Qt.Key_Up || event.text === "k") select(currentIndex - 1)
    else if (event.key === Qt.Key_Down || event.text === "j") select(currentIndex + 1)
    else if (event.key === Qt.Key_Home) select(0)
    else if (event.key === Qt.Key_End) select(repeater.count - 1)
    else return
    event.accepted = true
  }

  Rectangle {
    x: root.width - 1
    width: 1
    height: root.height
    color: Commons.Color.track
  }

  Column {
    id: column
    x: root._pad
    y: root._pad
    spacing: Style.spacing.sm

    Repeater {
      id: repeater
      model: root.model
      delegate: Rectangle {
        id: item
        required property var modelData
        required property int index
        readonly property bool selected: index === root.currentIndex
        readonly property bool hot: mouse.containsMouse || index === root.cursorIndex || (root.activeFocus && selected)

        width: root.itemSize
        height: root.itemSize
        radius: Style.cornerRadius
        color: mouse.pressed ? Style.pressedFill : hot ? Style.hoverFill : "transparent"
        Behavior on color { ColorAnimation { duration: Style.duration(120) } }

        OpticalGlyph {
          anchors.fill: parent
          text: item.modelData.icon
          fontSize: Style.font.iconLarge
          color: item.selected ? Commons.Color.accent : item.hot ? Commons.Color.foreground : Commons.Color.textDim
        }

        MouseArea {
          id: mouse
          anchors.fill: parent
          hoverEnabled: true
          cursorShape: Qt.PointingHandCursor
          onClicked: { root.forceActiveFocus(); root.select(item.index) }
        }
      }
    }
  }

  Rectangle {
    readonly property Item target: repeater.count > root.currentIndex && root.currentIndex >= 0 ? repeater.itemAt(root.currentIndex) : null
    visible: target !== null
    x: 0
    y: target ? column.y + target.y + Math.round((target.height - height) / 2) : 0
    width: Style.spacing.xxs
    height: Style.font.iconLarge
    radius: Math.min(1, Style.cornerRadius)
    color: Commons.Color.accent
    Behavior on y { NumberAnimation { duration: Style.duration(160); easing.type: Easing.Bezier; easing.bezierCurve: Style.easeOut } }
  }
}
