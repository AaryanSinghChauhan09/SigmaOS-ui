import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "color"
  title: "Color"

  // A fixed color, not the theme's accent, so the picker's content is the same in every theme.
  ColorPicker {
    Layout.preferredWidth: 300
    Layout.preferredHeight: 260
    Component.onCompleted: setColor("#7aa2f7")
  }
}
