import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A check row: a small square box, then a label and an optional dim description. Checked fills
// the box with the selected state and marks it with an accent check, since accent marks what is
// chosen. `exclusive` draws a radio instead (a circle with an accent dot); `Radio` is that preset.
//
// The caller owns the value, as with ToggleSwitch: bind `checked` and flip it on `toggled()`.
// Space (and Return) toggle while focused; focus and `hasCursor` look like hover, on the box.
Item {
  id: root

  property string label: ""
  property string description: ""
  property bool checked: false
  property bool exclusive: false

  // Panel-cursor flag, as Button.hasCursor: forces the hover look on the box.
  property bool hasCursor: false

  property color foreground: Commons.Color.foreground
  property color accent: Commons.Color.accent
  property string fontFamily: Style.font.family

  signal toggled()
  signal hovered(bool isHovered)

  readonly property int boxSize: Style.space(16)
  readonly property bool _hot: enabled && (hasCursor || mouse.containsMouse || activeFocus)
  readonly property string _state: _hot ? "hover-cursor" : (checked ? "selected" : "normal")
  readonly property int _labelLine: labelText.implicitHeight
  readonly property int _boxY: Math.max(0, Math.round((_labelLine - boxSize) / 2))
  readonly property int _textY: Math.max(0, Math.round((boxSize - _labelLine) / 2))

  activeFocusOnTab: enabled
  Keys.onSpacePressed: root.toggled()
  Keys.onReturnPressed: root.toggled()
  Keys.onEnterPressed: root.toggled()

  implicitWidth: boxSize + Style.spacing.rowGap + Math.max(labelText.implicitWidth, descriptionText.implicitWidth)
  implicitHeight: Math.max(_boxY + boxSize, _textY + words.implicitHeight)

  BorderSurface {
    id: box
    y: root._boxY
    width: root.boxSize
    height: root.boxSize
    opacity: root.enabled ? 1 : 0.5
    radius: root.exclusive ? root.boxSize / 2 : Math.min(4, Style.cornerRadius)
    color: root._hot ? Style.hoverFillFor(root.foreground, root.accent)
      : root.checked ? Style.selectedFillFor(root.foreground, root.accent)
      : Style.normalFillFor(root.foreground, root.accent)
    borderSpec: Border.controlSpec(root._state, root.foreground, root.accent)

    Behavior on color { ColorAnimation { duration: Style.duration(120) } }

    Text {
      anchors.centerIn: parent
      visible: root.checked && !root.exclusive
      text: "✓"
      color: root.accent
      font.family: root.fontFamily
      font.pixelSize: Math.round(root.boxSize * 0.85)
      font.bold: true
    }

    Rectangle {
      readonly property int dot: Math.round(root.boxSize / 2)
      anchors.centerIn: parent
      visible: root.checked && root.exclusive
      width: dot
      height: dot
      radius: dot / 2
      color: root.accent
    }
  }

  Column {
    id: words
    x: root.boxSize + Style.spacing.rowGap
    y: root._textY
    width: Math.max(0, root.width - x)
    spacing: Style.spacing.xxs

    Text {
      id: labelText
      textFormat: Text.PlainText
      text: root.label
      color: root.enabled ? root.foreground : Commons.Color.textFaint
      font.family: root.fontFamily
      font.pixelSize: Style.font.body
      elide: Text.ElideRight
      width: parent.width
    }

    Text {
      id: descriptionText
      textFormat: Text.PlainText
      visible: root.description !== ""
      text: root.description
      color: root.enabled ? Commons.Color.textDim : Commons.Color.textFaint
      font.family: root.fontFamily
      font.pixelSize: Style.font.caption
      wrapMode: Text.WordWrap
      width: parent.width
    }
  }

  MouseArea {
    id: mouse
    anchors.fill: parent
    enabled: root.enabled
    hoverEnabled: true
    cursorShape: Qt.PointingHandCursor
    onContainsMouseChanged: root.hovered(containsMouse)
    onClicked: root.toggled()
  }
}
