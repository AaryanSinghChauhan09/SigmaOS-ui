import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "data"
  title: "Data"

  RowLayout {
    spacing: Style.spacing.huge
    Layout.alignment: Qt.AlignTop

    Table {
      Layout.preferredWidth: 480
      Layout.preferredHeight: implicitHeight
      Layout.alignment: Qt.AlignTop
      columns: [
        { title: "Name" },
        { title: "Kind", width: 90 },
        { title: "Size", width: 70, align: "right" },
        { title: "Modified", width: 100 }
      ]
      rows: [
        ["assets", "Folder", "—", "2026-11-02"],
        ["Cargo.toml", "TOML", "1.2 KB", "2026-11-07"],
        ["README.md", "Markdown", "4.8 KB", "2026-11-05"],
        ["screenshot-of-the-new-settings-panel.png", "PNG image", "1.4 MB", "2026-11-08"],
        ["src", "Folder", "—", "2026-11-08"],
        ["theme.toml", "TOML", "812 B", "2026-10-30"]
      ]
      sortColumn: 0
      sortAscending: true
      selectedRow: 1
      cursorRow: 3
      hasCursor: true
    }

    Tree {
      Layout.preferredWidth: 220
      Layout.preferredHeight: implicitHeight
      Layout.alignment: Qt.AlignTop
      nodes: [
        { id: "root", label: "omarchy-ui", glyph: "󰝰", children: [
          { id: "qt", label: "qt", glyph: "󰝰", children: [
            { id: "qt/button", label: "Button.qml", glyph: "󰈙" },
            { id: "qt/table", label: "Table.qml", glyph: "󰈙" }
          ] },
          { id: "rune", label: "rune", glyph: "󰉋", children: [
            { id: "rune/lib", label: "lib.rs", glyph: "󰈙" }
          ] },
          { id: "spec", label: "spec", glyph: "󰝰", children: [
            { id: "spec/tokens", label: "tokens.md", glyph: "󰈙" }
          ] },
          { id: "readme", label: "README.md", glyph: "󰈙" }
        ] }
      ]
      expanded: ["root", "qt", "spec"]
      selectedId: "qt/table"
      cursorId: "readme"
      hasCursor: true
    }
  }
}
