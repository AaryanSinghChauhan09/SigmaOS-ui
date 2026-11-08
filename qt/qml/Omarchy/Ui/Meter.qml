import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A determinate level on a track: battery, volume, usage against a limit. The shell draws it by
// hand in the OSD, the power panel and the agents panel; this is that bar on the kit's tokens.
//
//   Meter { value: 0.65; label: "Battery"; valueText: "65%" }
//   Meter { value: 0.92; alarming: true; markers: [0.5, 0.8] }
//
// The fill is the foreground by default; `tone` picks info (accent), success, warning or error
// (urgent), and `alarming` turns it urgent whatever the tone. `markers` are fractions drawn as
// ticks across the track, in the fill's color. ProgressBar is this with an accent fill and an
// indeterminate mode.
Item {
  id: root

  property real value: 0
  property string tone: "neutral"
  property bool alarming: false
  property var markers: []
  property string label: ""
  property string valueText: ""
  // A sliding segment instead of a fill, for work of unknown length (ProgressBar's).
  property bool indeterminate: false
  property real thickness: Style.spacing.meterThickness

  readonly property color fillColor: alarming ? Commons.Color.urgent
    : tone === "info" ? Commons.Color.accent
    : tone === "success" ? Commons.Color.success
    : tone === "warning" ? Commons.Color.warning
    : tone === "error" ? Commons.Color.urgent
    : Commons.Color.foreground

  readonly property bool _header: label !== "" || valueText !== ""
  readonly property real _fraction: Math.max(0, Math.min(1, value))
  readonly property real _tickWidth: Math.max(2, Math.round(thickness * 0.5))
  readonly property real _tickHeight: Math.round(thickness * 2.5)
  readonly property real _barHeight: markers.length > 0 ? Math.max(thickness, _tickHeight) : thickness
  // Small parts cap at min(4, radius); a thin bar never rounds past a pill.
  readonly property real _radius: Math.min(Math.min(4, Style.cornerRadius), thickness / 2)
  readonly property real _headerGap: Style.spacing.md

  implicitWidth: Style.space(200)
  implicitHeight: (_header ? header.height + _headerGap : 0) + _barHeight

  Accessible.role: Accessible.ProgressBar
  Accessible.name: label

  Item {
    id: header
    visible: root._header
    width: root.width
    height: Math.max(labelText.implicitHeight, valueLabel.implicitHeight)

    Text {
      id: labelText
      textFormat: Text.PlainText
      text: root.label
      color: Commons.Color.foreground
      font.family: Style.font.family
      font.pixelSize: Style.font.body
      width: Math.max(0, root.width - valueLabel.implicitWidth - Style.spacing.controlGap)
      elide: Text.ElideRight
    }

    Text {
      id: valueLabel
      textFormat: Text.PlainText
      anchors.right: parent.right
      text: root.valueText
      color: Commons.Color.textDim
      font.family: Style.font.family
      font.pixelSize: Style.font.body
    }
  }

  Item {
    id: bar
    y: root._header ? header.height + root._headerGap : 0
    width: root.width
    height: root._barHeight

    Rectangle {
      id: track
      y: Math.round((bar.height - root.thickness) / 2)
      width: bar.width
      height: root.thickness
      radius: root._radius
      color: Commons.Color.track
      clip: root.indeterminate

      Rectangle {
        visible: !root.indeterminate
        width: track.width * root._fraction
        height: track.height
        radius: track.radius
        color: root.fillColor

        Behavior on width {
          NumberAnimation { duration: Style.duration(160); easing.type: Easing.OutCubic }
        }
      }

      // A quarter of the track sweeps across it and wraps; under reduced motion it holds still in
      // the middle, so the bar still reads as busy.
      Rectangle {
        id: segment
        visible: root.indeterminate
        width: Math.round(track.width * 0.25)
        height: track.height
        radius: track.radius
        color: root.fillColor
        x: (track.width - width) / 2

        NumberAnimation on x {
          running: root.indeterminate && root.visible && !Style.reduceMotion
          from: -segment.width
          to: track.width
          duration: 1400
          loops: Animation.Infinite
          easing.type: Easing.InOutQuad
        }
      }
    }

    Repeater {
      model: root.indeterminate ? [] : root.markers

      Rectangle {
        required property var modelData
        // Measured so the first and last ticks sit inside the track's ends.
        readonly property real along: (track.width - width) * Math.max(0, Math.min(1, Number(modelData)))
        x: LayoutMirroring.enabled ? track.width - width - along : along
        width: root._tickWidth
        height: root._tickHeight
        y: Math.round((bar.height - height) / 2)
        radius: Math.min(Math.min(4, Style.cornerRadius), width / 2)
        color: root.fillColor
      }
    }
  }
}
