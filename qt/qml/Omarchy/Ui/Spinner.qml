import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// Busy: a Nerd Font glyph turning once every 900 ms, as the shell's weather panel and
// MultiSelect spin theirs, or with `pulse` a glyph breathing between full and half opacity, as
// the Tailscale panel marks an account switching. Under reduced motion the glyph stands still
// (a pulse rests at half opacity).
//
//   Spinner {}
//   Spinner { pulse: true; glyph: ""; size: Style.font.iconLarge }
Item {
  id: root

  property string glyph: "\u{f0996}"
  property real size: Style.font.icon
  property bool pulse: false
  property color color: Commons.Color.foreground
  property bool running: true

  // A square of the glyph's line height, the glyph's ink centered across it, so it turns about
  // its own middle rather than its advance box's.
  implicitWidth: implicitHeight
  implicitHeight: mark.implicitHeight

  Accessible.role: Accessible.Animation
  Accessible.name: "Busy"

  TextMetrics {
    id: metrics
    font: mark.font
    text: root.glyph
  }

  Item {
    id: spin
    anchors.fill: parent
    transformOrigin: Item.Center

    RotationAnimator on rotation {
      running: !root.pulse && root.running && root.visible && !Style.reduceMotion
      from: 0
      to: 360
      duration: 900
      loops: Animation.Infinite
    }

    Text {
      id: mark
      textFormat: Text.PlainText
      x: Math.round((root.width - metrics.tightBoundingRect.width) / 2 - metrics.tightBoundingRect.x)
      y: Math.round((root.height - height) / 2)
      text: root.glyph
      color: root.color
      font.family: Style.font.family
      font.pixelSize: root.size
      opacity: root.pulse ? 0.5 : 1

      SequentialAnimation on opacity {
        running: root.pulse && root.running && root.visible && !Style.reduceMotion
        loops: Animation.Infinite
        NumberAnimation { to: 1.0; duration: 420; easing.type: Easing.InOutQuad }
        NumberAnimation { to: 0.5; duration: 420; easing.type: Easing.InOutQuad }
      }
    }
  }
}
