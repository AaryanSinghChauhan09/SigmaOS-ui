// The Qt binding's behavior tests: Qt Quick Test over every tst_*.qml in this directory.
//
//   build/qt-tests/omarchy-qt-tests                    every case
//   build/qt-tests/omarchy-qt-tests -functions         list them
//   build/qt-tests/omarchy-qt-tests SearchList::test_escape_clears_then_closes
//
// Offscreen, under the Basic style, in the tokyo-night fixture at radius 0 unless the
// environment says otherwise, so a run doesn't depend on the desktop.

#include <QByteArray>
#include <QObject>
#include <QtQuickTest>

#include "omarchyui.h"

class Setup : public QObject {
  Q_OBJECT

public:
  Setup() {
    // Before the application exists: the platform and style are read when it starts.
    auto preset = [](const char *name, const QByteArray &value) {
      if (!qEnvironmentVariableIsSet(name)) qputenv(name, value);
    };
    preset("QT_QPA_PLATFORM", "offscreen");
    preset("QT_QUICK_CONTROLS_STYLE", "Basic");
    preset("OMARCHY_UI_THEME_DIR", QUICK_TEST_SOURCE_DIR "/../../fixtures/themes/tokyo-night");
    preset("OMARCHY_UI_ROUNDING", "0");
    preset("QT_FORCE_STDERR_LOGGING", "1");
  }

public slots:
  void applicationAvailable() { OmarchyUi::registerTypes(); }
};

QUICK_TEST_MAIN_WITH_SETUP(omarchy_ui, Setup)

#include "main.moc"
