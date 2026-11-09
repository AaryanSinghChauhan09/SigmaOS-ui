// Keycap, KeyCombo, ShortcutSheet and the Tooltip with a shortcut (Keycap.qml, KeyCombo.qml,
// ShortcutSheet.qml, Tooltip.qml).

#include <QEvent>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QTimer>
#include <QVariantAnimation>
#include <cmath>

#include "Omarchy/Navigation.h"
#include "Omarchy/Paint.h"

namespace Omarchy {

namespace NavDetail {
void glyphByInk(QPainter &p, const QRectF &box, const QString &text, qreal fontSize, const QColor &color);
int ceilPx(qreal v);
}  // namespace NavDetail
using NavDetail::ceilPx;
using NavDetail::glyphByInk;

// ------------------------------------------------------------------------------------ key names

QString keyLabel(const QString &key) {
  const QString s = key.trimmed();
  static const QHash<QString, QString> names = {
      {QStringLiteral("super"), QStringLiteral("Super")},   {QStringLiteral("meta"), QStringLiteral("Super")},
      {QStringLiteral("win"), QStringLiteral("Super")},     {QStringLiteral("cmd"), QStringLiteral("Super")},
      {QStringLiteral("logo"), QStringLiteral("Super")},    {QStringLiteral("ctrl"), QStringLiteral("Ctrl")},
      {QStringLiteral("control"), QStringLiteral("Ctrl")},  {QStringLiteral("alt"), QStringLiteral("Alt")},
      {QStringLiteral("option"), QStringLiteral("Alt")},    {QStringLiteral("shift"), QStringLiteral("⇧")},
      {QStringLiteral("enter"), QStringLiteral("⏎")},       {QStringLiteral("return"), QStringLiteral("⏎")},
      {QStringLiteral("backspace"), QStringLiteral("⌫")},   {QStringLiteral("esc"), QStringLiteral("Esc")},
      {QStringLiteral("escape"), QStringLiteral("Esc")},    {QStringLiteral("tab"), QStringLiteral("Tab")},
      {QStringLiteral("space"), QStringLiteral("Space")},   {QStringLiteral("up"), QStringLiteral("↑")},
      {QStringLiteral("down"), QStringLiteral("↓")},        {QStringLiteral("left"), QStringLiteral("←")},
      {QStringLiteral("right"), QStringLiteral("→")},       {QStringLiteral("pageup"), QStringLiteral("PgUp")},
      {QStringLiteral("pagedown"), QStringLiteral("PgDn")}, {QStringLiteral("delete"), QStringLiteral("Del")},
      {QStringLiteral("del"), QStringLiteral("Del")},       {QStringLiteral("home"), QStringLiteral("Home")},
      {QStringLiteral("end"), QStringLiteral("End")},
  };
  const auto it = names.constFind(s.toLower());
  if (it != names.constEnd()) return *it;
  return s.size() == 1 ? s.toUpper() : s;
}

QStringList splitCombo(const QString &combo) {
  const QStringList parts = combo.split(QLatin1Char('+'));
  QStringList out;
  for (int i = 0; i < parts.size(); ++i) {
    if (!parts[i].isEmpty()) out << parts[i].trimmed();
    else if (i + 1 < parts.size() && parts[i + 1].isEmpty()) {
      out << QStringLiteral("+");
      ++i;
    }
  }
  return out;
}

// --------------------------------------------------------------------------------------- Keycap

Keycap::Keycap(const QString &key, QWidget *parent) : QWidget(parent), m_key(key) {
  follow(this);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void Keycap::setKey(const QString &k) {
  m_key = k;
  updateGeometry();
  update();
}
void Keycap::setForeground(const QColor &c) {
  m_fg = c;
  update();
}

QSizeF Keycap::capSize(const QString &key) {
  const Tokens &tk = shown();
  const Border b = tk.border(State::Normal);
  const qreal h = tk.space(18);
  const qreal advance = std::ceil(Paint::textWidth(tk.font("caption"), keyLabel(key)) - 1e-6);
  return QSizeF(qMax(h, advance + tk.spacing("sm") * 2 + b.left + b.right), h);
}

void Keycap::paintCap(QPainter &p, const QRectF &r, const QString &key, const QColor &foreground) {
  const Tokens &tk = shown();
  const QColor fg = foreground.isValid() ? foreground : tk.foreground();
  Paint::surface(p, r, tk.smallRadius(), tk.fill(State::Normal), foreground.isValid() ? tk.border(State::Normal, fg) : tk.border(State::Normal));
  glyphByInk(p, r, keyLabel(key), tk.fontSize("caption"), fg);
}

QSize Keycap::sizeHint() const {
  const QSizeF s = capSize(m_key);
  return QSize(ceilPx(s.width()), ceilPx(s.height()));
}

void Keycap::paintEvent(QPaintEvent *) {
  QPainter p(this);
  paintCap(p, QRectF(QPointF(0, 0), capSize(m_key)), m_key, m_fg);
}

// ------------------------------------------------------------------------------------- KeyCombo

KeyCombo::KeyCombo(const QString &keys, QWidget *parent) : QWidget(parent), m_keys(keys) {
  follow(this);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void KeyCombo::setKeys(const QString &k) {
  m_keys = k;
  updateGeometry();
  update();
}

QSizeF KeyCombo::comboSize(const QString &keys) {
  const QStringList parts = splitCombo(keys);
  qreal w = 0, h = 0;
  for (const QString &k : parts) {
    const QSizeF s = Keycap::capSize(k);
    w += s.width();
    h = qMax(h, s.height());
  }
  if (parts.size() > 1) w += shown().spacing("xxs") * (parts.size() - 1);
  return QSizeF(w, h);
}

// Qt Quick's Row: caps at their exact widths, xxs apart, at the top.
void KeyCombo::paintCombo(QPainter &p, const QPointF &topLeft, const QString &keys) {
  qreal x = topLeft.x();
  const qreal gap = shown().spacing("xxs");
  for (const QString &k : splitCombo(keys)) {
    const QSizeF s = Keycap::capSize(k);
    Keycap::paintCap(p, QRectF(QPointF(x, topLeft.y()), s), k);
    x += s.width() + gap;
  }
}

QSize KeyCombo::sizeHint() const {
  const QSizeF s = comboSize(m_keys);
  return QSize(ceilPx(s.width()), ceilPx(s.height()));
}

void KeyCombo::paintEvent(QPaintEvent *) {
  QPainter p(this);
  paintCombo(p, QPointF(0, 0), m_keys);
}

// -------------------------------------------------------------------------------- ShortcutSheet

ShortcutSheet::ShortcutSheet(QWidget *parent) : QWidget(parent) {
  follow(this);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void ShortcutSheet::setGroups(const QList<Group> &groups) {
  m_groups = groups;
  updateGeometry();
  update();
}
void ShortcutSheet::addGroup(const QString &title, const QList<QPair<QString, QString>> &keys) {
  m_groups << Group{title, keys};
  updateGeometry();
  update();
}
void ShortcutSheet::setColumns(int n) {
  m_columns = qMax(1, n);
  updateGeometry();
  update();
}

// Lays the sheet out (and paints it when `p` is given): a Row of Columns of groups, each a
// section header over a two-column Grid. Returns its implicit size.
QSizeF ShortcutSheet::layout(QPainter *p) const {
  const Tokens &tk = shown();
  const int caption = tk.fontSize("caption");
  const QFont header = tk.fontPx(caption, true), body = tk.font("body");
  const qreal headerPad = std::ceil(caption * 0.15), md = tk.spacing("md"), xxl = tk.spacing("xxl");
  const qreal colGap = tk.spacing("panelGap") * 2, groupGap = tk.spacing("panelGap");
  const qreal bodyLine = Paint::lineHeight(body), headerLine = Paint::lineHeight(header);
  const int columns = qMax(1, qMin(m_columns, int(m_groups.size())));
  qreal x = 0, height = 0;
  for (int c = 0; c < columns; ++c) {
    qreal y = 0, colWidth = 0;
    bool first = true;
    for (int g = c; g < m_groups.size(); g += m_columns) {
      const Group &group = m_groups[g];
      if (!first) y += groupGap;
      first = false;
      const QString title = group.title;
      if (p) Paint::text(*p, QPointF(x, y + headerPad), header, tk.textDim(), title);
      colWidth = qMax(colWidth, Paint::textWidth(header, title));
      y += headerLine + headerPad + md;
      qreal comboCol = 0, descCol = 0;
      for (const auto &k : group.keys) {
        comboCol = qMax(comboCol, KeyCombo::comboSize(k.first).width());
        descCol = qMax<qreal>(descCol, ceilPx(Paint::textWidth(body, k.second)));
      }
      for (int i = 0; i < group.keys.size(); ++i) {
        const auto &k = group.keys[i];
        const QSizeF cs = KeyCombo::comboSize(k.first);
        const qreal rowH = qMax(cs.height(), bodyLine);
        if (i > 0) y += md;
        if (p) {
          KeyCombo::paintCombo(*p, QPointF(x, y + (rowH - cs.height()) / 2), k.first);
          Paint::text(*p, QPointF(x + comboCol + xxl, y + (rowH - bodyLine) / 2), body, tk.foreground(), k.second);
        }
        y += rowH;
      }
      if (!group.keys.isEmpty()) colWidth = qMax(colWidth, comboCol + xxl + descCol);
    }
    height = qMax(height, y);
    x += colWidth + (c + 1 < columns ? colGap : 0);
  }
  return QSizeF(x, height);
}

QSizeF ShortcutSheet::implicitSize() const { return layout(nullptr); }

QSize ShortcutSheet::sizeHint() const {
  const QSizeF s = implicitSize();
  return QSize(ceilPx(s.width()), ceilPx(s.height()));
}

void ShortcutSheet::paintEvent(QPaintEvent *) {
  QPainter p(this);
  layout(&p);
}

// -------------------------------------------------------------------------------------- Tooltip

static const Qt::WindowFlags kTipFlags = Qt::ToolTip | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint;

Tooltip::Tooltip(const QString &text, QWidget *parent) : QWidget(parent, kTipFlags), m_text(text) {
  follow(this);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_ShowWithoutActivating);
  m_timer = new QTimer(this);
  m_timer->setSingleShot(true);
  connect(m_timer, &QTimer::timeout, this, &Tooltip::popup);
  m_fade = new QVariantAnimation(this);
  connect(m_fade, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
    setWindowOpacity(v.toReal());
    if (v.toReal() <= 0 && m_fade->endValue().toReal() <= 0) hide();
  });
  if (parent) watch(parent);
}

void Tooltip::watch(QWidget *w) {
  if (m_watched) m_watched->removeEventFilter(this);
  m_watched = w;
  if (w) w->installEventFilter(this);
}

void Tooltip::setText(const QString &t) {
  m_text = t;
  updateGeometry();
  if (!m_held && isVisible()) resize(sizeHint());
  update();
}
void Tooltip::setShortcut(const QString &s) {
  m_shortcut = s;
  updateGeometry();
  if (!m_held && isVisible()) resize(sizeHint());
  update();
}
void Tooltip::setDelay(int ms) { m_delay = ms; }

void Tooltip::setHeld(bool on) {
  if (m_held == on) return;
  m_held = on;
  m_timer->stop();
  m_fade->stop();
  setWindowFlags(on ? Qt::WindowFlags(Qt::Widget) : kTipFlags);
  setAttribute(Qt::WA_TranslucentBackground, !on);
  if (on) setAttribute(Qt::WA_NoSystemBackground, false);
  setWindowOpacity(1);
  if (!on) hide();
  else if (parentWidget()) show();
}

// Centered above the parent, sm away, kept sm inside the screen.
void Tooltip::popup() {
  if (m_held) return;
  QWidget *anchor = m_watched ? m_watched.data() : parentWidget();
  if (!anchor || m_text.isEmpty()) return;
  const Tokens &tk = shown();
  resize(sizeHint());
  QPoint at = anchor->mapToGlobal(QPoint(int(std::floor((anchor->width() - width()) / 2.0 + 0.5)), -height() - tk.spacing("sm")));
  if (QScreen *s = anchor->screen()) {
    const QRect a = s->availableGeometry().adjusted(tk.spacing("sm"), tk.spacing("sm"), -tk.spacing("sm"), -tk.spacing("sm"));
    at.setX(qMax(a.left(), qMin(at.x(), a.right() + 1 - width())));
    at.setY(qMax(a.top(), qMin(at.y(), a.bottom() + 1 - height())));
  }
  move(at);
  m_fade->stop();
  const int ms = tk.duration(120);
  setWindowOpacity(ms > 0 ? 0 : 1);
  show();
  raise();
  if (ms > 0) {
    m_fade->setDuration(ms);
    m_fade->setStartValue(0.0);
    m_fade->setEndValue(1.0);
    m_fade->start();
  }
}

void Tooltip::dismiss() {
  m_timer->stop();
  if (m_held || !isVisible()) return;
  const int ms = shown().duration(120);
  m_fade->stop();
  if (ms <= 0) {
    hide();
    return;
  }
  m_fade->setDuration(ms);
  m_fade->setStartValue(windowOpacity());
  m_fade->setEndValue(0.0);
  m_fade->start();
}

bool Tooltip::eventFilter(QObject *o, QEvent *e) {
  if (o == m_watched && !m_held) {
    switch (e->type()) {
    case QEvent::Enter: m_timer->start(m_delay); break;
    case QEvent::Leave:
    case QEvent::MouseButtonPress:
    case QEvent::Hide:
    case QEvent::KeyPress: dismiss(); break;
    default: break;
    }
  }
  return QWidget::eventFilter(o, e);
}

QSizeF Tooltip::implicitSize() const {
  const Tokens &tk = shown();
  const Border b = tk.border(QStringLiteral("surfaces.tooltip.border"));
  const QSizeF ts = Paint::textSize(tk.font("bodySmall"), m_text);
  qreal w = ts.width(), h = ts.height();
  if (!m_shortcut.isEmpty()) {
    const QSizeF cs = KeyCombo::comboSize(m_shortcut);
    w += tk.spacing("controlGap") + cs.width();
    h = qMax(h, cs.height());
  }
  return QSizeF(w + b.left + b.right + tk.spacing("controlPaddingX") * 2, h + b.top + b.bottom + tk.spacing("controlPaddingY") * 2);
}

QSize Tooltip::sizeHint() const {
  const QSizeF s = implicitSize();
  return QSize(ceilPx(s.width()), ceilPx(s.height()));
}

void Tooltip::paintEvent(QPaintEvent *) {
  const Tokens &tk = shown();
  QPainter p(this);
  const Border b = tk.border(QStringLiteral("surfaces.tooltip.border"));
  // The popup's box covers whole pixels, as Qt Quick's ToolTip lands.
  Paint::surface(p, QRectF(rect()), tk.radius(), tk.color(QStringLiteral("surfaces.tooltip.background")), b);
  const QFont f = tk.font("bodySmall");
  const QSizeF ts = Paint::textSize(f, m_text);
  const QSizeF cs = m_shortcut.isEmpty() ? QSizeF() : KeyCombo::comboSize(m_shortcut);
  const qreal rowH = qMax(ts.height(), cs.height());
  const qreal x = b.left + tk.spacing("controlPaddingX"), y = b.top + tk.spacing("controlPaddingY");
  Paint::text(p, QPointF(x, y + Paint::centered(rowH, ts.height())), f, tk.color(QStringLiteral("surfaces.tooltip.text")), m_text);
  if (!m_shortcut.isEmpty()) KeyCombo::paintCombo(p, QPointF(x + ts.width() + tk.spacing("controlGap"), y + Paint::centered(rowH, cs.height())), m_shortcut);
}

}  // namespace Omarchy
