# OmarchyWidgets: the Omarchy UI kit for Qt Widgets and KDE Frameworks apps (docs/using-kde.md).
#
#   cmake -S kde -B build/kde -G Ninja && ninja -C build/kde     (kde/build does this)
#
# Tokens come from libomarchy-theme (pkg-config omarchy-theme): `bin/build core` stages it in
# build/ffi, and kde/build points PKG_CONFIG_PATH there. KF6 (KColorScheme, KConfig) is optional:
# with it the one call also keeps KDE's color scheme and Kirigami in step.

cmake_minimum_required(VERSION 3.21)
project(OmarchyWidgets VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(OMARCHY_WIDGETS_KF6 "Theme KDE Frameworks (KColorScheme, Kirigami) too, when KF6 is installed" ON)
option(OMARCHY_WIDGETS_GALLERY "Build the gallery" ON)
option(OMARCHY_WIDGETS_TESTS "Build the tests" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(OMARCHY_THEME REQUIRED IMPORTED_TARGET omarchy-theme)

set(OMARCHY_HAVE_KF6 OFF)
if(OMARCHY_WIDGETS_KF6)
  find_package(KF6ColorScheme QUIET)
  find_package(KF6Config QUIET)
  if(KF6ColorScheme_FOUND AND KF6Config_FOUND)
    set(OMARCHY_HAVE_KF6 ON)
  endif()
endif()
message(STATUS "OmarchyWidgets: KF6 color scheme integration ${OMARCHY_HAVE_KF6}")

file(GLOB OMARCHY_PUBLIC_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/Omarchy/*.h)
file(GLOB OMARCHY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/components/*.h)

add_library(OmarchyWidgets SHARED ${OMARCHY_SOURCES} ${OMARCHY_PUBLIC_HEADERS})
add_library(Omarchy::Widgets ALIAS OmarchyWidgets)
set_target_properties(OmarchyWidgets PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  EXPORT_NAME Widgets)
target_compile_definitions(OmarchyWidgets PRIVATE OMARCHY_WIDGETS_BUILDING)
target_include_directories(OmarchyWidgets
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/OmarchyWidgets>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components)
target_link_libraries(OmarchyWidgets PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets PRIVATE PkgConfig::OMARCHY_THEME)
if(OMARCHY_HAVE_KF6)
  target_link_libraries(OmarchyWidgets PRIVATE KF6::ColorScheme KF6::ConfigCore)
  target_compile_definitions(OmarchyWidgets PRIVATE OMARCHY_WITH_KF6=1)
endif()

# --- install: the library, headers, a CMake package and pkg-config omarchy-widgets ---------------
install(TARGETS OmarchyWidgets EXPORT OmarchyWidgetsTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${OMARCHY_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/OmarchyWidgets/Omarchy)
install(EXPORT OmarchyWidgetsTargets NAMESPACE Omarchy:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OmarchyWidgets)
configure_package_config_file(cmake/OmarchyWidgetsConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/OmarchyWidgetsConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OmarchyWidgets)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/OmarchyWidgetsConfigVersion.cmake
  COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/OmarchyWidgetsConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/OmarchyWidgetsConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/OmarchyWidgets)
configure_file(cmake/omarchy-widgets.pc.in ${CMAKE_CURRENT_BINARY_DIR}/omarchy-widgets.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/omarchy-widgets.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# A package config usable straight from the build tree (find_package with
# OmarchyWidgets_DIR=build/kde), so examples build against a checkout without installing.
export(EXPORT OmarchyWidgetsTargets NAMESPACE Omarchy:: FILE ${CMAKE_CURRENT_BINARY_DIR}/OmarchyWidgetsTargets.cmake)

# --- the gallery, tests and live-switch test -----------------------------------------------------
if(OMARCHY_WIDGETS_GALLERY)
  file(GLOB GALLERY_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gallery/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/gallery/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/gallery/sections/*.cpp)
  add_executable(omarchy-widgets-gallery ${GALLERY_SOURCES} gallery/assets.qrc)
  target_include_directories(omarchy-widgets-gallery PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gallery)
  target_link_libraries(omarchy-widgets-gallery PRIVATE OmarchyWidgets)
  install(TARGETS omarchy-widgets-gallery RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if(OMARCHY_WIDGETS_TESTS)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  file(GLOB TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/tst_*.cpp)
  add_executable(omarchy-widgets-tests tests/main.cpp ${TEST_SOURCES})
  target_include_directories(omarchy-widgets-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_SOURCE_DIR}/src/components)
  target_link_libraries(omarchy-widgets-tests PRIVATE OmarchyWidgets Qt6::Test)
  target_compile_definitions(omarchy-widgets-tests PRIVATE OMARCHY_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

  add_executable(omarchy-widgets-live-test tests/live.cpp)
  target_link_libraries(omarchy-widgets-live-test PRIVATE OmarchyWidgets Qt6::Test)
  if(OMARCHY_HAVE_KF6)
    target_link_libraries(omarchy-widgets-live-test PRIVATE KF6::ColorScheme KF6::ConfigCore)
    target_compile_definitions(omarchy-widgets-live-test PRIVATE OMARCHY_WITH_KF6=1)
  endif()
endif()
