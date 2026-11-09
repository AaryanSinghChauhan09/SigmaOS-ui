#include "Omarchy/Theme.h"

#include <QCoreApplication>
#include <QEasingCurve>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSocketNotifier>
#include <QVariantAnimation>
#include <cmath>

extern "C" {
#include <omarchy-theme.h>
}

namespace Omarchy {

// ---------------------------------------------------------------------------------------- Border

int Border::maxWidth() const { return qMax(qMax(top, right), qMax(bottom, left)); }

Border Border::withWidth(int w) const {
  Border b = *this;
  b.top = b.right = b.bottom = b.left = qMax(0, w);
  return b;
}

Border Border::withColor(const QColor &c) const {
  Border b = *this;
  b.color = c;
  b.gradient.clear();
  return b;
}

Border Border::flat(const QColor &color, int width) {
  Border b;
  b.color = color;
  b.top = b.right = b.bottom = b.left = qMax(0, width);
  return b;
}

// ---------------------------------------------------------------------------------------- Tokens

QColor Tokens::parseColor(const QString &s) {
  // "#rrggbbaa" (QColor alone would read eight digits as #aarrggbb).
  if (s.size() == 9 && s.startsWith(QLatin1Char('#'))) {
    bool ok = false;
    const uint v = s.mid(1).toUInt(&ok, 16);
    if (ok) return QColor((v >> 24) & 0xff, (v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff);
  }
  if (s.size() == 7 && s.startsWith(QLatin1Char('#'))) return QColor(s);
  return QColor();
}

QString Tokens::colorName(const QColor &c) {
  return QString::asprintf("#%02x%02x%02x%02x", c.red(), c.green(), c.blue(), c.alpha());
}

static void flatten(const QJsonValue &v, const QString &path, QHash<QString, QColor> &out) {
  if (v.isString()) {
    const QString s = v.toString();
    if (s.startsWith(QLatin1Char('#'))) {
      const QColor c = Tokens::parseColor(s);
      if (c.isValid()) out.insert(path, c);
    }
  } else if (v.isObject()) {
    const QJsonObject o = v.toObject();
    for (auto it = o.begin(); it != o.end(); ++it)
      flatten(it.value(), path.isEmpty() ? it.key() : path + QLatin1Char('.') + it.key(), out);
  } else if (v.isArray()) {
    const QJsonArray a = v.toArray();
    for (int i = 0; i < a.size(); ++i) flatten(a.at(i), path + QLatin1Char('.') + QString::number(i), out);
  }
}

Tokens::Tokens(const QJsonObject &json) : m_json(json) { flatten(json, QString(), m_colors); }

Tokens Tokens::fromJson(const QByteArray &json) {
  return Tokens(QJsonDocument::fromJson(json).object());
}

QJsonValue Tokens::value(const QString &path) const {
  QJsonValue v = m_json;
  for (const QStringView part : QStringView(path).split(QLatin1Char('.'))) {
    if (v.isObject()) v = v.toObject().value(part);
    else if (v.isArray()) v = v.toArray().at(part.toInt());
    else return QJsonValue();
  }
  return v;
}

QColor Tokens::color(const QString &path) const {
  const auto it = m_colors.constFind(path);
  if (it != m_colors.constEnd()) return it.value();
  // A border spec's path reads as its color.
  return m_colors.value(path + QStringLiteral(".color"));
}

int Tokens::number(const QString &path, int fallback) const {
  const QJsonValue v = value(path);
  return v.isDouble() ? int(std::lround(v.toDouble())) : fallback;
}

Border Tokens::border(const QString &path) const {
  Border b;
  b.color = color(path + QStringLiteral(".color"));
  const QJsonArray w = value(path + QStringLiteral(".widths")).toArray();
  if (w.size() == 4) {
    b.top = w.at(0).toInt();
    b.right = w.at(1).toInt();
    b.bottom = w.at(2).toInt();
    b.left = w.at(3).toInt();
  }
  const QJsonValue g = value(path + QStringLiteral(".gradient"));
  if (g.isObject()) {
    const QJsonArray stops = g.toObject().value(QStringLiteral("colors")).toArray();
    for (int i = 0; i < stops.size(); ++i)
      b.gradient << color(path + QStringLiteral(".gradient.colors.") + QString::number(i));
    b.angle = g.toObject().value(QStringLiteral("angle")).toDouble();
    if (b.gradient.size() < 2) b.gradient.clear();
  }
  return b;
}

bool Tokens::dark() const { return m_json.value(QStringLiteral("mode")).toString() != QStringLiteral("light"); }

bool Tokens::reduceMotion() const { return m_json.value(QStringLiteral("reduceMotion")).toBool(); }

int Tokens::spacing(const char *name) const {
  return m_json.value(QStringLiteral("spacing")).toObject().value(QLatin1String(name)).toInt();
}

qreal Tokens::spacingScale() const {
  const QJsonValue v = m_json.value(QStringLiteral("spacing")).toObject().value(QStringLiteral("scale"));
  return v.isDouble() ? v.toDouble() : 1.0;
}

int Tokens::space(qreal px) const {
  const qreal n = px * spacingScale();
  if (n <= 0) return 0;
  return qMax(1, int(std::lround(n)));
}

int Tokens::fontSize(const char *name) const {
  const int v = m_json.value(QStringLiteral("font")).toObject().value(QLatin1String(name)).toInt();
  return v > 0 ? v : 12;
}

QString Tokens::fontFamily() const {
  const QJsonObject f = m_json.value(QStringLiteral("font")).toObject();
  const QString family = f.value(QStringLiteral("family")).toString();
  // The kit binds to the alias itself ("monospace"), as Qt Quick does, so fontconfig picks the
  // same face; an explicit OMARCHY_UI_FONT_FAMILY comes through as the family.
  return family.isEmpty() ? QStringLiteral("monospace") : family;
}

QFont Tokens::fontPx(qreal px, bool bold) const {
  QFont f(fontFamily());
  f.setPixelSize(qMax(1, int(std::lround(px))));
  if (bold) f.setBold(true);
  return f;
}

QFont Tokens::font(const char *token, bool bold) const { return fontPx(fontSize(token), bold); }

QString Tokens::stateKey(State s) const {
  switch (s) {
  case State::Normal: return QStringLiteral("controls.normal");
  case State::Hover: return QStringLiteral("controls.hover");
  case State::Focus: return QStringLiteral("controls.focus");
  case State::Selected: return QStringLiteral("controls.selected");
  case State::Pressed: return QStringLiteral("controls.pressed");
  case State::Selection: return QStringLiteral("controls.selection");
  }
  return QStringLiteral("controls.normal");
}

QColor Tokens::fill(State s) const { return color(stateKey(s) + QStringLiteral(".fill")); }

Border Tokens::border(State s) const {
  if (s == State::Pressed || s == State::Selection) return Border();
  return border(stateKey(s) + QStringLiteral(".border"));
}

// The kit's Style.<state>FillFor(foreground): a state drawn in the foreground (the default)
// takes the caller's color at the state's alpha; a state the theme draws in its accent, or in a
// literal color, keeps that.
QColor Tokens::retint(const QColor &themed, const QColor &tint) const {
  if (!themed.isValid() || !tint.isValid()) return themed;
  const QColor fg = foreground();
  if (themed.red() == fg.red() && themed.green() == fg.green() && themed.blue() == fg.blue()) {
    QColor c = tint;
    c.setAlpha(themed.alpha());
    return c;
  }
  return themed;
}

QColor Tokens::fill(State s, const QColor &tint) const { return retint(fill(s), tint); }

Border Tokens::border(State s, const QColor &tint) const {
  Border b = border(s);
  b.color = retint(b.color, tint);
  for (QColor &c : b.gradient) c = retint(c, tint);
  return b;
}

static QColor mixColor(const QColor &a, const QColor &b, qreal t) {
  if (t >= 1) return b;
  if (t <= 0) return a;
  auto ch = [t](int x, int y) { return int(std::lround(x + (y - x) * t)); };
  return QColor(ch(a.red(), b.red()), ch(a.green(), b.green()), ch(a.blue(), b.blue()), ch(a.alpha(), b.alpha()));
}

Tokens Tokens::mix(const Tokens &a, const Tokens &b, qreal t) {
  Tokens out = b;
  if (t >= 1) return out;
  for (auto it = out.m_colors.begin(); it != out.m_colors.end(); ++it) {
    const auto from = a.m_colors.constFind(it.key());
    if (from != a.m_colors.constEnd()) it.value() = mixColor(from.value(), it.value(), t);
  }
  return out;
}

// ----------------------------------------------------------------------------------------- Theme

static QString takeString(char *s) {
  if (!s) return QString();
  const QString out = QString::fromUtf8(s);
  omarchy_theme_free(s);
  return out;
}

static QPointer<Theme> s_theme;

Theme *Theme::instance() {
  if (!s_theme) {
    Q_ASSERT_X(QCoreApplication::instance(), "Omarchy::Theme", "create the application first");
    s_theme = new Theme(QCoreApplication::instance());
  }
  return s_theme;
}

Theme::Theme(QObject *parent) : QObject(parent) {
  // The watch first, then its current tokens, so no change can fall in between (docs/using-c.md).
  auto *watch = omarchy_theme_watch_new();
  m_watch = watch;
  m_tokens = Tokens::fromJson(takeString(omarchy_theme_watch_current_json(watch)).toUtf8());
  m_shown = m_tokens;
  readExtras();
  const int fd = omarchy_theme_watch_fd(watch);
  if (fd >= 0) {
    m_notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &Theme::onReadable);
  }

  m_fade = new QVariantAnimation(this);
  m_fade->setStartValue(0.0);
  m_fade->setEndValue(1.0);
  connect(m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { setFadeProgress(v.toReal()); });
  connect(m_fade, &QVariantAnimation::finished, this, [this] {
    setFadeProgress(1.0);
    emit fadingChanged(false);
  });
}

Theme::~Theme() {
  if (m_watch) omarchy_theme_watch_free(static_cast<OmarchyThemeWatch *>(m_watch));
}

bool Theme::fading() const { return m_fade && m_fade->state() == QAbstractAnimation::Running; }

void Theme::readExtras() {
  m_name = takeString(omarchy_theme_name());
  m_iconTheme = takeString(omarchy_theme_icon_theme());
  const QJsonObject s = QJsonDocument::fromJson(takeString(omarchy_theme_strings_json()).toUtf8()).object();
  m_language = s.value(QStringLiteral("language")).toString();
  m_strings.clear();
  const QJsonObject strings = s.value(QStringLiteral("strings")).toObject();
  for (auto it = strings.begin(); it != strings.end(); ++it) m_strings.insert(it.key(), it.value().toString());
  m_iconCache.clear();
}

void Theme::onReadable() {
  const QString json = takeString(omarchy_theme_watch_take(static_cast<OmarchyThemeWatch *>(m_watch)));
  if (!json.isEmpty()) deliver(json.toUtf8());
}

void Theme::deliver(const QByteArray &tokensJson) {
  const Tokens next = Tokens::fromJson(tokensJson);
  if (next.isNull()) return;
  // Mid-fade, the next fade starts from the colors on screen, not from the old theme.
  const Tokens from = m_shown;
  const bool wasFading = fading();
  m_fade->stop();
  m_tokens = next;
  ++m_revision;
  readExtras();

  // Rounding, sizes and fonts switch at once; colors fade over motion.theme on motion.ease,
  // unless motion is reduced (spec/following.md).
  const int ms = next.reduceMotion() ? 0 : next.number(QStringLiteral("motion.theme"), 280);
  const bool fade = ms > 0 && !from.sameColors(next);
  if (fade) {
    m_from = from;
    m_shown = Tokens::mix(from, next, 0.0);
  } else {
    m_shown = next;
  }
  emit metricsChanged();
  emit colorsChanged();
  emit changed(m_tokens.json());

  if (fade) {
    const QJsonArray e = next.value(QStringLiteral("motion.ease")).toArray();
    QEasingCurve curve(QEasingCurve::BezierSpline);
    const qreal x1 = e.size() == 4 ? e.at(0).toDouble() : 0.25, y1 = e.size() == 4 ? e.at(1).toDouble() : 0.1;
    const qreal x2 = e.size() == 4 ? e.at(2).toDouble() : 0.25, y2 = e.size() == 4 ? e.at(3).toDouble() : 1.0;
    curve.addCubicBezierSegment(QPointF(x1, y1), QPointF(x2, y2), QPointF(1, 1));
    m_fade->setEasingCurve(curve);
    m_fade->setDuration(ms);
    m_fade->start();
    if (!wasFading) emit fadingChanged(true);
  } else if (wasFading) {
    emit fadingChanged(false);
  }
}

void Theme::setFadeProgress(qreal t) {
  // The end lands exactly on the tokens (no interpolation drift).
  m_shown = t >= 1.0 ? m_tokens : Tokens::mix(m_from, m_tokens, t);
  emit colorsChanged();
}

QString Theme::string(const QString &key, const QString &english) const {
  const auto it = m_strings.constFind(key);
  return it != m_strings.constEnd() && !it.value().isEmpty() ? it.value() : english;
}

QString Theme::iconPath(const QString &name, int size) const {
  if (name.isEmpty()) return QString();
  if (name.startsWith(QLatin1Char('/'))) return QFile::exists(name) ? name : QString();
  const QString key = name + QLatin1Char('@') + QString::number(size);
  const auto it = m_iconCache.constFind(key);
  if (it != m_iconCache.constEnd()) return it.value();
  const QString path = takeString(omarchy_theme_icon_lookup(name.toUtf8().constData(), size));
  m_iconCache.insert(key, path);
  return path;
}

QIcon Theme::icon(const QString &name, int size) const {
  const QString path = iconPath(name, size);
  return path.isEmpty() ? QIcon() : QIcon(path);
}

}  // namespace Omarchy
