import QtQuick
import QtQuick.Controls as QQC
import Omarchy.Theme
import Omarchy.Theme as Commons

// A multi-line TextField: the same chrome (normal border at rest, the hover-cursor look under
// the pointer, the cursor flag and focus), padding, type and placeholder, wrapping its text.
// It is `minRows` lines tall at least and grows with its text up to `maxRows`, then scrolls.
Item {
  id: root

  property alias text: area.text
  property alias placeholderText: area.placeholderText
  property alias area: area
  property int minRows: 3
  property int maxRows: 6
  property bool hasCursor: false
  property color foreground: Commons.Color.foreground
  property color accent: Commons.Color.accent
  property real horizontalPadding: Style.spacing.controlPaddingX
  property real verticalPadding: Style.spacing.inputPaddingY

  readonly property bool _focused: area.activeFocus
  readonly property bool _hot: hover.hovered || hasCursor
  readonly property var _borderSpec: Border.controlSpec(_focused ? "focus" : (_hot ? "hover-cursor" : "normal"), foreground, accent)
  // The laid-out line pitch, which can exceed the font's height by its leading.
  readonly property real _lineHeight: area.lineCount > 0 ? area.contentHeight / area.lineCount : metrics.height
  readonly property int _rows: Math.max(minRows, Math.min(maxRows, area.lineCount))

  implicitWidth: Style.space(240)
  implicitHeight: Math.ceil(_rows * _lineHeight) + area.topPadding + area.bottomPadding

  FontMetrics {
    id: metrics
    font: area.font
  }

  HoverHandler { id: hover }

  BorderSurface {
    anchors.fill: parent
    radius: Style.cornerRadius
    color: Style.controlFill(root._focused, root._hot, root.foreground, root.accent)
    borderSpec: root._borderSpec
  }

  Flickable {
    id: flick
    anchors.fill: parent
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    contentWidth: width
    contentHeight: area.implicitHeight
    QQC.ScrollBar.vertical: QQC.ScrollBar {
      policy: flick.contentHeight > flick.height ? QQC.ScrollBar.AsNeeded : QQC.ScrollBar.AlwaysOff
      contentItem: Rectangle {
        implicitWidth: Style.spacing.scrollbarThickness
        radius: Math.min(4, Style.cornerRadius)
        color: Commons.Color.textFaint
      }
    }

    QQC.TextArea.flickable: QQC.TextArea {
      id: area
      wrapMode: TextEdit.Wrap
      font.family: Style.font.family
      font.pixelSize: Style.font.body
      color: root.foreground
      selectionColor: Style.selectionFillFor(root.foreground, root.accent)
      selectedTextColor: root.foreground
      placeholderTextColor: Qt.darker(root.foreground, 1.6)
      leftPadding: root.horizontalPadding + Border.left(root._borderSpec)
      rightPadding: root.horizontalPadding + Border.right(root._borderSpec)
      topPadding: root.verticalPadding + Border.top(root._borderSpec)
      bottomPadding: root.verticalPadding + Border.bottom(root._borderSpec)
      background: null
    }
  }
}
