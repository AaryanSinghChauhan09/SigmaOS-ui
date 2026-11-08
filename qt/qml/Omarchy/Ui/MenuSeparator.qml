import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A rule between groups of menu rows: 1px in the track color, sm above and below.
Item {
  width: parent ? parent.width : 0
  implicitWidth: 0
  implicitHeight: Style.spacing.sm * 2 + 1

  Rectangle {
    y: Style.spacing.sm
    width: parent.width
    height: 1
    color: Commons.Color.track
  }
}
