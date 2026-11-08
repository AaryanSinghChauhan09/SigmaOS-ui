#include "omarchyui.h"

#include <QtQml>

#include "themesource.h"

namespace OmarchyUi {

void registerTypes() {
  qmlRegisterSingletonInstance("Omarchy.Theme.Native", 1, 0, "ThemeSource", ThemeSource::instance());
}

}  // namespace OmarchyUi
