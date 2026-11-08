import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A label heading a group of menu rows: the section header style (caption, bold, dim), inset
// like the rows' labels.
Item {
  id: root

  property string text: ""

  width: parent ? parent.width : implicitWidth
  implicitWidth: Math.ceil(header.implicitWidth) + Style.spacing.controlPaddingX * 2
  implicitHeight: header.implicitHeight + Style.spacing.sm + Style.spacing.xxs

  PanelSectionHeader {
    id: header
    x: Style.spacing.controlPaddingX
    y: Style.spacing.sm
    text: root.text
  }
}
