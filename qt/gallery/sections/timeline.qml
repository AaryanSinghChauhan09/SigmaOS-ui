import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  id: root

  name: "timeline"
  title: "Timeline"

  property real position: 12
  property var tracks: [
    [{ start: 0, end: 7.5, label: "Intro" }, { start: 8, end: 18, label: "Demo" }, { start: 18.5, end: 30, label: "Outro" }],
    [{ start: 2, end: 12, label: "Voiceover" }, { start: 14, end: 26, label: "Music" }],
  ]
  property int selectedTrack: 0
  property int selectedClip: 1

  Timeline {
    duration: 30
    position: root.position
    tracks: root.tracks
    selectedTrack: root.selectedTrack
    selectedClip: root.selectedClip
    Layout.preferredWidth: 720
    Layout.preferredHeight: 160
    onSeek: function(time) { root.position = time }
    onClipPressed: function(track, clip) { root.selectedTrack = track; root.selectedClip = clip }
  }

  Scrubber {
    position: 42
    duration: 130
    Layout.preferredWidth: 360
  }

  RowLayout {
    spacing: Style.spacing.huge
    Layout.alignment: Qt.AlignTop

    // Rows reordered by their grips: the second under the cursor, a drop slot between the
    // third and fourth.
    Item {
      implicitWidth: 360
      implicitHeight: rows.implicitHeight
      Layout.alignment: Qt.AlignTop

      Column {
        id: rows
        width: parent.width
        Repeater {
          model: ["Workspaces", "Clock", "Battery", "Network"]
          delegate: Rectangle {
            required property string modelData
            required property int index
            width: rows.width
            height: Style.spacing.popupRowHeight
            radius: Style.cornerRadius
            color: index === 1 ? Style.hoverFill : "transparent"
            DragHandle {
              id: grip
              x: Style.spacing.sm
              anchors.verticalCenter: parent.verticalCenter
              hasCursor: index === 1
            }
            Text {
              x: grip.x + grip.width + Style.spacing.sm
              anchors.verticalCenter: parent.verticalCenter
              text: modelData
              color: Commons.Color.foreground
              font.family: Style.font.family
              font.pixelSize: Style.font.body
            }
          }
        }
      }
      DropIndicator {
        width: parent.width
        y: Style.spacing.popupRowHeight * 3 - Math.round(height / 2)
      }
    }

    Rectangle {
      implicitWidth: 240
      implicitHeight: 140
      Layout.alignment: Qt.AlignTop
      color: Style.normalFill
      CropFrame { anchors.fill: parent }
    }
  }
}
