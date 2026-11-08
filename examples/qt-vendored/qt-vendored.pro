# A Qt Quick app that vendors Omarchy UI: the kit's QML and C++ compile into the binary, so it
# runs with nothing installed. In an app's own repository the path is wherever the checkout or
# submodule lives (third_party/omarchy-ui, ...).
#
#   mkdir -p build && cd build && qmake6 ../qt-vendored.pro && make
#   ./omarchy-ui-example-vendored [--snap out.png]

TEMPLATE = app
TARGET = omarchy-ui-example-vendored
QT += quick

include(../../qt/omarchy-ui.pri)

SOURCES += main.cpp
app_qml.files = Main.qml
app_qml.prefix = /example
RESOURCES += app_qml
