#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

class QQmlEngine;
class QJSEngine;

// Freedesktop icon-theme lookup, the same algorithm as the Rust `omarchy_theme::icons`
// (spec/components/lists.md, Avatar):
//
// 1. The icon theme is ThemeSource.iconTheme: the current Omarchy theme's icons.theme line, else
//    Yaru-blue, as omarchy-theme-set-gnome applies it (OMARCHY_UI_ICON_THEME pins it).
// 2. That theme, its index.theme Inherits chain depth-first (themes not installed are skipped),
//    then hicolor.
// 3. Search roots: $XDG_DATA_HOME/icons, each $XDG_DATA_DIRS/*/icons, ~/.icons, then
//    /usr/share/pixmaps; OMARCHY_UI_ICON_DIRS (colon-separated) replaces them.
// 4. In each theme an SVG wins (from a Scalable directory first), else the PNG from the
//    smallest directory at or above the requested size, else the largest.
// 5. No theme has it: a loose file in a root (pixmaps). Else "".
//
// From QML (`import Omarchy.Theme.Native`):
//
//   Image { source: IconLookup.theme, IconLookup.url("firefox", 96) }
//
// Reading `theme` in the binding re-evaluates it on an icon theme switch. Answers are cached
// per theme.
class IconLookup : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString theme READ theme NOTIFY themeChanged)
  Q_PROPERTY(QStringList themes READ themes NOTIFY themeChanged)

public:
  explicit IconLookup(QObject *parent = nullptr);

  static IconLookup *instance();
  static IconLookup *create(QQmlEngine *, QJSEngine *);

  QString theme() const { return m_theme; }
  // The themes searched, in order.
  QStringList themes() const;

  // The file for icon `name` at `size` device pixels, or "" when nothing has it. An absolute
  // path to an existing file comes back as is.
  Q_INVOKABLE QString path(const QString &name, int size);
  // path() as a file URL, or an empty URL.
  Q_INVOKABLE QUrl url(const QString &name, int size);

  // Search `theme` under `roots` instead of following ThemeSource (tests).
  void setTheme(const QString &theme, const QStringList &roots);

  static QStringList searchDirs();

signals:
  void themeChanged();

private:
  enum class Kind { Fixed, Scalable, Threshold };
  struct Subdir {
    QString path;
    int pixels = 0;
    Kind kind = Kind::Threshold;
  };
  struct Index {
    QString name;
    QStringList inherits;
    QVector<Subdir> dirs;
  };

  void follow();
  void visit(const QString &name, QStringList &seen);
  QString search(const QString &name, int size) const;
  QString inTheme(const Index &index, const QString &name, int size) const;

  QString m_theme;
  QStringList m_roots;
  QVector<Index> m_chain;
  QHash<QString, QString> m_found;
};
