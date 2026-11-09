// The live-switch test (spec/following.md): run under tools/live/run, which starts a temp HOME
// on tokyo-night. The app makes the one call, the theme is switched to catppuccin-latte the way
// `omarchy theme set` does it, and we check what an app sees: the palette (and with KF6, KDE's
// color scheme), the changed hook once with the new tokens, the revision, and the fade (or none
// under OMARCHY_UI_REDUCE_MOTION=1).
//
//   tools/live/run build/kde/omarchy-widgets-live-test

#include <QApplication>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QProcess>
#include <QTest>
#include <cstdio>

#include <Omarchy/Widgets.h>

#ifdef OMARCHY_WITH_KF6
#include <KColorScheme>
#endif

static int failures = 0;
static void check(bool ok, const char *what) {
  printf("%s: %s\n", ok ? "ok" : "FAIL", what);
  if (!ok) ++failures;
}

static bool waitFor(const std::function<bool()> &cond, int ms) {
  QElapsedTimer t;
  t.start();
  while (!cond() && t.elapsed() < ms) QTest::qWait(20);
  return cond();
}

int main(int argc, char **argv) {
  qputenv("QT_QPA_PLATFORM", "offscreen");
  QApplication app(argc, argv);
  const QByteArray home = qgetenv("OMARCHY_UI_LIVE_HOME"), sw = qgetenv("OMARCHY_UI_LIVE_SWITCH");
  if (home.isEmpty() || sw.isEmpty()) {
    fprintf(stderr, "run me under tools/live/run\n");
    return 2;
  }
  const bool reduced = qgetenv("OMARCHY_UI_REDUCE_MOTION") == "1";

  Omarchy::Theme *theme = Omarchy::Widgets::apply(app);
  const QColor latte(0xef, 0xf1, 0xf5), night(0x1a, 0x1b, 0x26);
  check(QApplication::palette().color(QPalette::Window) == night, "starts on tokyo-night's background");
  check(theme->revision() == 0, "revision 0 at start");

  // A kit widget on screen, to see it repaint through the fade.
  Omarchy::Button button(QStringLiteral("Wi-Fi"));
  button.setBordered(true);
  button.show();

  int changes = 0;
  QJsonObject hooked;
  QColor seenInHook;
  QObject::connect(theme, &Omarchy::Theme::changed, &app, [&](const QJsonObject &tokens) {
    ++changes;
    hooked = tokens;
    // Rule 3: everything the binding exposes already reads the new values.
    seenInHook = theme->tokens().background();
  });
  bool faded = false, between = false;
  QObject::connect(theme, &Omarchy::Theme::fadingChanged, &app, [&](bool on) { faded |= on; });
  QObject::connect(theme, &Omarchy::Theme::colorsChanged, &app, [&] {
    const QColor c = QApplication::palette().color(QPalette::Window);
    if (c != latte && c != night) between = true;
  });

  QProcess::execute(QString::fromLocal8Bit(sw), {QString::fromLocal8Bit(home), QStringLiteral("catppuccin-latte")});

  check(waitFor([&] { return changes > 0; }, 5000), "the changed hook fires after a theme switch");
  check(waitFor([&] { return !theme->fading() && QApplication::palette().color(QPalette::Window) == latte; }, 5000),
        "the palette's background is catppuccin-latte's #eff1f5");
  check(hooked.value(QStringLiteral("palette")).toObject().value(QStringLiteral("background")).toString() == QStringLiteral("#eff1f5ff"),
        "the hook gets the new tokens (palette.background #eff1f5ff)");
  check(seenInHook == latte, "inside the hook, tokens() already reads the new theme");
  check(theme->shown().background() == latte && theme->shown().sameColors(theme->tokens()), "the colors on screen land exactly on the tokens");
#ifdef OMARCHY_WITH_KF6
  check(KColorScheme(QPalette::Active, KColorScheme::Window).background().color() == latte, "KColorScheme (KDE apps, Kirigami) reads the new background");
#endif
  QTest::qWait(600);
  check(changes == 1, "the hook fired once, and still once 600 ms later");
  check(theme->revision() == 1, "revision 1");
  if (reduced) {
    check(!faded && !between, "no fade under reduced motion");
  } else {
    check(faded && between, "the colors faded through values between the two themes");
  }

  printf("%s: live switch%s, %d changed hook\n", failures ? "FAILED" : "passed", reduced ? " (reduced motion)" : "", changes);
  return failures ? 1 : 0;
}
