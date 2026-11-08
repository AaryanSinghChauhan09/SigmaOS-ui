#pragma once

namespace OmarchyUi {

// Registers the native theme source as `import Omarchy.Theme.Native` (used by the Omarchy.Theme
// singletons). Call once, before the QML engine loads anything.
void registerTypes();

}  // namespace OmarchyUi
