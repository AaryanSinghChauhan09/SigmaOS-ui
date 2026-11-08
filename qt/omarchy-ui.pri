# Omarchy UI for Qt Quick. In an app's .pro:
#
#   include(path/to/omarchy-ui/qt/omarchy-ui.pri)
#
# and before loading QML:
#
#   #include "omarchyui.h"
#   OmarchyUi::registerTypes();
#
# Then `import Omarchy.Theme` (Color, Style, Border, Util, I18n, Tokens) and `import Omarchy.Ui`
# (Button, Toggle, TextField, Dropdown, Card, ...) work from any QML file.

QT += quick quickcontrols2 network
CONFIG += c++17

include($$PWD/src/native.pri)

omarchy_ui_qml.files = \
  $$files($$PWD/qml/Omarchy/Theme/*.qml) $$files($$PWD/qml/Omarchy/Theme/*.js) $$PWD/qml/Omarchy/Theme/qmldir \
  $$files($$PWD/qml/Omarchy/Ui/*.qml) $$PWD/qml/Omarchy/Ui/qmldir
omarchy_ui_qml.prefix = /qt/qml
omarchy_ui_qml.base = $$PWD/qml
RESOURCES += omarchy_ui_qml
