import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A small status dot ringed in the surface color, so it reads as separate from whatever it sits
// on: dock it on an icon's or an Avatar's corner (`Avatar { status: "success" }` does that), or
// set it inline before a label.
//
//   StatusDot { tone: "success" }
//   StatusDot { tone: "urgent"; anchors.right: icon.right; anchors.bottom: icon.bottom }
//
// `tone`: "success", "warning", "urgent", "muted", "accent" (or "foreground"). Always a circle.
Item {
  id: root

  property string tone: "success"
  property real size: Style.space(8)
  property real ring: Math.max(1, Style.space(2))
  // The surface the dot sits on.
  property color ground: Commons.Color.popups.background

  readonly property color toneColor: {
    switch (tone) {
    case "accent": return Commons.Color.accent
    case "success": return Commons.Color.success
    case "warning": return Commons.Color.warning
    case "urgent": return Commons.Color.urgent
    case "muted": return Commons.Color.muted
    }
    return Commons.Color.foreground
  }

  implicitWidth: size + ring * 2
  implicitHeight: implicitWidth

  Rectangle {
    anchors.fill: parent
    radius: width / 2
    color: root.ring > 0 ? root.ground : "transparent"
  }

  Rectangle {
    x: root.ring
    y: root.ring
    width: root.size
    height: root.size
    radius: width / 2
    color: root.toneColor
  }
}
