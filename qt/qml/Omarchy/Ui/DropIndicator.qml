import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// Where a dragged row will land: an accent line `max(1, space(2))` thick, as wide as the list.
// Place it centered on the boundary between two rows, over them (it takes no layout space).
Rectangle {
  implicitWidth: Style.space(200)
  implicitHeight: Math.max(1, Style.space(2))
  radius: Math.min(Math.min(4, Style.cornerRadius), height / 2)
  color: Commons.Color.accent
}
