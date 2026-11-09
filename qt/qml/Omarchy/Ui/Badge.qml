import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A count or a short label in a small pill: unread counts, "New", "Offline".
//
//   Badge { text: "3"; tone: "accent"; filled: true }
//   Badge { text: "Offline"; tone: "urgent" }
//
// `tone` is "" (neutral glass: the normal control fill and border), "accent", "success",
// "warning", "urgent" or "muted" (the tone at the selected fill's alpha, text in the tone). `filled` paints
// the tone solid with the background as text, for counts that must stand out. Never narrower
// than tall, so a one-digit count is square (round only when Hyprland rounds, up to 4).
BorderSurface {
  id: root

  property string text: ""
  property string tone: ""
  property bool filled: false

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
  readonly property bool neutral: tone === "" && !filled
  // Every tone reserves the neutral border, so badges line up whatever their tone.
  readonly property real line: Style.normalBorderWidth

  radius: Math.min(4, Style.cornerRadius)
  color: filled ? toneColor : (neutral ? Style.normalFill : Commons.Util.alpha(toneColor, Style.selectedFillAlpha))
  borderSpec: neutral ? Border.controlSpec("normal", Commons.Color.foreground, Commons.Color.accent) : Border.none()

  implicitHeight: label.implicitHeight + (Style.space(1) + line) * 2
  implicitWidth: Math.max(implicitHeight, label.implicitWidth + (Style.space(6) + line) * 2)

  Text {
    id: label
    anchors.centerIn: parent
    textFormat: Text.PlainText
    text: root.text
    color: root.filled ? Commons.Color.background : root.toneColor
    font.family: Style.font.family
    font.pixelSize: Style.font.caption
    font.bold: true
  }
}
