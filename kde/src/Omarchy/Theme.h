#pragma once

// The live Omarchy theme, from libomarchy-theme (the Rust resolver; spec/tokens.md), as Qt types.
//
//   Omarchy::Theme::instance()->tokens()           the tokens in effect (Omarchy::Tokens)
//   connect(Omarchy::Theme::instance(), &Omarchy::Theme::changed, ...)   the changed hook
//   Omarchy::Theme::instance()->revision()         0, plus one per change
//
// The singleton follows the desktop through the C library's watch fd (a QSocketNotifier on the
// GUI thread) and fades colors on a switch (spec/following.md). Omarchy::Widgets::apply() maps it
// onto the application's palette, style and font; kit widgets read it when they paint.

#include <QColor>
#include <QFont>
#include <QHash>
#include <QIcon>
#include <QJsonObject>
#include <QList>
#include <QMargins>
#include <QObject>
#include <QString>

#include "Omarchy/Export.h"

class QVariantAnimation;
class QSocketNotifier;

namespace Omarchy {

// A border spec (spec/tokens.md): a color, four widths and an optional gradient. Color-only
// consumers use `color` (a gradient's first stop).
struct OMARCHY_EXPORT Border {
  QColor color;
  int top = 0, right = 0, bottom = 0, left = 0;
  QList<QColor> gradient;  // two or more stops when the theme gives a gradient
  qreal angle = 0;         // degrees, Hyprland's convention

  bool isNone() const { return maxWidth() <= 0; }
  bool isUniform() const { return top == right && top == bottom && top == left; }
  bool hasGradient() const { return gradient.size() > 1; }
  int maxWidth() const;
  QMargins margins() const { return QMargins(left, top, right, bottom); }
  Border withWidth(int w) const;
  Border withColor(const QColor &c) const;
  static Border none() { return Border(); }
  static Border flat(const QColor &color, int width);
};

// The five control states of spec/tokens.md ([controls]).
enum class State { Normal, Hover, Focus, Selected, Pressed, Selection };

// One set of resolved tokens: the canonical JSON, with typed accessors. Cheap to copy.
class OMARCHY_EXPORT Tokens {
public:
  Tokens() = default;
  explicit Tokens(const QJsonObject &json);
  static Tokens fromJson(const QByteArray &json);

  bool isNull() const { return m_json.isEmpty(); }
  const QJsonObject &json() const { return m_json; }

  // Any color by its path in the canonical JSON: "palette.accent", "roles.textDim",
  // "controls.hover.fill", "surfaces.popups.background". Invalid when there is none.
  QColor color(const QString &path) const;
  // Any border spec: "controls.normal.border", "surfaces.menu.border".
  Border border(const QString &path) const;
  QJsonValue value(const QString &path) const;
  int number(const QString &path, int fallback = 0) const;

  QColor foreground() const { return color(QStringLiteral("palette.foreground")); }
  QColor background() const { return color(QStringLiteral("palette.background")); }
  QColor accent() const { return color(QStringLiteral("palette.accent")); }
  QColor urgent() const { return color(QStringLiteral("palette.urgent")); }
  QColor muted() const { return color(QStringLiteral("palette.muted")); }
  QColor success() const { return color(QStringLiteral("palette.success")); }
  QColor warning() const { return color(QStringLiteral("palette.warning")); }
  QColor textDim() const { return color(QStringLiteral("roles.textDim")); }
  QColor textFaint() const { return color(QStringLiteral("roles.textFaint")); }
  QColor track() const { return color(QStringLiteral("roles.track")); }
  QColor scrim() const { return color(QStringLiteral("roles.scrim")); }
  bool dark() const;

  // Hyprland's rounding, and the cap small parts use: min(4, radius).
  int radius() const { return number(QStringLiteral("radius")); }
  int smallRadius() const { return qMin(4, radius()); }
  int gapsOut() const { return number(QStringLiteral("gapsOut")); }
  bool reduceMotion() const;
  // A duration in ms, 0 under reduced motion.
  int duration(int ms) const { return reduceMotion() ? 0 : ms; }

  // spacing.<name> ("controlHeight", "panelPadding", ...), and space(px) for any other size:
  // max(1, round(px × spacing.scale)).
  int spacing(const char *name) const;
  qreal spacingScale() const;
  int space(qreal px) const;

  // font.<name> in px ("body", "caption", "icon", ...), the family (fontconfig's monospace,
  // resolved), and a QFont of it at a size token or pixel size.
  int fontSize(const char *name) const;
  QString fontFamily() const;
  QFont font(const char *token = "body", bool bold = false) const;
  QFont fontPx(qreal px, bool bold = false) const;

  // State fills and borders ([controls]) for the theme's own colors, or re-tinted for a
  // control drawn in another color (an urgent button): the state's alpha on `tint` when the
  // theme draws that state in the foreground, else the theme's color.
  QColor fill(State s) const;
  QColor fill(State s, const QColor &tint) const;
  Border border(State s) const;
  Border border(State s, const QColor &tint) const;
  // The selected state's text color (controls.selected.text).
  QColor selectedText() const { return color(QStringLiteral("controls.selected.text")); }
  QColor selectedText(const QColor &tint) const { return retint(selectedText(), tint); }

  // Every color moved `t` of the way from `a` to `b` (sRGB, alpha included); everything else is b's.
  static Tokens mix(const Tokens &a, const Tokens &b, qreal t);
  bool sameColors(const Tokens &o) const { return m_colors == o.m_colors; }

  // `themed` (a state color of this theme) re-tinted for a control drawn in `tint`: the tint at
  // the state's alpha when the theme draws that state in the foreground, else `themed`.
  QColor tinted(const QColor &themed, const QColor &tint) const { return retint(themed, tint); }

  static QColor parseColor(const QString &rrggbbaa);
  static QString colorName(const QColor &c);  // "#rrggbbaa"

private:
  QString stateKey(State s) const;
  QColor retint(const QColor &themed, const QColor &tint) const;
  QJsonObject m_json;
  QHash<QString, QColor> m_colors;
};

class OMARCHY_EXPORT Theme : public QObject {
  Q_OBJECT
  Q_PROPERTY(qulonglong revision READ revision NOTIFY changed)
  Q_PROPERTY(bool fading READ fading NOTIFY fadingChanged)
  Q_PROPERTY(QString name READ name NOTIFY changed)
  Q_PROPERTY(QString iconTheme READ iconTheme NOTIFY changed)
  Q_PROPERTY(QString language READ language NOTIFY changed)

public:
  // Created on first use (after the QCoreApplication), destroyed with it.
  static Theme *instance();

  // The tokens in effect: the new set as soon as a change is delivered.
  const Tokens &tokens() const { return m_tokens; }
  // The colors on screen: tokens() except mid-fade, when its colors are between the old theme
  // and the new. Painting reads this one.
  const Tokens &shown() const { return m_shown; }
  QJsonObject json() const { return m_tokens.json(); }
  quint64 revision() const { return m_revision; }
  bool fading() const;

  QString name() const { return m_name; }
  QString iconTheme() const { return m_iconTheme; }
  QString language() const { return m_language; }

  // The kit's strings in the display language (omarchy_theme_strings_json), else `english`.
  QString string(const QString &key, const QString &english = QString()) const;
  // The file a freedesktop icon name resolves to at `size` device pixels, Omarchy's lookup.
  QString iconPath(const QString &name, int size) const;
  QIcon icon(const QString &name, int size) const;

  // For tests: deliver a change as the watch would (the canonical JSON).
  void deliver(const QByteArray &tokensJson);

Q_SIGNALS:
  // The changed hook (spec/following.md): once per real change, on the GUI thread, after
  // tokens() and revision() hold the new values, with the new tokens; never for the initial load.
  void changed(const QJsonObject &tokens);
  // Each time the colors on screen change: a switch, and every frame of its fade.
  void colorsChanged();
  // A change moved sizes, rounding or fonts (they switch at once, at the start of a fade).
  void metricsChanged();
  void fadingChanged(bool fading);

private:
  explicit Theme(QObject *parent);
  ~Theme() override;
  void onReadable();
  void readExtras();
  void setFadeProgress(qreal t);

  void *m_watch = nullptr;
  QSocketNotifier *m_notifier = nullptr;
  QVariantAnimation *m_fade = nullptr;
  Tokens m_tokens, m_shown, m_from;
  quint64 m_revision = 0;
  QString m_name, m_iconTheme, m_language;
  QHash<QString, QString> m_strings;
  mutable QHash<QString, QString> m_iconCache;
};

}  // namespace Omarchy
