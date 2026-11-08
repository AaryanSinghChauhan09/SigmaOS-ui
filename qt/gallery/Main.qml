import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons

Window {
  id: window

  title: "Omarchy UI · Qt"
  width: gallery.width
  height: Math.max(200, page.implicitHeight + Style.spacing.panelPadding * 2)
  visible: true
  color: Commons.Color.background

  Flickable {
    anchors.fill: parent
    contentHeight: page.implicitHeight + Style.spacing.panelPadding * 2
    clip: true

    Sections {
      id: page
      x: Style.spacing.panelPadding
      y: Style.spacing.panelPadding
      width: window.width - Style.spacing.panelPadding * 2
    }
  }
}
