import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A crop rectangle: a 1px accent outline, rule-of-thirds guides in `track`, and eight crop
// TrimHandles (four corner brackets, four edge bars) inside its edges. Size and place it over
// the image; it reports drags per handle and the owner moves the rectangle.
Item {
  id: root

  // The handle drawn hot, as if under the pointer ("" for none).
  property string cursorEdge: ""

  signal pressed(string edge)
  signal moved(string edge, real dx, real dy)
  signal released(string edge)
  signal nudged(string edge, real dx, real dy)

  readonly property var edges: ["top-left", "top", "top-right", "left", "right", "bottom-left", "bottom", "bottom-right"]

  implicitWidth: Style.space(240)
  implicitHeight: Style.space(140)

  Repeater {
    model: 2
    Rectangle {
      required property int index
      x: Math.round(root.width * (index + 1) / 3)
      width: 1
      height: root.height
      color: Commons.Color.track
    }
  }
  Repeater {
    model: 2
    Rectangle {
      required property int index
      y: Math.round(root.height * (index + 1) / 3)
      width: root.width
      height: 1
      color: Commons.Color.track
    }
  }

  Rectangle {
    anchors.fill: parent
    color: "transparent"
    border.width: 1
    border.color: Commons.Color.accent
  }

  Repeater {
    model: root.edges
    delegate: TrimHandle {
      required property string modelData
      crop: true
      edge: modelData
      hasCursor: root.cursorEdge === modelData
      x: modelData.indexOf("left") >= 0 ? 0
        : modelData.indexOf("right") >= 0 ? root.width - width
        : Math.round((root.width - width) / 2)
      y: modelData.indexOf("top") === 0 ? 0
        : modelData.indexOf("bottom") === 0 ? root.height - height
        : Math.round((root.height - height) / 2)
      onDraggingChanged: dragging ? root.pressed(modelData) : root.released(modelData)
      onMoved: function(dx, dy) { root.moved(modelData, dx, dy) }
      onNudged: function(dx, dy) { root.nudged(modelData, dx, dy) }
    }
  }
}
