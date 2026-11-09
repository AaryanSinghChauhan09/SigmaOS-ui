// Omarchy::Widgets::apply: the palette, the style, the font and (with KF6) KDE's color scheme,
// from the live tokens, kept following them.

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QPointer>
#include <QSaveFile>
#include <QStandardPaths>
#include <QWidget>

#include "Omarchy/Widgets.h"

#ifdef OMARCHY_WITH_KF6
#include <KConfigGroup>
#include <KSharedConfig>
#endif

namespace Omarchy::Widgets {

namespace {

// `top` composited over `base`: palettes and KDE schemes want opaque colors.
QColor over(const QColor &top, const QColor &base) {
  const qreal a = top.alphaF();
  auto ch = [a](qreal t, qreal b) { return t * a + b * (1 - a); };
  return QColor::fromRgbF(ch(top.redF(), base.redF()), ch(top.greenF(), base.greenF()), ch(top.blueF(), base.blueF()), 1.0);
}

struct Applied {
  Options options;
  QPointer<QApplication> app;
  QPointer<Style> style;
  QString schemePath;
  QFont font;
  bool connected = false;
};

Applied &state() {
  static Applied s;
  return s;
}

void applyColors() {
  Applied &s = state();
  if (!s.app) return;
  const Tokens &t = Theme::instance()->shown();
#ifdef OMARCHY_WITH_KF6
  if (s.options.kdeColorScheme && !s.schemePath.isEmpty()) {
    QSaveFile f(s.schemePath);
    if (f.open(QIODevice::WriteOnly)) {
      f.write(kdeColorScheme(t));
      f.commit();
    }
    // KColorScheme reads the scheme KColorSchemeManager names in this property, through a shared
    // config that has to re-read the file.
    KSharedConfig::openConfig(s.schemePath)->reparseConfiguration();
    s.app->setProperty("KDE_COLOR_SCHEME_PATH", s.schemePath);
  }
#endif
  if (s.options.palette) QApplication::setPalette(palette(t));
}

void applyMetrics() {
  Applied &s = state();
  if (!s.app) return;
  const Tokens &t = Theme::instance()->tokens();
  if (s.options.font) {
    const QFont f = t.font("body");
    if (f != s.font) {
      s.font = f;
      QApplication::setFont(f);
    }
  }
  if (s.options.style) {
    // Sizes the style reports (padding, rounding, row heights) changed: widgets drop their
    // cached size hints and relayout, as they do when the style itself changes.
    for (QWidget *w : QApplication::allWidgets()) {
      QEvent e(QEvent::StyleChange);
      QCoreApplication::sendEvent(w, &e);
      w->update();
    }
  }
}

}  // namespace

QPalette palette(const Tokens &t) {
  const QColor bg = t.background(), fg = t.foreground(), accent = t.accent();
  QPalette p;
  for (const QPalette::ColorGroup g : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
    const bool off = g == QPalette::Disabled;
    const QColor text = off ? t.textFaint() : fg;
    p.setColor(g, QPalette::Window, bg);
    p.setColor(g, QPalette::WindowText, text);
    p.setColor(g, QPalette::Base, bg);
    p.setColor(g, QPalette::AlternateBase, over(t.fill(State::Normal), bg));
    p.setColor(g, QPalette::Text, text);
    p.setColor(g, QPalette::PlaceholderText, t.textFaint());
    p.setColor(g, QPalette::Button, over(t.fill(State::Normal), bg));
    p.setColor(g, QPalette::ButtonText, text);
    p.setColor(g, QPalette::BrightText, bg);
    p.setColor(g, QPalette::Highlight, over(t.fill(State::Selection), bg));
    p.setColor(g, QPalette::HighlightedText, text);
    p.setColor(g, QPalette::ToolTipBase, over(t.color(QStringLiteral("surfaces.tooltip.background")), bg));
    p.setColor(g, QPalette::ToolTipText, t.color(QStringLiteral("surfaces.tooltip.text")));
    p.setColor(g, QPalette::Link, accent);
    p.setColor(g, QPalette::LinkVisited, accent);
    p.setColor(g, QPalette::Light, over(Paint::alpha(fg, 0.25), bg));
    p.setColor(g, QPalette::Midlight, over(Paint::alpha(fg, 0.12), bg));
    p.setColor(g, QPalette::Mid, over(Paint::alpha(fg, 0.4), bg));
    p.setColor(g, QPalette::Dark, over(Paint::alpha(fg, 0.5), bg));
    p.setColor(g, QPalette::Shadow, t.dark() ? QColor(0, 0, 0) : over(Paint::alpha(fg, 0.6), bg));
    p.setColor(g, QPalette::Accent, accent);
  }
  return p;
}

QByteArray kdeColorScheme(const Tokens &t) {
  const QColor bg = t.background(), fg = t.foreground();
  auto rgb = [](const QColor &c) { return QByteArray::number(c.red()) + ',' + QByteArray::number(c.green()) + ',' + QByteArray::number(c.blue()); };
  auto o = [&](const QColor &c) { return rgb(over(c, bg)); };
  QByteArray out;
  auto set = [&](const char *group, const QColor &back, const QColor &alt, const QColor &text) {
    out += QByteArray("[Colors:") + group + "]\n";
    out += "BackgroundNormal=" + rgb(over(back, bg)) + "\n";
    out += "BackgroundAlternate=" + rgb(over(alt, bg)) + "\n";
    out += "DecorationFocus=" + o(t.border(State::Focus).color) + "\n";
    out += "DecorationHover=" + o(t.border(State::Hover).color) + "\n";
    out += "ForegroundNormal=" + rgb(text) + "\n";
    out += "ForegroundActive=" + rgb(t.accent()) + "\n";
    out += "ForegroundInactive=" + rgb(t.textDim()) + "\n";
    out += "ForegroundLink=" + rgb(t.accent()) + "\n";
    out += "ForegroundVisited=" + rgb(t.accent()) + "\n";
    out += "ForegroundNegative=" + rgb(t.urgent()) + "\n";
    out += "ForegroundNeutral=" + rgb(t.warning()) + "\n";
    out += "ForegroundPositive=" + rgb(t.success()) + "\n\n";
  };
  out += "[General]\nColorScheme=Omarchy\nName=Omarchy\nshadeSortColumn=false\n\n";
  out += "[KDE]\ncontrast=0\n\n";
  // Disabled: the faint text color is already in the scheme; keep the effects mild. No inactive
  // window effect: Omarchy windows don't dim when they lose focus.
  out += "[ColorEffects:Disabled]\nColor=" + rgb(bg) + "\nColorAmount=0\nColorEffect=0\nContrastAmount=0.65\nContrastEffect=1\nIntensityAmount=0.1\nIntensityEffect=2\n\n";
  out += "[ColorEffects:Inactive]\nChangeSelectionColor=false\nEnable=false\n\n";
  set("Window", bg, t.fill(State::Normal), fg);
  set("View", bg, t.fill(State::Normal), fg);
  set("Button", t.fill(State::Normal), t.fill(State::Hover), fg);
  set("Header", bg, t.fill(State::Normal), fg);
  set("Complementary", bg, t.fill(State::Normal), fg);
  set("Selection", t.fill(State::Selected), t.fill(State::Hover), t.color(QStringLiteral("surfaces.menu.selectedText")));
  set("Tooltip", t.color(QStringLiteral("surfaces.tooltip.background")), t.fill(State::Normal), t.color(QStringLiteral("surfaces.tooltip.text")));
  out += "[WM]\nactiveBackground=" + rgb(bg) + "\nactiveForeground=" + rgb(fg) + "\nactiveBlend=" + rgb(fg) + "\ninactiveBackground=" + rgb(bg) +
         "\ninactiveForeground=" + rgb(t.textDim()) + "\ninactiveBlend=" + rgb(t.textDim()) + "\n";
  return out;
}

QString kdeColorSchemePath() {
#ifdef OMARCHY_WITH_KF6
  QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
  if (dir.isEmpty()) dir = QDir::tempPath();
  return dir + QStringLiteral("/omarchy-ui/Omarchy-") + QString::number(QCoreApplication::applicationPid()) + QStringLiteral(".colors");
#else
  return QString();
#endif
}

Theme *apply(QApplication *app, const Options &options) { return apply(*app, options); }

Theme *apply(QApplication &app, const Options &options) {
  Applied &s = state();
  s.options = options;
  s.app = &app;
  Theme *theme = Theme::instance();

  if (options.quickControlsStyle && !qEnvironmentVariableIsSet("QT_QUICK_CONTROLS_STYLE")) {
    // Kirigami and Qt Quick Controls in this app draw through the QStyle with KDE's desktop
    // style (qqc2-desktop-style), and its Kirigami theme reads the color scheme below.
    const QString qml = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
    if (QFile::exists(qml + QStringLiteral("/org/kde/desktop/qmldir"))) qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");
  }

#ifdef OMARCHY_WITH_KF6
  if (options.kdeColorScheme) {
    s.schemePath = kdeColorSchemePath();
    QDir().mkpath(QFileInfo(s.schemePath).absolutePath());
    if (!s.connected) {
      QObject::connect(&app, &QCoreApplication::aboutToQuit, [] { QFile::remove(state().schemePath); });
    }
  }
#endif

  if (options.style && !s.style) {
    s.style = new Style;
    QApplication::setStyle(s.style);
  }
  s.font = QFont();
  applyMetrics();
  applyColors();

  if (!s.connected) {
    s.connected = true;
    QObject::connect(theme, &Theme::colorsChanged, &app, &applyColors);
    QObject::connect(theme, &Theme::metricsChanged, &app, &applyMetrics);
  }
  return theme;
}

}  // namespace Omarchy::Widgets
