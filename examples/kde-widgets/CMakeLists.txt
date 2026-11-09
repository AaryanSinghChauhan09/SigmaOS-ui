# A tiny Omarchy panel in Qt Widgets, following the live theme (docs/using-kde.md): the kit's
# widgets beside a stock Qt widget, after one call.
#
#   cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/omarchy-widgets/prefix && cmake --build build
#   build/omarchy-widgets-example                      a window; switch themes while it runs
#   build/omarchy-widgets-example --snap out.png       render once, offscreen, at 2x
#
# Or with pkg-config: c++ -std=c++17 -fPIC main.cpp $(pkg-config --cflags --libs omarchy-widgets)

cmake_minimum_required(VERSION 3.21)
project(omarchy-widgets-example LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(OmarchyWidgets REQUIRED)

add_executable(omarchy-widgets-example main.cpp)
target_link_libraries(omarchy-widgets-example PRIVATE Omarchy::Widgets)
