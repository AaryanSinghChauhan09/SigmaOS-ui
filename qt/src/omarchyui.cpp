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

Theme *Theme::instance() {
  static Theme *theme = new Theme();
  return theme;
}

QColor Theme::color(const QJsonValue &token) {
  const QString s = token.toString();
  if (s.size() != 9 || !s.startsWith(QLatin1Char('#'))) return QColor();
  QColor c(s.left(7));
  bool ok = false;
  const int alpha = s.mid(7, 2).toInt(&ok, 16);
  if (!ok || !c.isValid()) return QColor();
  c.setAlpha(alpha);
  return c;
}

void Theme::publish(const QJsonObject &tokens, int revision, bool change) {
  m_tokens = tokens;
  m_revision = revision;
  if (change) emit changed(tokens);
}

}  // namespace OmarchyUi
