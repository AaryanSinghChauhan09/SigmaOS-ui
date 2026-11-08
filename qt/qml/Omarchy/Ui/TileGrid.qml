import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A grid of tiles, each a glyph (font.display) or an Avatar above a label of up to two lines,
// elided: the shell's emoji picker and app grid. The cursor tile takes the hover-cursor look
// and the current one the selected fill, as ListRow's do (CursorSurface).
//
//   TileGrid {
//     columns: 4
//     tiles: [{ glyph: "", label: "Terminal" }, { name: "Photos", source: "file:///…" }]
//     cursorIndex: cursor.cursorActive ? cursor.selectedIndex : -1
//     onActivated: function(index) { launch(index) }
//     onPointerMoved: function(index, item, mouse) { cursor.pointerMoved(item, mouse, "", index) }
//   }
//
// A tile is { label, glyph } or { label, name, source, glyph } for an Avatar (medium).
Grid {
  id: root

  property var tiles: []
  property int cursorIndex: -1
  property int currentIndex: -1
  property real cellWidth: Style.space(88)

  signal activated(int index)
  signal pointerMoved(int index, var item, var mouse)

  readonly property real tilePadding: Style.spacing.md
  readonly property real iconHeight: Math.max(glyphProbe.implicitHeight, Style.font.displayLarge)
  readonly property real labelHeight: labelProbe.implicitHeight
  readonly property real cellHeight: tilePadding * 2 + iconHeight + Style.spacing.md + labelHeight

  columns: 4

  // Line heights for the icon box and two label lines, so every tile is the same height.
  Text { id: glyphProbe; visible: false; text: "x"; font.family: Style.font.family; font.pixelSize: Style.font.display }
  Text { id: labelProbe; visible: false; text: "x\nx"; font.family: Style.font.family; font.pixelSize: Style.font.bodySmall }

  Repeater {
    model: root.tiles.length
    delegate: CursorSurface {
      id: tile
      required property int index
      readonly property var entry: root.tiles[index] || ({})
      readonly property bool avatared: entry.source !== undefined || entry.name !== undefined

      width: root.cellWidth
      height: root.cellHeight
      hasCursor: root.cursorIndex === index
      current: root.currentIndex === index

      Item {
        id: iconBox
        x: 0
        y: root.tilePadding
        width: parent.width
        height: root.iconHeight

        Text {
          visible: !tile.avatared
          anchors.centerIn: parent
          textFormat: Text.PlainText
          text: tile.entry.glyph || ""
          color: Commons.Color.foreground
          font.family: Style.font.family
          font.pixelSize: Style.font.display
        }

        Avatar {
          visible: tile.avatared
          anchors.centerIn: parent
          size: "medium"
          name: tile.entry.name || tile.entry.label || ""
          source: tile.entry.source || ""
          glyph: tile.avatared ? (tile.entry.glyph || "") : ""
        }
      }

      Text {
        x: root.tilePadding
        y: iconBox.y + iconBox.height + Style.spacing.md
        width: parent.width - root.tilePadding * 2
        textFormat: Text.PlainText
        text: tile.entry.label || ""
        color: Commons.Color.foreground
        font.family: Style.font.family
        font.pixelSize: Style.font.bodySmall
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        maximumLineCount: 2
        elide: Text.ElideRight
      }

      MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onPositionChanged: function(mouse) { root.pointerMoved(tile.index, tile, mouse) }
        onClicked: root.activated(tile.index)
      }
    }
  }
}
