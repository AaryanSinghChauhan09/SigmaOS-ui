// The installed QML plugin behind `import Omarchy.Theme.Native` (qt/module). The Omarchy.Theme
// singletons import that module, so the engine loads this plugin the first time a QML file
// imports Omarchy.Theme or Omarchy.Ui from the installed modules, and the native types come from
// the same OmarchyUi::registerTypes() a vendoring app calls itself.
//
// A QQmlExtensionPlugin rather than a QQmlEngineExtensionPlugin: the engine checks the module has
// types as soon as the plugin loads, before initializeEngine() runs, so they must be registered
// here. This runs on the type loader's thread; registerTypes() only registers factories, and the
// singletons are made on the engine's thread when QML first uses them.

#include <QQmlExtensionPlugin>

#include "omarchyui.h"

class OmarchyUiPlugin : public QQmlExtensionPlugin {
  Q_OBJECT
  Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
  void registerTypes(const char *) override { OmarchyUi::registerTypes(); }
};

#include "omarchyuiplugin.moc"
