# The kit's C++ side (ThemeSource, IconLookup, OmarchyUi::registerTypes), shared by the vendored
# route (../omarchy-ui.pri) and the installed QML plugin (../module/module.pro).

QT += qml network
CONFIG += c++17

INCLUDEPATH += $$PWD
HEADERS += $$PWD/themesource.h $$PWD/iconlookup.h $$PWD/omarchyui.h
SOURCES += $$PWD/themesource.cpp $$PWD/iconlookup.cpp $$PWD/omarchyui.cpp
