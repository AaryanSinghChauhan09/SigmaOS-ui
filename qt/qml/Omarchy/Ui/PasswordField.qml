import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A TextField for secrets: dots instead of text, a lock glyph before them, an optional eye that
// reveals what's typed, and an error state. Lock (LockView) and polkit (PolkitAgent) are the
// shell's two, and the error colors are polkit's: `textError` for the glyph and the message,
// `borderError` for the border.
//
// Setting `error` to a message shows it under the field and flashes and shakes the field (the
// polkit shake); `shake()` replays that for a repeated failure. Under reduced motion both land at
// once, so the field only changes color.
Item {
  id: root

  property alias text: field.text
  property alias placeholderText: field.placeholderText
  property alias field: field
  property bool revealable: false
  property bool revealed: false
  property string error: ""
  property bool hasCursor: false
  property color foreground: Commons.Color.foreground
  property color accent: Commons.Color.accent

  signal accepted()

  readonly property bool hasError: error !== ""
  readonly property color errorColor: Commons.Color.polkit.textError

  property real _shake: 0
  property real _flash: 0

  function shake() {
    shakeAnimation.restart()
    flashAnimation.restart()
  }

  onErrorChanged: if (error !== "") shake()

  implicitWidth: Style.space(240)
  implicitHeight: field.implicitHeight + (hasError ? Style.spacing.labelGap + message.implicitHeight : 0)

  TextField {
    id: field
    x: root._shake
    width: root.width
    password: !root.revealed
    passwordCharacter: "•"
    passwordMaskDelay: 0
    // textFaint, as every placeholder.
    placeholderTextColor: Commons.Color.textFaint
    hasCursor: root.hasCursor
    foreground: root.foreground
    accent: root.accent
    leftPadding: horizontalPadding + Border.left(_borderSpec) + lock.width + Style.spacing.controlGap
    rightPadding: horizontalPadding + Border.right(_borderSpec) + (root.revealable ? eye.width + Style.spacing.controlGap : 0)
    onAccepted: root.accepted()

    background: BorderSurface {
      radius: Style.cornerRadius
      color: Style.controlFill(field._focused, field._hot, root.foreground, root.accent)
      borderSpec: root.hasError
        ? Border.flat(Commons.Color.polkit.borderError, Math.max(1, Border.top(field._borderSpec)))
        : field._borderSpec

      Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: Util.alpha(root.errorColor, Style.selectedFillAlpha * root._flash)
      }
    }

    Text {
      id: lock
      x: field.horizontalPadding + Border.left(field._borderSpec)
      anchors.verticalCenter: parent.verticalCenter
      text: ""
      color: root.hasError ? root.errorColor : Commons.Color.textDim
      font.family: Style.font.family
      font.pixelSize: Style.font.body
    }

    Text {
      id: eye
      visible: root.revealable
      anchors.right: parent.right
      anchors.rightMargin: field.horizontalPadding + Border.right(field._borderSpec)
      anchors.verticalCenter: parent.verticalCenter
      text: root.revealed ? "󰈉" : "󰈈"
      color: eyeMouse.containsMouse ? root.foreground : Commons.Color.textDim
      font.family: Style.font.family
      font.pixelSize: Style.font.body

      MouseArea {
        id: eyeMouse
        anchors.fill: parent
        anchors.margins: -Style.spacing.xs
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.revealed = !root.revealed
      }
    }
  }

  Text {
    id: message
    visible: root.hasError
    y: field.height + Style.spacing.labelGap
    width: root.width
    textFormat: Text.PlainText
    text: root.error
    color: root.errorColor
    font.family: Style.font.family
    font.pixelSize: Style.font.caption
    wrapMode: Text.WordWrap
  }

  SequentialAnimation {
    id: shakeAnimation
    NumberAnimation { target: root; property: "_shake"; to: -Style.space(8); duration: Style.duration(35); easing.type: Easing.OutQuad }
    NumberAnimation { target: root; property: "_shake"; to: Style.space(8); duration: Style.duration(50); easing.type: Easing.InOutQuad }
    NumberAnimation { target: root; property: "_shake"; to: 0; duration: Style.duration(55); easing.type: Easing.OutQuad }
  }

  SequentialAnimation {
    id: flashAnimation
    PropertyAction { target: root; property: "_flash"; value: Style.reduceMotion ? 0 : 1 }
    NumberAnimation { target: root; property: "_flash"; to: 0; duration: Style.duration(400); easing.type: Easing.OutQuad }
  }
}
