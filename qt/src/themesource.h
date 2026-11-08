#pragma once

#include <QFileSystemWatcher>
#include <QLocalSocket>
#include <QObject>
#include <QString>
#include <QTimer>

class QQmlEngine;
class QJSEngine;

// Everything the QML theme singletons read from outside the process, kept current without
// polling. This replaces the Omarchy shell's FileView / Process / Hyprland pieces:
//
// - colors.toml and shell.toml from the current theme. A theme switch swaps
//   ~/.local/state/omarchy/current/theme as a whole directory and then rewrites theme.name, so
//   the watch is on current/, not on the files.
// - ~/.config/omarchy/shell.toml, the user's override (`omarchy display text size`).
// - Hyprland's decoration:rounding, general:gaps_out, animations:enabled and general:border_size,
//   asked over the instance socket and re-asked on `configreloaded` from socket2.
// - The family fontconfig's `monospace` alias resolves to (`omarchy font set`).
// - ~/.config/omarchy/language, the display language (`omarchy language`).
//
// OMARCHY_UI_THEME_DIR, OMARCHY_UI_ROUNDING, OMARCHY_UI_GAPS_OUT, OMARCHY_UI_REDUCE_MOTION and
// OMARCHY_UI_FONT_FAMILY pin any of these for reviews and tests (spec/tokens.md).
class ThemeSource : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString colorsText READ colorsText NOTIFY themeChanged)
  Q_PROPERTY(QString shellText READ shellText NOTIFY themeChanged)
  Q_PROPERTY(QString userShellText READ userShellText NOTIFY themeChanged)
  Q_PROPERTY(QString themeName READ themeName NOTIFY themeChanged)
  Q_PROPERTY(QString languageText READ languageText NOTIFY languageChanged)
  Q_PROPERTY(QString themeDir READ themeDir CONSTANT)
  Q_PROPERTY(bool pinned READ pinned CONSTANT)
  Q_PROPERTY(int rounding READ rounding NOTIFY hyprlandChanged)
  Q_PROPERTY(double gapsOut READ gapsOut NOTIFY hyprlandChanged)
  Q_PROPERTY(bool animations READ animations NOTIFY hyprlandChanged)
  Q_PROPERTY(int borderSize READ borderSize NOTIFY hyprlandChanged)
  Q_PROPERTY(QString fontFamily READ fontFamily CONSTANT)
  Q_PROPERTY(QString resolvedFontFamily READ resolvedFontFamily NOTIFY fontChanged)

public:
  explicit ThemeSource(QObject *parent = nullptr);

  static ThemeSource *instance();
  static ThemeSource *create(QQmlEngine *, QJSEngine *);

  QString colorsText() const { return m_colors; }
  QString shellText() const { return m_shell; }
  QString userShellText() const { return m_userShell; }
  QString themeName() const { return m_themeName; }
  QString languageText() const { return m_language; }
  QString themeDir() const { return m_themeDir; }
  bool pinned() const { return m_pinned; }
  int rounding() const { return m_rounding; }
  double gapsOut() const { return m_gapsOut; }
  bool animations() const { return m_animations; }
  int borderSize() const { return m_borderSize; }
  QString fontFamily() const { return m_fontFamily; }
  QString resolvedFontFamily() const { return m_resolvedFontFamily; }

  // Re-read everything now. Changes arrive on their own; this is for tests.
  Q_INVOKABLE void reload();

signals:
  void themeChanged();
  void hyprlandChanged();
  void fontChanged();
  void languageChanged();

private:
  void readFiles();
  void readHyprland();
  void readFont();
  void armWatcher();
  void connectEvents();
  QString hyprDir() const;
  QByteArray hyprRequest(const QString &request) const;

  QString m_colors, m_shell, m_userShell, m_themeName, m_themeDir, m_language;
  bool m_pinned = false;
  int m_rounding = 0;
  double m_gapsOut = 10;
  bool m_animations = true;
  int m_borderSize = 2;
  QString m_fontFamily = QStringLiteral("monospace");
  QString m_resolvedFontFamily = QStringLiteral("monospace");

  QFileSystemWatcher m_watcher;
  QTimer m_settle;
  QTimer m_reconnect;
  QLocalSocket m_events;
  QByteArray m_eventBuffer;
};
