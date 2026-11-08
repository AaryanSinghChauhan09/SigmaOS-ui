# Qt Quick Test cases for the Qt binding's behavior: every tst_*.qml here, run against the kit
# with a fixture theme (main.cpp). bin/build builds it, bin/test runs it.
TEMPLATE = app
TARGET = omarchy-qt-tests
QT += quick quickcontrols2 qmltest
CONFIG += console

include(../omarchy-ui.pri)

SOURCES += main.cpp
DEFINES += QUICK_TEST_SOURCE_DIR=\\\"$$PWD\\\"
OTHER_FILES += $$files($$PWD/tst_*.qml)
