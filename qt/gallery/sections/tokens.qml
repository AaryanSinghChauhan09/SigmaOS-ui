import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "tokens"
  title: "Tokens"

  Flow {
    Layout.fillWidth: true
    spacing: Style.spacing.lg
    Repeater {
      model: [
        { name: "foreground", c: Commons.Color.foreground },
        { name: "background", c: Commons.Color.background },
        { name: "accent", c: Commons.Color.accent },
        { name: "urgent", c: Commons.Color.urgent },
        { name: "muted", c: Commons.Color.muted },
      ]
      delegate: Column {
        spacing: Style.spacing.xs
        Rectangle {
          width: 64; height: 32
          radius: Math.min(4, Style.cornerRadius)
          color: modelData.c
          border.width: 1
          border.color: Style.normalBorderColor
        }
        Text {
          text: modelData.name
          color: Commons.Color.foreground
          font.family: Style.font.family
          font.pixelSize: Style.font.caption
        }
      }
    }
  }

  Flow {
    Layout.fillWidth: true
    spacing: Style.spacing.lg
    Repeater {
      model: ["normal", "hover-cursor", "focus", "selected"]
      delegate: BorderSurface {
        readonly property var spec: Border.controlSpec(modelData, Commons.Color.foreground, Commons.Color.accent)
        width: 120; height: Style.spacing.controlHeight
        radius: Style.cornerRadius
        borderSpec: spec
        color: modelData === "normal" ? Style.normalFill
          : modelData === "hover-cursor" ? Style.hoverFill
          : modelData === "focus" ? Style.focusFillColor
          : Style.selectedFill
        Text {
          anchors.centerIn: parent
          text: modelData
          color: modelData === "selected" ? Style.selectedStateColor(Commons.Color.foreground, Commons.Color.accent) : Commons.Color.foreground
          font.family: Style.font.family
          font.pixelSize: Style.font.body
          font.bold: modelData === "selected"
        }
      }
    }
  }

  Column {
    spacing: Style.spacing.xs
    Repeater {
      model: [
        ["displayLarge", Style.font.displayLarge], ["display", Style.font.display], ["heading", Style.font.heading],
        ["title", Style.font.title], ["subtitle", Style.font.subtitle], ["body", Style.font.body],
        ["bodySmall", Style.font.bodySmall], ["caption", Style.font.caption],
      ]
      delegate: Text {
        text: modelData[0] + " " + modelData[1]
        color: Commons.Color.foreground
        font.family: Style.font.family
        font.pixelSize: modelData[1]
      }
    }
  }
}
