import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "buttons"
  title: "Buttons"

  RowLayout {
    spacing: Style.spacing.controlGap
    Button { text: "Plain" }
    Button { text: "Bordered"; bordered: true }
    Button { text: "Hover"; bordered: true; hasCursor: true }
    Button { text: "Selected"; bordered: true; selected: true }
    Button { text: "Icon"; iconText: ""; bordered: true }
    PanelActionButton { iconText: ""; bordered: true }
    PanelActionButton { iconText: ""; hasCursor: true; bordered: true }
  }

  ButtonGroup {
    options: [{ value: "left", label: "Left" }, { value: "center", label: "Center" }, { value: "right", label: "Right" }]
    value: "center"
  }
}
