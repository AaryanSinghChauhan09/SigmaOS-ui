TEMPLATE = app
TARGET = omarchy-qt-gallery
QT += quick quickcontrols2 gui

include(../omarchy-ui.pri)

SOURCES += main.cpp
# Every QML file here and under sections/, so a new section needs no resource list edit.
gallery_qml.files = $$files($$PWD/*.qml) $$files($$PWD/sections/*.qml) $$files($$PWD/assets/*.png)
gallery_qml.prefix = /gallery
gallery_qml.base = $$PWD
RESOURCES += gallery_qml
