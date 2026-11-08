#include "omarchyui.h"

#include <QtQml>

#include "themesource.h"
#include "iconlookup.h"

namespace OmarchyUi {

// The process-wide instance, handed to every engine that asks and owned by none of them. Made on
// first use, so on the engine's thread even when registerTypes() runs on the type loader's (the
// installed plugin).
template <typename T>
static QObject *shared(QQmlEngine *, QJSEngine *) {
  T *object = T::instance();
  QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
  return object;
}

void registerTypes() {
  // Once, however many times it is called.
  static bool registered = false;
  if (registered) return;
  registered = true;
  qmlRegisterSingletonType<ThemeSource>("Omarchy.Theme.Native", 1, 0, "ThemeSource", shared<ThemeSource>);
  qmlRegisterSingletonType<IconLookup>("Omarchy.Theme.Native", 1, 0, "IconLookup", shared<IconLookup>);
}

}  // namespace OmarchyUi
