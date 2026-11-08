import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

// Real UIs built only from kit components and tokens: the test that the kit is complete. No
// Rectangle or Text of its own, only layout.
Section {
  id: section
  name: "composed"
  title: "Composed"

  function g(cp) { return String.fromCodePoint(cp) }

  readonly property var searchItems: [
    { glyph: g(0xF120), title: "Terminal", subtitle: "App", detail: "Open a new terminal window." },
    { glyph: g(0xF031), title: "Terminal font", subtitle: "Style", detail: "Pick the monospace font every terminal uses." },
    { glyph: g(0xF1FC), title: "Terminal theme", subtitle: "Style", detail: "Match the terminal colors to the desktop theme." },
    { glyph: g(0xF0A3), title: "Intermediate CA", subtitle: "Certificate", detail: "Trust a certificate authority for this machine." },
  ]

  RowLayout {
    spacing: Style.spacing.huge * 2

    // A Wi-Fi panel, as the shell's network panel.
    Card {
      Layout.preferredWidth: 360
      Layout.alignment: Qt.AlignTop

      Hero {
        Layout.fillWidth: true
        glyph: section.g(0xF0928); title: "Wi-Fi"; status: "Connected to Basecamp"
        trailing: Component { ToggleSwitch { checked: true } }
      }

      SectionHeader { text: "Networks"; Layout.fillWidth: true }

      ScrollArea {
        Layout.fillWidth: true
        maxHeight: 280
        spacing: Style.spacing.xxs

        ListRow {
          glyph: section.g(0xF0928); title: "Basecamp"; status: "Connected"; statusTone: "success"
          current: true
        }
        ListRow {
          glyph: section.g(0xF0925); title: "Basecamp Guest"; subtitle: "Open"
          actionGlyph: section.g(0xF0159); actionUrgent: true
          hasCursor: true
        }
        ListRow {
          glyph: section.g(0xF0922); title: "Coffee Shop"; subtitle: "WPA2"; value: section.g(0xF033E)
          expansion: Component {
            RowLayout {
              spacing: Style.spacing.md
              PasswordField { Layout.fillWidth: true; placeholderText: "Password" }
              Button { text: "Connect"; bordered: true }
            }
          }
        }
        ListRow {
          glyph: section.g(0xF091F); title: "Library"; subtitle: "Saved"
        }
      }

      Separator { Layout.fillWidth: true }

      KeyValue {
        pairs: [
          { label: "IP", value: "192.168.1.24" },
          { label: "Gateway", value: "192.168.1.1" },
          { label: "DNS", value: "1.1.1.1" },
        ]
      }
    }

    // An app inspector, as omareel's side panel.
    Card {
      Layout.preferredWidth: 300
      Layout.alignment: Qt.AlignTop

      ColumnLayout {
        Layout.fillWidth: true
        spacing: 0

        Toolbar {
          Layout.fillWidth: true
          divided: false
          IconButton { iconText: section.g(0xF0E2) }
          IconButton { iconText: section.g(0xF01E) }
          trailing: [ Button { text: "Export"; bordered: true } ]
        }

        Tabs {
          Layout.fillWidth: true
          model: ["Shape", "Cursor", "Zoom"]
          currentIndex: 0
        }
      }

      SectionHeader { text: "Frame"; Layout.fillWidth: true }

      Column {
        Layout.fillWidth: true

        SettingRow {
          width: parent.width
          label: "Radius"
          ValueSlider { width: 110; value: 0.4 }
        }
        SettingRow {
          width: parent.width
          label: "Border"
          Dropdown {
            width: 110
            showLabel: false
            value: "solid"
            options: [{ value: "solid", label: "Solid" }, { value: "none", label: "None" }]
          }
        }
        SettingRow {
          width: parent.width
          label: "Shadow"
          hasCursor: true
          ToggleSwitch { checked: true }
        }
      }

      SectionHeader { text: "Background"; Layout.fillWidth: true }

      Column {
        Layout.fillWidth: true

        SettingRow {
          width: parent.width
          label: "Fill"
          ColorWell { swatch: Commons.Color.accent }
        }
        SettingRow {
          width: parent.width
          label: "Padding"
          NumberField { value: 24; from: 0; to: 96 }
        }
      }
    }
  }

  RowLayout {
    spacing: Style.spacing.huge * 2

    // A type-ahead overlay with "term" typed: four results, the second under the cursor, and
    // the detail pane.
    SearchList {
      Layout.preferredWidth: 360
      Layout.preferredHeight: 300
      Layout.alignment: Qt.AlignTop
      query: "term"
      items: section.searchItems
      showDetail: true
      Component.onCompleted: cursor.set("results", 1)
    }

    // Nothing matches "zzz".
    SearchList {
      Layout.preferredWidth: 300
      Layout.preferredHeight: 200
      Layout.alignment: Qt.AlignTop
      query: "zzz"
      items: section.searchItems.filter(function(i) { return matches(i.title, "zzz") })
    }
  }
}
