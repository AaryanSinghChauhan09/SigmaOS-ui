import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui

// Every specimen, grouped as spec/gallery.md lists them. States that need a pointer or keyboard
// (hover, focus) are forced with the components' own `hasCursor` flag, which is what the shell's
// keyboard cursor sets, so a still render shows them.
ColumnLayout {
  id: root
  spacing: Style.spacing.huge * 2

  readonly property QtObject sliderColors: QtObject {
    property color foreground: Commons.Color.foreground
    property color background: Commons.Color.popups.background
  }

  Section {
    name: "buttons"
    title: "Buttons"

    RowLayout {
      spacing: Style.spacing.controlGap
      Button { text: "Plain" }
      Button { text: "Bordered"; bordered: true }
      Button { text: "Hover"; bordered: true; hasCursor: true }
      Button { text: "Selected"; bordered: true; selected: true }
      Button { text: "Icon"; iconText: ""; bordered: true }
      PanelActionButton { iconText: ""; bordered: true }
      PanelActionButton { iconText: ""; hasCursor: true; bordered: true }
    }

    ButtonGroup {
      options: [{ value: "left", label: "Left" }, { value: "center", label: "Center" }, { value: "right", label: "Right" }]
      value: "center"
    }
  }

  Section {
    name: "toggles"
    title: "Toggles"

    RowLayout {
      spacing: Style.spacing.xxl
      ToggleSwitch { checked: false }
      ToggleSwitch { checked: true }
      ToggleSwitch { checked: false; hasCursor: true }
      ToggleSwitch { checked: true; hasCursor: true }
    }

    Toggle {
      Layout.preferredWidth: 360
      label: "Wi-Fi"
      description: "Connected to Basecamp"
      checked: true
    }
    Toggle {
      Layout.preferredWidth: 360
      label: "Bluetooth"
      description: "Off"
      checked: false
      hasCursor: true
    }
  }

  Section {
    name: "inputs"
    title: "Inputs"

    RowLayout {
      spacing: Style.spacing.controlGap
      TextField { placeholderText: "Search…"; Layout.preferredWidth: 220 }
      TextField { text: "Omarchy"; Layout.preferredWidth: 220 }
      TextField { text: "Hovered"; hasCursor: true; Layout.preferredWidth: 220 }
    }

    RowLayout {
      spacing: Style.spacing.xxl
      NumberField { label: "Gaps"; value: 10; from: 0; to: 40 }
      Dropdown {
        label: "Theme"
        value: "tokyo-night"
        options: [{ value: "tokyo-night", label: "Tokyo Night" }, { value: "nord", label: "Nord" }]
      }
    }
  }

  Section {
    name: "slider"
    title: "Slider"

    PanelSlider {
      bar: root.sliderColors
      value: 0.35
      Layout.preferredWidth: 360
    }
    PanelSlider {
      bar: root.sliderColors
      value: 0.75
      tickCount: 5
      Layout.preferredWidth: 360
    }
  }

  Section {
    name: "surfaces"
    title: "Surfaces"

    RowLayout {
      spacing: Style.spacing.huge
      Layout.alignment: Qt.AlignTop

      Card {
        title: "Network"
        Layout.preferredWidth: 300
        Layout.alignment: Qt.AlignTop

        Toggle { label: "Wi-Fi"; description: "Basecamp"; checked: true; Layout.fillWidth: true }
        PanelSeparator { Layout.fillWidth: true }
        Column {
          Layout.fillWidth: true
          PopupMenuRow { text: "Basecamp"; checked: true }
          PopupMenuRow { text: "Basecamp Guest"; highlighted: true }
          PopupMenuRow { text: "Coffee Shop" }
        }
      }

      ColumnLayout {
        spacing: Style.spacing.huge
        Layout.alignment: Qt.AlignTop

        // A tooltip as it draws, held open (the real one shows on hover).
        BorderSurface {
          color: Commons.Color.tooltip.background
          borderSpec: Border.surfaceSpec("tooltip", "border", Commons.Color.tooltip.border, Math.max(1, Style.normalBorderWidth))
          implicitWidth: tip.implicitWidth + contentLeftInset + contentRightInset
          implicitHeight: tip.implicitHeight + contentTopInset + contentBottomInset
          leftPadding: Style.spacing.controlPaddingX
          rightPadding: Style.spacing.controlPaddingX
          topPadding: Style.spacing.controlPaddingY
          bottomPadding: Style.spacing.controlPaddingY
          Text {
            id: tip
            x: parent.contentLeftInset
            y: parent.contentTopInset
            text: "Restart Wi-Fi"
            color: Commons.Color.tooltip.text
            font.family: Style.font.family
            font.pixelSize: Style.font.bodySmall
          }
        }

        Item {
          implicitWidth: 340
          implicitHeight: 170
          ConfirmDialog {
            anchors.fill: parent
            opened: true
            message: "Remove the Coffee Shop network?"
          }
        }
      }
    }
  }

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
}
