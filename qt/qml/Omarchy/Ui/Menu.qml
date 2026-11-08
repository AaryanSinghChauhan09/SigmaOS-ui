import QtQuick
import Omarchy.Theme
import Omarchy.Theme as Commons

// A menu: MenuItems, MenuSeparators and MenuSections on the menu surface ([menu] background and
// border, Hyprland's rounding), sm inside its border. It anchors below its `trigger` when
// opened; ContextMenu opens one at a point instead.
//
//   Button { id: fileButton; text: "File"; onClicked: fileMenu.open() }
//   Menu {
//     id: fileMenu
//     parent: window.contentItem          // something covering the window, so it floats
//     trigger: fileButton
//     MenuItem { text: "Open…"; shortcut: "Ctrl+O"; onTriggered: openDialog() }
//     MenuSeparator {}
//     MenuItem { text: "Export"; submenu: Menu { MenuItem { text: "PDF" } } }
//   }
//
// Keyboard (while open it holds focus): Up/Down or k/j move the cursor over enabled rows and
// wrap, Home/End jump, Enter or Space runs the row, Right or l opens its submenu, Left or h
// goes back out of a submenu, Esc closes. A press outside closes it. `held` draws it open in
// place with no positioning or dismissal, for galleries and still renders.
BorderSurface {
  id: root

  property bool opened: false
  property bool held: false
  property Item trigger: null
  // Distance from the trigger.
  property real gap: Style.spacing.xs
  property real minimumWidth: Style.space(160)
  // The keyboard cursor, an index into `rows` (-1: none).
  property int currentIndex: -1
  // Set while this menu is open as another's submenu.
  property Item parentMenu: null
  default property alias items: column.data

  // The MenuItems, in order: what the cursor walks.
  readonly property var rows: {
    var out = []
    var kids = column.children
    for (var i = 0; i < kids.length; ++i)
      if (kids[i].isMenuItem === true && kids[i].visible) out.push(kids[i])
    return out
  }
  readonly property Item currentItem: currentIndex >= 0 && currentIndex < rows.length ? rows[currentIndex] : null
  readonly property bool hasIcons: rows.some(function(r) { return r.iconText !== "" })
  readonly property bool hasMarks: rows.some(function(r) { return r.checkable || r.hasSubmenu })
  readonly property real _widest: {
    var w = 0
    var kids = column.children
    for (var i = 0; i < kids.length; ++i)
      if (kids[i].visible) w = Math.max(w, kids[i].implicitWidth)
    return w
  }

  // A row ran (in this menu or a submenu); the menu has closed itself unless held.
  signal triggered(Item item)
  signal closed()

  function open() {
    show()
    if (trigger && parent) {
      var p = trigger.mapToItem(parent, 0, trigger.height + gap)
      place(p.x, p.y)
    }
  }

  // Opens with the top-left corner at (px, py) in the parent's coordinates, kept inside it.
  function popup(px, py) {
    show()
    place(px, py)
  }

  function place(px, py) {
    column.forceLayout()
    var w = parent ? parent.width : 0
    var h = parent ? parent.height : 0
    x = Math.max(0, Math.min(px, w - implicitWidth))
    y = Math.max(0, Math.min(py, h - implicitHeight))
  }

  function show() {
    currentIndex = -1
    opened = true
    column.forceLayout()
    forceActiveFocus()
  }

  function close() {
    closeSubmenus()
    if (!opened) return
    opened = false
    currentIndex = -1
    closed()
  }

  function closeSubmenus() {
    for (var i = 0; i < rows.length; ++i)
      if (rows[i].submenu && rows[i].submenu.opened) rows[i].submenu.close()
  }

  function closeAll() {
    var m = root
    while (m.parentMenu) m = m.parentMenu
    if (!m.held) {
      m.close()
    } else {
      // A held menu stays: the keys come back to it from the submenu that just closed.
      m.closeSubmenus()
      m.forceActiveFocus()
    }
  }

  function move(step) {
    var n = rows.length
    if (n === 0) return
    var i = currentIndex < 0 ? (step > 0 ? -1 : n) : currentIndex
    for (var tries = 0; tries < n; ++tries) {
      i = (i + step + n) % n
      if (rows[i].enabled) { currentIndex = i; return }
    }
  }

  function jump(toEnd) {
    currentIndex = -1
    if (toEnd) move(-1)
    else move(1)
  }

  function openSubmenu(item) {
    var sub = item ? item.submenu : null
    if (!sub) return
    closeSubmenus()
    currentIndex = rows.indexOf(item)
    sub.parentMenu = root
    sub.parent = root.parent
    sub.show()
    column.forceLayout()
    var p = item.mapToItem(root.parent, item.width + root.contentRightInset, -sub.contentTopInset)
    sub.place(p.x, p.y)
    sub.move(1)
  }

  function itemTriggered(item) {
    var m = root
    while (m.parentMenu) m = m.parentMenu
    m.triggered(item)
    closeAll()
  }

  // Handles a key for the menu; true when it was the menu's.
  function handleKey(event) {
    var t = event.text
    if (event.key === Qt.Key_Down || t === "j") { move(1); return true }
    if (event.key === Qt.Key_Up || t === "k") { move(-1); return true }
    if (event.key === Qt.Key_Home) { jump(false); return true }
    if (event.key === Qt.Key_End) { jump(true); return true }
    if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
      if (currentItem) currentItem.activate()
      return true
    }
    if (event.key === Qt.Key_Right || t === "l") {
      if (currentItem && currentItem.hasSubmenu) openSubmenu(currentItem)
      return true
    }
    if (event.key === Qt.Key_Left || t === "h" || event.key === Qt.Key_Escape) {
      if (parentMenu) {
        var back = parentMenu
        close()
        back.forceActiveFocus()
      } else if (event.key === Qt.Key_Escape) {
        close()
      }
      return true
    }
    return false
  }

  Keys.onPressed: function(event) { event.accepted = root.handleKey(event) }

  visible: opened || held
  z: held ? 0 : 1000
  color: Commons.Color.menu.background
  borderSpec: Border.surfaceSpec("menu", "border", Commons.Color.menu.border, Math.max(1, Style.space(2)))
  radius: Style.cornerRadius
  padding: Style.spacing.sm
  implicitWidth: Math.max(minimumWidth, _widest) + contentLeftInset + contentRightInset
  implicitHeight: column.implicitHeight + contentTopInset + contentBottomInset

  // Presses outside an open (not held) top-level menu close it.
  MouseArea {
    readonly property bool active: root.opened && !root.held && root.parentMenu === null
    parent: active ? root.parent : root
    anchors.fill: parent
    z: root.z - 1
    enabled: active
    visible: active
    acceptedButtons: Qt.AllButtons
    onPressed: root.close()
  }

  Column {
    id: column
    readonly property Item menu: root
    x: root.contentLeftInset
    y: root.contentTopInset
    width: root.width - root.contentLeftInset - root.contentRightInset
  }
}
