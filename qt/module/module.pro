# Omarchy UI as installed QML modules, so `import Omarchy.Theme` and `import Omarchy.Ui` work in
# any QML app (the qml tool, PySide, Quickshell, a C++ app linking nothing extra):
#
#   qmake6 qt/module/module.pro && make && make install INSTALL_ROOT=/tmp/stage
#
# installs into $$[QT_INSTALL_QML]/Omarchy (pass QML_DIR=... to qmake6 to install elsewhere):
#
#   Omarchy/Theme/         qmldir, the theme singletons and their .js
#   Omarchy/Theme/Native/  qmldir + libomarchyuiplugin.so (ThemeSource, IconLookup)
#   Omarchy/Ui/            qmldir, the components
#
# The plugin is the same C++ as the vendored route (../omarchy-ui.pri), registered through the
# same OmarchyUi::registerTypes().

TEMPLATE = lib
TARGET = omarchyuiplugin
CONFIG += plugin qt
QT += qml

include($$PWD/../src/native.pri)
SOURCES += $$PWD/../src/omarchyuiplugin.cpp

isEmpty(QML_DIR): QML_DIR = $$[QT_INSTALL_QML]
OMARCHY = $$QML_DIR/Omarchy
SRC = $$PWD/../qml/Omarchy

target.path = $$OMARCHY/Theme/Native
native.files = $$PWD/qmldir
native.path = $$OMARCHY/Theme/Native
theme.files = $$SRC/Theme/qmldir $$files($$SRC/Theme/*.qml) $$files($$SRC/Theme/*.js)
theme.path = $$OMARCHY/Theme
ui.files = $$SRC/Ui/qmldir $$files($$SRC/Ui/*.qml)
ui.path = $$OMARCHY/Ui
INSTALLS += target native theme ui
