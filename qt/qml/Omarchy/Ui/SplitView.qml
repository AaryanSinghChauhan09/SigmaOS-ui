import QtQuick
import QtQuick.Controls as QQC
import Omarchy.Theme
import Omarchy.Theme as Commons

// Qt Quick's SplitView with Omarchy's handle: a `md`-wide strip holding a 1px
// separator line in the track color, lit with the hover glass under the
// pointer and while dragged. Panes size themselves with the attached
// properties (`QQC.SplitView.preferredWidth`, `minimumWidth`, `fillWidth`);
// set a pane's `visible: false` to collapse it, and its handle goes with it.
//
//   O.SplitView {
//     Sidebar { QQC.SplitView.preferredWidth: 200; QQC.SplitView.minimumWidth: 120 }
//     Item { QQC.SplitView.fillWidth: true }
//   }
QQC.SplitView {
  id: root

  // Force the handles' hot look, for still renders.
  property bool hasCursor: false
  property color foreground: Commons.Color.foreground
  property color accent: Commons.Color.accent

  readonly property int handleThickness: Style.spacing.md

  handle: Item {
    id: handle
    implicitWidth: root.orientation === Qt.Horizontal ? root.handleThickness : root.width
    implicitHeight: root.orientation === Qt.Horizontal ? root.height : root.handleThickness

    readonly property bool hot: root.hasCursor || handle.QQC.SplitHandle.hovered || handle.QQC.SplitHandle.pressed

    Rectangle {
      anchors.fill: parent
      radius: Math.min(4, Style.cornerRadius)
      color: handle.hot ? Style.hoverFillFor(root.foreground, root.accent) : "transparent"
    }

    Rectangle {
      readonly property bool across: root.orientation === Qt.Horizontal
      x: across ? Math.floor((parent.width - width) / 2) : 0
      y: across ? 0 : Math.floor((parent.height - height) / 2)
      width: across ? Style.spacing.hairline : parent.width
      height: across ? parent.height : Style.spacing.hairline
      color: Commons.Color.track
    }
  }
}
