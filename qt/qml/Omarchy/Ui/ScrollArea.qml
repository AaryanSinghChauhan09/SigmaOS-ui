import QtQuick
import QtQuick.Controls
import Omarchy.Theme
import Omarchy.Theme as Commons

// A vertical scroll view capped at `maxHeight`, with Omarchy's thin scrollbar beside it: a
// `track` groove `scrollbarThickness` wide and a foreground-glass thumb, corners min(4, radius).
// The bar shows only while the content overflows, but its gutter is always reserved, so rows
// never reflow when it appears. Scrolling stops at the ends; the thumb drags.
//
//   ScrollArea {
//     maxHeight: 240
//     ListRow { ... }
//     ListRow { ... }
//   }
//
// Children stack in a column `spacing` apart, as wide as the area less the gutter.
// ensureVisible(item) scrolls just far enough to show a child (the cursor's row after a key).
Item {
  id: root

  property real maxHeight: Number.POSITIVE_INFINITY
  property real spacing: 0
  default property alias content: column.data
  readonly property alias flickable: flick
  readonly property alias contentColumn: column
  readonly property int thickness: Style.spacing.scrollbarThickness
  readonly property real gutter: thickness + Style.space(4)
  readonly property bool overflowing: column.implicitHeight > flick.height + 0.5

  function ensureVisibleRange(y, h) {
    var top = flick.contentY
    var bottom = top + flick.height
    var maxY = Math.max(0, flick.contentHeight - flick.height)
    if (y < top) flick.contentY = Math.max(0, y)
    else if (y + h > bottom) flick.contentY = Math.min(maxY, y + h - flick.height)
  }

  function ensureVisible(item) {
    if (!item) return
    var p = item.mapToItem(column, 0, 0)
    ensureVisibleRange(p.y, item.height)
  }

  implicitWidth: column.implicitWidth + gutter
  implicitHeight: Math.min(column.implicitHeight, maxHeight)

  Flickable {
    id: flick
    width: root.width - root.gutter
    height: root.height
    contentWidth: width
    contentHeight: column.implicitHeight
    boundsBehavior: Flickable.StopAtBounds
    clip: true

    Column {
      id: column
      width: flick.width
      spacing: root.spacing
    }
  }

  ScrollBar {
    id: bar
    orientation: Qt.Vertical
    x: root.width - root.thickness
    width: root.thickness
    height: root.height
    padding: 0
    visible: root.overflowing
    policy: ScrollBar.AlwaysOn
    hoverEnabled: true
    size: flick.visibleArea.heightRatio
    minimumSize: Style.space(24) / Math.max(1, height)
    onPositionChanged: if (pressed) flick.contentY = position * flick.contentHeight

    Binding on position {
      value: flick.visibleArea.yPosition
      when: !bar.pressed
      restoreMode: Binding.RestoreNone
    }

    background: Rectangle {
      color: Commons.Color.track
      radius: Math.min(4, Style.cornerRadius)
    }

    contentItem: Rectangle {
      implicitWidth: root.thickness
      radius: Math.min(4, Style.cornerRadius)
      color: bar.pressed || bar.hovered ? Commons.Color.textDim : Style.normalBorderColor
    }
  }
}
