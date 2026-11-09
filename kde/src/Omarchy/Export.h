#pragma once

#if defined(OMARCHY_WIDGETS_BUILDING)
#define OMARCHY_EXPORT __attribute__((visibility("default")))
#else
#define OMARCHY_EXPORT
#endif
