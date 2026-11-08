#include "iconlookup.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>

#include "themesource.h"

namespace {

const char *const kExtensions[] = {"svg", "png"};

QString readText(const QString &path) {
  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) return QString();
  return QString::fromUtf8(f.readAll());
}

QStringList splitList(const QString &value) {
  QStringList out;
  for (const QString &part : value.split(QLatin1Char(',')))
    if (!part.trimmed().isEmpty()) out << part.trimmed();
  return out;
}

bool isFile(const QString &path) { return QFileInfo(path).isFile(); }

}  // namespace

IconLookup::IconLookup(QObject *parent) : QObject(parent) {
  follow();
  connect(ThemeSource::instance(), &ThemeSource::themeChanged, this, [this] {
    if (ThemeSource::instance()->iconTheme() != m_theme) follow();
  });
}

IconLookup *IconLookup::instance() {
  static IconLookup *lookup = new IconLookup();
  return lookup;
}

IconLookup *IconLookup::create(QQmlEngine *, QJSEngine *) { return instance(); }

QStringList IconLookup::searchDirs() {
  const QString pinned = qEnvironmentVariable("OMARCHY_UI_ICON_DIRS").trimmed();
  if (!pinned.isEmpty()) return pinned.split(QLatin1Char(':'), Qt::SkipEmptyParts);

  const QString home = QDir::homePath();
  QString dataHome = qEnvironmentVariable("XDG_DATA_HOME");
  if (dataHome.isEmpty()) dataHome = home + QStringLiteral("/.local/share");
  QString dataDirs = qEnvironmentVariable("XDG_DATA_DIRS");
  if (dataDirs.isEmpty()) dataDirs = QStringLiteral("/usr/local/share:/usr/share");

  QStringList dirs{dataHome + QStringLiteral("/icons")};
  for (const QString &d : dataDirs.split(QLatin1Char(':'), Qt::SkipEmptyParts)) dirs << d + QStringLiteral("/icons");
  dirs << home + QStringLiteral("/.icons") << QStringLiteral("/usr/share/pixmaps");
  return dirs;
}

void IconLookup::follow() { setTheme(ThemeSource::instance()->iconTheme(), searchDirs()); }

void IconLookup::setTheme(const QString &theme, const QStringList &roots) {
  m_theme = theme;
  m_roots = roots;
  m_chain.clear();
  m_found.clear();
  QStringList seen;
  visit(theme, seen);
  visit(QStringLiteral("hicolor"), seen);
  emit themeChanged();
}

QStringList IconLookup::themes() const {
  QStringList out;
  for (const Index &i : m_chain) out << i.name;
  return out;
}

// A theme, then its parents depth-first. Themes that aren't installed are skipped.
void IconLookup::visit(const QString &name, QStringList &seen) {
  if (seen.contains(name)) return;
  seen << name;

  QString text;
  bool found = false;
  for (const QString &root : m_roots) {
    const QString file = root + QLatin1Char('/') + name + QStringLiteral("/index.theme");
    if (isFile(file)) {
      text = readText(file);
      found = true;
      break;
    }
  }
  if (!found) return;

  // The index.theme groups, as key → value per group.
  QHash<QString, QHash<QString, QString>> groups;
  QString group;
  for (const QString &raw : text.split(QLatin1Char('\n'))) {
    const QString line = raw.trimmed();
    if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) continue;
    if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
      group = line.mid(1, line.size() - 2);
      continue;
    }
    const qsizetype eq = line.indexOf(QLatin1Char('='));
    if (eq > 0) groups[group].insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
  }

  const QHash<QString, QString> head = groups.value(QStringLiteral("Icon Theme"));
  Index index;
  index.name = name;
  index.inherits = splitList(head.value(QStringLiteral("Inherits")));
  QStringList names = splitList(head.value(QStringLiteral("Directories")));
  names << splitList(head.value(QStringLiteral("ScaledDirectories")));
  names.removeDuplicates();
  for (const QString &dir : names) {
    if (!groups.contains(dir)) continue;
    const QHash<QString, QString> g = groups.value(dir);
    bool ok = false;
    const int size = g.value(QStringLiteral("Size")).toInt(&ok);
    if (!ok || size < 0) continue;
    bool scaleOk = false;
    const int scale = g.value(QStringLiteral("Scale")).toInt(&scaleOk);
    const QString type = g.value(QStringLiteral("Type"));
    Subdir s;
    s.path = dir;
    s.pixels = size * (scaleOk && scale > 0 ? scale : 1);
    s.kind = type == QLatin1String("Fixed") ? Kind::Fixed
             : type == QLatin1String("Scalable") ? Kind::Scalable
                                                  : Kind::Threshold;
    index.dirs << s;
  }

  const QStringList parents = index.inherits;
  m_chain << index;
  for (const QString &p : parents) visit(p, seen);
}

QString IconLookup::path(const QString &name, int size) {
  if (name.isEmpty()) return QString();
  const QString key = name + QLatin1Char('\0') + QString::number(size);
  auto it = m_found.constFind(key);
  if (it != m_found.constEnd()) return it.value();
  const QString hit = search(name, size);
  m_found.insert(key, hit);
  return hit;
}

QUrl IconLookup::url(const QString &name, int size) {
  const QString p = path(name, size);
  return p.isEmpty() ? QUrl() : QUrl::fromLocalFile(p);
}

QString IconLookup::search(const QString &name, int size) const {
  if (QDir::isAbsolutePath(name)) return isFile(name) ? name : QString();
  if (name.contains(QLatin1Char('/'))) return QString();
  for (const Index &index : m_chain) {
    const QString hit = inTheme(index, name, size);
    if (!hit.isEmpty()) return hit;
  }
  // Loose files in a root: /usr/share/pixmaps.
  for (const QString &root : m_roots)
    for (const char *ext : kExtensions) {
      const QString file = root + QLatin1Char('/') + name + QLatin1Char('.') + QLatin1String(ext);
      if (isFile(file)) return file;
    }
  return QString();
}

QString IconLookup::inTheme(const Index &index, const QString &name, int size) const {
  struct Hit {
    const Subdir *dir;
    QString path;
  };
  // Every directory's file, from the first root that has one.
  QVector<Hit> svgs, pngs;
  for (const Subdir &dir : index.dirs) {
    for (const char *ext : kExtensions) {
      const QString file = name + QLatin1Char('.') + QLatin1String(ext);
      for (const QString &root : m_roots) {
        const QString p = root + QLatin1Char('/') + index.name + QLatin1Char('/') + dir.path + QLatin1Char('/') + file;
        if (isFile(p)) {
          (qstrcmp(ext, "svg") == 0 ? svgs : pngs) << Hit{&dir, p};
          break;
        }
      }
    }
  }
  for (const Hit &h : svgs)
    if (h.dir->kind == Kind::Scalable) return h.path;

  // The smallest directory at or above `size`, else the largest; the first in the theme's
  // order on a tie.
  auto nearest = [size](const QVector<Hit> &hits) -> QString {
    const Hit *above = nullptr;
    const Hit *largest = nullptr;
    for (const Hit &h : hits) {
      if (h.dir->pixels >= size && (!above || h.dir->pixels < above->dir->pixels)) above = &h;
      if (!largest || h.dir->pixels > largest->dir->pixels) largest = &h;
    }
    const Hit *pick = above ? above : largest;
    return pick ? pick->path : QString();
  };
  const QString svg = nearest(svgs);
  return svg.isEmpty() ? nearest(pngs) : svg;
}
