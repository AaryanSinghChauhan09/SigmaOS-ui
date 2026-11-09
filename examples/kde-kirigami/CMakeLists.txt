# A Kirigami app that follows the Omarchy theme through OmarchyWidgets' one call
# (docs/using-kde.md): KDE's desktop Qt Quick Controls style draws its controls with
# Omarchy::Style, and Kirigami's colors come from the generated KDE color scheme.
#
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/omarchy-widgets/prefix && cmake --build build
#   build/omarchy-kirigami-example                      a window, following the desktop
#   build/omarchy-kirigami-example --snap out.png       render once, offscreen, at 2x

cmake_minimum_required(VERSION 3.21)
project(omarchy-kirigami-example LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Quick)
find_package(OmarchyWidgets REQUIRED)

add_executable(omarchy-kirigami-example main.cpp main.qrc)
target_link_libraries(omarchy-kirigami-example PRIVATE Omarchy::Widgets Qt6::Widgets Qt6::Quick)
