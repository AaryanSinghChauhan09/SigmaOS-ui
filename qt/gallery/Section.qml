import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui

// One gallery section: a caption naming it, then its specimens. Sections are the unit of
// comparison with the Rune gallery (spec/gallery.md), so each renders on its own with --section.
ColumnLayout {
  id: root

  property string name: ""
  property string title: ""
  default property alias specimens: body.data

  visible: gallery.section === "" || gallery.section === name
  spacing: Style.spacing.lg
  Layout.fillWidth: true

  PanelSectionHeader {
    text: root.title
    Layout.fillWidth: true
  }

  ColumnLayout {
    id: body
    spacing: Style.spacing.xxl
    Layout.fillWidth: true
  }
}
