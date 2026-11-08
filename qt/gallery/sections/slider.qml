import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  id: root

  readonly property QtObject sliderColors: QtObject {
    property color foreground: Commons.Color.foreground
    property color background: Commons.Color.popups.background
  }

  name: "slider"
  title: "Slider"

  PanelSlider {
    bar: root.sliderColors
    value: 0.35
    Layout.preferredWidth: 360
  }
  PanelSlider {
    bar: root.sliderColors
    value: 0.75
    tickCount: 5
    Layout.preferredWidth: 360
  }
}
