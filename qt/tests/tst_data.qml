import QtQuick
import QtTest
import Omarchy.Ui as O

// Table and Tree, as the Rune gallery's data_selects_and_folds and data_keys_as_in_qt tests drive
// Rune's: a press selects a row or folds a branch, a header press asks to sort, and the keys move
// the cursor, choose its row, and (in the tree) fold and unfold.
TestCase {
  id: testCase
  name: "Data"
  width: 600
  height: 500
  visible: true
  when: windowShown

  Component {
    id: tableComponent
    O.Table {
      id: table
      width: 480
      height: implicitHeight
      columns: [{ title: "Name" }, { title: "Kind", width: 90 }, { title: "Size", width: 70, align: "right" }]
      rows: [["assets", "Folder", "—"], ["Cargo.toml", "TOML", "1.2 KB"], ["README.md", "Markdown", "4.8 KB"], ["src", "Folder", "—"], ["target", "Folder", "—"]]
      sortColumn: 0
      onSortRequested: function(column) {
        if (column === sortColumn) sortAscending = !sortAscending
        else { sortColumn = column; sortAscending = true }
      }
    }
  }

  // The app owns `expanded` and `selectedId`, as the gallery's tree does.
  Component {
    id: treeComponent
    O.Tree {
      id: tree
      width: 220
      height: implicitHeight
      expanded: ["root", "qt", "spec"]
      nodes: [
        { id: "root", label: "omarchy-ui", children: [
          { id: "qt", label: "qt", children: [{ id: "qt/button", label: "Button.qml" }, { id: "qt/table", label: "Table.qml" }] },
          { id: "rune", label: "rune", children: [{ id: "rune/lib", label: "lib.rs" }] },
          { id: "spec", label: "spec", children: [{ id: "spec/tokens", label: "tokens.md" }] }
        ] }
      ]
      onToggled: function(id) {
        var i = expanded.indexOf(id)
        var next = expanded.slice()
        if (i >= 0) next.splice(i, 1)
        else next.push(id)
        expanded = next
      }
      onActivated: function(id) { selectedId = id }
    }
  }

  SignalSpy { id: rowActivated; signalName: "rowActivated" }

  function test_table_selects_on_click_and_sorts_on_header() {
    var table = createTemporaryObject(tableComponent, testCase)
    var cell = findText(table, "src")
    mouseClick(cell)
    compare(table.selectedRow, 3)
    mouseClick(findText(table, "Name"))
    verify(!table.sortAscending, "the sorted column's header flips the order")
    mouseClick(findText(table, "Kind"))
    compare(table.sortColumn, 1)
    verify(table.sortAscending)
  }

  function test_table_keys_move_the_cursor_and_choose() {
    var table = createTemporaryObject(tableComponent, testCase)
    rowActivated.clear()
    rowActivated.target = table
    table.forceActiveFocus()
    keyClick(Qt.Key_Down)
    compare(table.cursorRow, 0, "the first key lands on the first row")
    keyClick(Qt.Key_Down)
    keyClick(Qt.Key_Return)
    compare(table.selectedRow, 1)
    keyClick(Qt.Key_End)
    keyClick(Qt.Key_Down)
    compare(table.cursorRow, 4, "stops at the last row")
    keyClick(Qt.Key_Space)
    compare(table.selectedRow, 4)
    keyClick(Qt.Key_Home)
    keyClick(Qt.Key_Up)
    compare(table.cursorRow, 0, "stops at the first row")
    keyClick(Qt.Key_Enter)
    compare(table.selectedRow, 0)
    compare(rowActivated.count, 3)
  }

  function test_tree_folds_and_selects_on_click() {
    var tree = createTemporaryObject(treeComponent, testCase)
    var qt = findText(tree, "qt")
    // The chevron sits in the slot before the label.
    mouseClick(qt, -qt.x / 2 + 4, qt.height / 2)
    verify(tree.expanded.indexOf("qt") < 0, "the chevron folds qt")
    verify(!findText(tree, "Button.qml"))
    mouseClick(findText(tree, "tokens.md"))
    compare(tree.selectedId, "spec/tokens")
  }

  function test_tree_keys_fold_unfold_and_choose() {
    var tree = createTemporaryObject(treeComponent, testCase)
    tree.forceActiveFocus()
    keyClick(Qt.Key_Down)
    compare(tree.cursorId, "root")
    keyClick(Qt.Key_Down)
    compare(tree.cursorId, "qt")
    keyClick(Qt.Key_Left)
    verify(tree.expanded.indexOf("qt") < 0, "Left folds an open branch")
    keyClick(Qt.Key_Left)
    verify(tree.expanded.indexOf("qt") < 0, "and does nothing to a folded one")
    keyClick(Qt.Key_Right)
    verify(tree.expanded.indexOf("qt") >= 0, "Right unfolds it")
    keyClick(Qt.Key_Down)
    compare(tree.cursorId, "qt/button")
    keyClick(Qt.Key_Right)
    compare(tree.expanded.length, 3, "a leaf doesn't fold")
    keyClick(Qt.Key_Return)
    compare(tree.selectedId, "qt/button")
  }

  // The visible Text item showing exactly `text` under `item`, or null.
  function findText(item, text) {
    if (item.visible === false) return null
    if (item.text === text && item.hasOwnProperty("textFormat")) return item
    for (var i = 0; i < item.children.length; i++) {
      var hit = findText(item.children[i], text)
      if (hit) return hit
    }
    return null
  }
}
