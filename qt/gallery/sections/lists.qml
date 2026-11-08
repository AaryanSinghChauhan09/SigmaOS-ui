import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  id: section
  name: "lists"
  title: "Lists"

  function g(cp) { return String.fromCodePoint(cp) }

  // Drives the cursor row; the keys work in the live gallery.
  CursorModel {
    id: cursor
    sections: [{ name: "rows", count: 9 }]
    referenceItem: section
    Component.onCompleted: set("rows", 1)
    onMoved: function(name, index, byKey) { if (byKey) scroll.ensureVisible(scroll.contentColumn.children[index]) }
  }

  ScrollArea {
    id: scroll
    Layout.preferredWidth: 420
    maxHeight: 340
    spacing: Style.spacing.xxs
    focus: true
    Keys.onPressed: function(event) { cursor.handleKey(event) }

    ListRow {
      glyph: section.g(0xF0928); title: "Coffee Shop"; subtitle: "WPA2"; value: "5 GHz"
      hasCursor: cursor.isAt("rows", 0)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 0) }
    }
    ListRow {
      glyph: section.g(0xF0925); title: "Basecamp Guest"; subtitle: "Open"
      hasCursor: cursor.isAt("rows", 1)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 1) }
    }
    ListRow {
      glyph: section.g(0xF0928); title: "Basecamp"; status: "Connected"; statusTone: "success"
      current: true
      hasCursor: cursor.isAt("rows", 2)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 2) }
    }
    ListRow {
      glyph: section.g(0xF091F); title: "Neighbor 5G"; subtitle: "Out of range"
      disabled: true
      hasCursor: cursor.isAt("rows", 3)
    }
    ListRow {
      leading: Component { Avatar { name: "Messages"; size: "small" } }
      title: "Messages"; subtitle: "From Basecamp"
      trailing: Component { Badge { text: "3"; tone: "accent"; filled: true } }
      hasCursor: cursor.isAt("rows", 4)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 4) }
    }
    ListRow {
      glyph: section.g(0xF00AF); title: "Headphones"; subtitle: "Battery 80%"
      trailing: Component { ToggleSwitch { checked: true; interactive: false } }
      hasCursor: cursor.isAt("rows", 5)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 5) }
    }
    ListRow {
      glyph: section.g(0xF0922); title: "Office"; subtitle: "Saved"
      actionGlyph: section.g(0xF0159); actionUrgent: true; actionCursor: true
      hasCursor: cursor.isAt("rows", 6)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 6) }
    }
    // Past the cap, so the bar shows.
    ListRow {
      glyph: section.g(0xF0922); title: "Library"; subtitle: "WPA2"
      hasCursor: cursor.isAt("rows", 7)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 7) }
    }
    ListRow {
      glyph: section.g(0xF091F); title: "Airport Free WiFi"; subtitle: "Open"
      hasCursor: cursor.isAt("rows", 8)
      onPointerMoved: function(mouse) { cursor.pointerMoved(this, mouse, "rows", 8) }
    }
  }

  Hero {
    Layout.preferredWidth: 420
    glyph: section.g(0xF0928); title: "Wi-Fi"; status: "Connected to Basecamp"
    trailing: Component { ToggleSwitch { checked: true } }
  }

  Hero {
    Layout.preferredWidth: 420
    glyph: section.g(0xF00B2); title: "Bluetooth"; status: "No adapter found"; error: true
    glyphOpacity: 0.5
  }

  KeyValue {
    pairColumns: 2
    pairs: [
      { label: "Ping", value: "18 ms" },
      { label: "Packet loss", value: "4%", tone: "urgent" },
      { label: "Receiving", value: "1.2 MB/s" },
      { label: "Sending", value: "240 KB/s" },
      { label: "IP address", value: "192.168.1.24" },
      { label: "Gateway", value: "192.168.1.1" },
    ]
  }
}
