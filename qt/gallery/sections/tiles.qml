import QtQuick
import QtQuick.Layouts
import Omarchy.Theme
import Omarchy.Theme as Commons
import Omarchy.Ui
import ".."

Section {
  id: section
  name: "tiles"
  title: "Tiles"

  function g(cp) { return String.fromCodePoint(cp) }
  readonly property string photo: "qrc:/gallery/assets/avatar.png"

  RowLayout {
    spacing: Style.spacing.controlGap
    Badge { text: "New" }
    Badge { text: "3"; tone: "accent"; filled: true }
    Badge { text: "12"; tone: "urgent"; filled: true }
    Badge { text: "Online"; tone: "success" }
    Badge { text: "Low"; tone: "warning" }
    Badge { text: "Offline"; tone: "urgent" }
  }

  RowLayout {
    spacing: Style.spacing.xxl
    StatusDot { tone: "success" }
    StatusDot { tone: "warning" }
    StatusDot { tone: "urgent" }
    StatusDot { tone: "muted" }
    StatusDot { tone: "accent" }
    Avatar { name: "Ryan"; round: true; status: "success"; ground: Commons.Color.background }
    Avatar { name: "Photo"; source: section.photo; status: "warning"; ground: Commons.Color.background }
    Avatar { name: "Server"; glyph: section.g(0xF048B); status: "urgent"; ground: Commons.Color.background }
  }

  RowLayout {
    spacing: Style.spacing.xxl
    Avatar { name: "ada"; size: "small" }
    Avatar { name: "Basecamp" }
    Avatar { name: "Omarchy"; size: "large"; round: true }
    Avatar { name: "Robot"; glyph: section.g(0xF06A9) }
    Avatar { name: "Photo"; source: section.photo; size: "small" }
    Avatar { name: "Photo"; source: section.photo; round: true }
    Avatar { name: "Photo"; source: section.photo; size: "large" }
    Avatar { name: "Missing"; source: "qrc:/gallery/assets/missing.png" }
  }

  TileGrid {
    columns: 4
    cursorIndex: 2
    tiles: [
      { glyph: section.g(0xF120), label: "Terminal" },
      { glyph: section.g(0xF07B), label: "Files" },
      { glyph: section.g(0xF0AC), label: "Browser" },
      { glyph: section.g(0xF001), label: "Music" },
      { glyph: section.g(0xF013), label: "Settings" },
      { glyph: section.g(0xF073), label: "Calendar" },
      { glyph: section.g(0xF03D), label: "Screen Recorder" },
      { name: "Photos", source: section.photo, label: "Photos" },
    ]
  }
}
