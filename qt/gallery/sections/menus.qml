import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  name: "menus"
  title: "Menus"

  RowLayout {
    spacing: Style.spacing.huge
    Layout.alignment: Qt.AlignTop

    // A menu held open: section labels, leading glyphs, shortcut hints, a check, a disabled
    // row, separators, a submenu row, and the keyboard cursor on "Open…".
    Menu {
      held: true
      Layout.alignment: Qt.AlignTop
      MenuSection { text: "File" }
      MenuItem { text: "New"; iconText: ""; shortcut: "Ctrl+N" }
      MenuItem { text: "Open…"; iconText: ""; shortcut: "Ctrl+O"; hasCursor: true }
      MenuItem { text: "Save"; iconText: ""; shortcut: "Ctrl+S" }
      MenuItem { text: "Duplicate"; iconText: ""; enabled: false }
      MenuSeparator {}
      MenuSection { text: "View" }
      MenuItem { text: "Sidebar"; checkable: true; checked: true }
      MenuItem { text: "Word wrap"; checkable: true }
      MenuSeparator {}
      MenuItem { text: "Export"; iconText: ""; hasSubmenu: true }
    }

    // A context menu held open: radio choices and a destructive row.
    ContextMenu {
      held: true
      Layout.alignment: Qt.AlignTop
      MenuItem { text: "Cut"; shortcut: "Ctrl+X" }
      MenuItem { text: "Copy"; shortcut: "Ctrl+C" }
      MenuItem { text: "Paste"; shortcut: "Ctrl+V" }
      MenuSeparator {}
      MenuItem { text: "By name"; checkable: true; exclusive: true; checked: true }
      MenuItem { text: "By date"; checkable: true; exclusive: true }
      MenuSeparator {}
      MenuItem { text: "Delete"; shortcut: "Del"; destructive: true }
    }

    // A tooltip with its shortcut, held open.
    Item {
      Layout.alignment: Qt.AlignTop
      implicitWidth: tip.implicitWidth
      implicitHeight: tip.implicitHeight
      Tooltip { id: tip; held: true; text: "Save"; shortcut: "Ctrl+S" }
    }
  }

  RowLayout {
    spacing: Style.spacing.lg
    Repeater {
      model: ["Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K"]
      delegate: Keycap { required property string modelData; key: modelData }
    }
    KeyCombo { keys: "Ctrl+Shift+P" }
  }

  ShortcutSheet {
    groups: [
      { title: "File", keys: [["Ctrl+N", "New file"], ["Ctrl+O", "Open"], ["Ctrl+S", "Save"]] },
      { title: "View", keys: [["Ctrl+B", "Sidebar"], ["Ctrl+Shift+P", "Commands"], ["F11", "Full screen"]] },
    ]
  }
}
