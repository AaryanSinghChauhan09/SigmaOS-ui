#include "omarchyui.h"

#include <QtQml>

#include "themesource.h"
#include "iconlookup.h"

namespace OmarchyUi {

void registerTypes() {
  qmlRegisterSingletonInstance("Omarchy.Theme.Native", 1, 0, "ThemeSource", ThemeSource::instance());
  qmlRegisterSingletonInstance("Omarchy.Theme.Native", 1, 0, "IconLookup", IconLookup::instance());
}

}  // namespace OmarchyUi
