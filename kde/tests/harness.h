#pragma once

// The behavior tests: each tst_*.cpp defines QObject test classes (QTest slots) and registers
// them with OMARCHY_TEST(Class); main.cpp runs every registered class, offscreen, under a fixture
// theme, and fails if any fails. Mirrors qt/tests (keys and clicks on the kit's components).

#include <QList>
#include <QObject>
#include <QTest>
#include <functional>

namespace Harness {

struct Entry {
  const char *name;
  std::function<QObject *()> make;
};
QList<Entry> &registry();

template <typename T>
struct Registrar {
  explicit Registrar(const char *name) { registry().append({name, [] { return new T; }}); }
};

// Show a widget offscreen and wait until it's exposed and laid out.
void show(QWidget *w, QSize size = QSize());

}  // namespace Harness

#define OMARCHY_TEST(Class) static Harness::Registrar<Class> s_register_##Class(#Class);
