import QtQuick
import Omarchy.Theme

// A Menu that opens at a point (the pointer) instead of under a trigger: `popup(x, y)` in the
// parent's coordinates, kept inside the parent. Give it a `target` and right-clicks there open it
// at the pointer.
//
//   ContextMenu {
//     parent: window.contentItem
//     target: canvas
//     MenuItem { text: "Copy"; shortcut: "Ctrl+C" }
//   }
Menu {
  id: root

  property Item target: null
  property Item _catcher: null

  onTargetChanged: {
    if (_catcher) _catcher.destroy()
    _catcher = target ? catcher.createObject(target) : null
  }

  Component {
    id: catcher
    MouseArea {
      anchors.fill: parent
      acceptedButtons: Qt.RightButton
      onClicked: function(mouse) {
        var p = mapToItem(root.parent, mouse.x, mouse.y)
        root.popup(p.x, p.y)
      }
    }
  }
}
