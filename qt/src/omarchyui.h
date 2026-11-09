#pragma once

#include <QColor>
#include <QJsonObject>
#include <QJsonValue>
#include <QObject>

class ThemeSource;

namespace OmarchyUi {

// Registers the native theme source as `import Omarchy.Theme.Native` (used by the Omarchy.Theme
// singletons). Call once, before the QML engine loads anything.
void registerTypes();

// The theme's changed hook for C++ (spec/following.md), for what QML bindings can't reach: a
// QQuickPaintedItem or scene-graph renderer, a cache, a child process. The same tokens and the
// same moment as QML's `Tokens.changed`:
//
//   QObject::connect(OmarchyUi::Theme::instance(), &OmarchyUi::Theme::changed, renderer,
//                    [renderer](const QJsonObject &tokens) {
//                      renderer->setBackground(OmarchyUi::Theme::color(tokens["palette"]["background"]));
//                    });
//
// `changed` fires once per change that alters the tokens, on the GUI thread, after Color and
// Style have re-resolved (QML already reads the new values) and before the color fade starts;
// never for the initial load. `tokens` is the canonical JSON of spec/tokens.md; its colors are
// "#rrggbbaa", which QColor would read as #aarrggbb, so turn them into colors with color().
// tokens() is empty
// until a QML engine has loaded Omarchy.Theme (the first view using the kit). Vendored route
// only: the installed plugin's copy isn't linkable, so with the installed modules get QML's
// `Tokens` singleton (engine.singletonInstance<QObject *>("Omarchy.Theme", "Tokens")) and
// connect to its changed(QVariant) signal instead.
class Theme : public QObject {
  Q_OBJECT
  Q_PROPERTY(QJsonObject tokens READ tokens NOTIFY changed)
  Q_PROPERTY(int revision READ revision NOTIFY changed)

public:
  static Theme *instance();

  // The tokens in effect: the initial load, then the latest change.
  QJsonObject tokens() const { return m_tokens; }
  // 0 for the initial load, plus one per change: a cache key.
  int revision() const { return m_revision; }

  // A token color ("#rrggbbaa") as a QColor; invalid for anything else.
  static QColor color(const QJsonValue &token);

signals:
  void changed(const QJsonObject &tokens);

private:
  explicit Theme(QObject *parent = nullptr) : QObject(parent) {}
  friend class ::ThemeSource;
  void publish(const QJsonObject &tokens, int revision, bool change);

  QJsonObject m_tokens;
  int m_revision = 0;
};

}  // namespace OmarchyUi
