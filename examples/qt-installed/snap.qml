// Render a QML file whose root is a Window to a PNG and quit, with nothing but the qml tool:
//
//   QT_QPA_PLATFORM=offscreen QT_QUICK_CONTROLS_STYLE=Basic QT_SCALE_FACTOR=2 \
//     qml examples/qt-installed/snap.qml -- "$PWD/examples/qt-installed/main.qml" "$PWD/out.png"
//
// Both paths absolute. The window's color is painted behind its content, as the window shows it.

import QtQuick

QtObject {
  id: snap

  readonly property var args: {
    const all = Qt.application.arguments
    const at = all.indexOf("--")
    return at < 0 ? [] : all.slice(at + 1)
  }
  property var window: null

  function fail(message) {
    console.error("snap: " + message)
    Qt.exit(1)
  }

  property Timer settle: Timer {
    interval: 600
    onTriggered: {
      snap.window.contentItem.grabToImage(function(result) {
        if (!result.saveToFile(snap.args[1])) snap.fail("could not write " + snap.args[1])
        else Qt.exit(0)
      })
    }
  }

  Component.onCompleted: {
    if (args.length < 2) return fail("usage: qml snap.qml -- /abs/view.qml /abs/out.png")
    const component = Qt.createComponent("file://" + args[0])
    if (component.status !== Component.Ready) return fail(component.errorString())
    window = component.createObject(null)
    if (!window || !window.contentItem) return fail(args[0] + " has no Window at its root")
    Qt.createQmlObject("import QtQuick; Rectangle { anchors.fill: parent; z: -1 }", window.contentItem).color =
      Qt.binding(function() { return snap.window.color })
    settle.start()
  }
}
