import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// Dim labels beside their values, aligned in a grid: the details block the power, dropbox and
// network panels each define as InfoLabel/InfoValue. `columns` pairs sit on each line (the
// network panel shows two).
//
//   KeyValue {
//     columns: 2
//     pairs: [{ label: "IP address", value: "192.168.1.24" },
//             { label: "Packet loss", value: "4%", tone: "urgent" }]
//   }
//
// A pair's `tone` ("accent", "success", "warning", "urgent") tints its value.
Grid {
  id: root

  property var pairs: []
  property int pairColumns: 1
  property color foreground: Commons.Color.foreground

  function toneColor(tone) {
    switch (tone) {
    case "accent": return Commons.Color.accent
    case "success": return Commons.Color.success
    case "warning": return Commons.Color.warning
    case "urgent": return Commons.Color.urgent
    }
    return root.foreground
  }

  columns: Math.max(1, pairColumns) * 2
  columnSpacing: Style.space(20)
  rowSpacing: Style.spacing.labelGap

  Repeater {
    model: root.pairs.length * 2
    delegate: Text {
      required property int index
      readonly property var pair: root.pairs[Math.floor(index / 2)]
      readonly property bool isLabel: index % 2 === 0
      textFormat: Text.PlainText
      text: pair ? (isLabel ? pair.label : pair.value) : ""
      color: isLabel ? Commons.Color.textDim : root.toneColor(pair ? pair.tone : "")
      font.family: Style.font.family
      font.pixelSize: Style.font.bodySmall
    }
  }
}
