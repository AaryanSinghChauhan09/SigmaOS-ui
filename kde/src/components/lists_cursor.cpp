// CursorModel: the keyboard cursor of qt/qml/Omarchy/Ui/CursorModel.qml (and its
// PointerMoveGate), case for case.

#include <QKeyEvent>
#include <QWidget>
#include <cmath>

#include "Omarchy/Lists.h"

namespace Omarchy {

namespace {
// Emits CursorModel::changed() on scope exit when the section, index or visibility moved.
struct ChangeGuard {
  CursorModel *m;
  QString section;
  int index;
  bool active;
  explicit ChangeGuard(CursorModel *model)
      : m(model), section(model->focusSection()), index(model->selectedIndex()), active(model->isCursorActive()) {}
  ~ChangeGuard() {
    if (section != m->focusSection() || index != m->selectedIndex() || active != m->isCursorActive()) emit m->changed();
  }
};
}  // namespace

CursorModel::CursorModel(QObject *parent) : QObject(parent) { m_sections = {{QString(), 0}}; }

void CursorModel::setSections(const QList<Section> &sections) {
  ChangeGuard g(this);
  m_sections = sections;
  // As Qt Quick's binding: the first section's name until something moves the cursor.
  if (!m_pinned) m_section = m_sections.isEmpty() ? QString() : m_sections[0].name;
  if (m_active) settle();
}

int CursorModel::sectionIndex(const QString &name) const {
  for (int i = 0; i < m_sections.size(); ++i)
    if (m_sections[i].name == name) return i;
  return -1;
}

int CursorModel::count(const QString &name) const {
  const int i = sectionIndex(name);
  return i < 0 ? 0 : m_sections[i].count;
}

int CursorModel::neighbour(int from, int dir) const {
  for (int i = from + dir; i >= 0 && i < m_sections.size(); i += dir)
    if (m_sections[i].count > 0) return i;
  return -1;
}

bool CursorModel::isAt(const QString &section, int index) const { return m_active && m_section == section && m_index == index; }

void CursorModel::set(const QString &section, int index) {
  if (sectionIndex(section) < 0) return;
  ChangeGuard g(this);
  m_pinned = true;
  m_section = section;
  m_index = index;
  m_active = true;
  settle();
}

void CursorModel::reset() {
  ChangeGuard g(this);
  m_active = false;
  resetGate();
}

void CursorModel::setCount(const QString &section, int count) {
  const int i = sectionIndex(section);
  if (i < 0) return;
  ChangeGuard g(this);
  m_sections[i].count = count;
  settle();
}

// Keep the cursor on a real row: off an empty section, within the count.
void CursorModel::settle() {
  const int s = sectionIndex(m_section);
  if (s < 0 || m_sections[s].count == 0) {
    int n = neighbour(qMax(0, s), 1);
    if (n < 0) n = neighbour(qMax(0, s), -1);
    if (n < 0 && s < 0 && !m_sections.isEmpty()) n = 0;
    if (n >= 0) {
      m_pinned = true;
      m_section = m_sections[n].name;
      m_index = 0;
    }
  }
  m_index = qMax(0, qMin(m_index, count(m_section) - 1));
}

// Show a hidden cursor; false when it was hidden (the key only woke it).
bool CursorModel::wake() {
  resetGate();
  if (m_active) return true;
  m_active = true;
  settle();
  return false;
}

void CursorModel::announce(bool byKey) { emit moved(m_section, m_index, byKey); }

void CursorModel::move(int dx, int dy) {
  {
    ChangeGuard g(this);
    const int start = sectionIndex(m_section);
    if (wake() && start >= 0) step(dx, dy);
  }
  announce(true);
}

void CursorModel::step(int dx, int dy) {
  int s = sectionIndex(m_section);
  int cnt = count(m_section);
  int index = m_index;
  if (dx != 0 && m_columns > 1) {
    const int col = index % m_columns + dx;
    if (col >= 0 && col < m_columns && index + dx >= 0 && index + dx < cnt) index += dx;
  }
  for (int i = 0; i < std::abs(dy); ++i) {
    const int next = index + m_columns * (dy > 0 ? 1 : -1);
    if (next >= 0 && next < cnt) {
      index = next;
      continue;
    }
    const int n = neighbour(s, dy > 0 ? 1 : -1);
    if (n >= 0) {
      const int c = m_sections[n].count;
      if (dy > 0) index = qMin(index % m_columns, c - 1);
      else index = qMin((c - 1) / m_columns * m_columns + index % m_columns, c - 1);
      s = n;
      cnt = c;
    } else if (dy > 0 && m_columns > 1 && index / m_columns < (cnt - 1) / m_columns) {
      index = cnt - 1;
    }
  }
  m_pinned = true;
  m_section = m_sections[s].name;
  m_index = index;
}

void CursorModel::moveSection(int delta) {
  {
    ChangeGuard g(this);
    if (wake()) {
      const int n = neighbour(sectionIndex(m_section), delta > 0 ? 1 : -1);
      if (n < 0) return;
      m_pinned = true;
      m_section = m_sections[n].name;
      m_index = 0;
    }
  }
  announce(true);
}

void CursorModel::page(int delta) {
  {
    ChangeGuard g(this);
    if (wake()) {
      const int last = count(m_section) - 1;
      m_index = qMax(0, qMin(last, m_index + qMax(1, m_pageSize) * m_columns * (delta > 0 ? 1 : -1)));
    }
  }
  announce(true);
}

void CursorModel::home() {
  {
    ChangeGuard g(this);
    m_active = true;
    resetGate();
    const int n = neighbour(-1, 1);
    if (n >= 0) {
      m_pinned = true;
      m_section = m_sections[n].name;
      m_index = 0;
    }
  }
  announce(true);
}

void CursorModel::end() {
  {
    ChangeGuard g(this);
    m_active = true;
    resetGate();
    const int n = neighbour(int(m_sections.size()), -1);
    if (n >= 0) {
      m_pinned = true;
      m_section = m_sections[n].name;
      m_index = m_sections[n].count - 1;
    }
  }
  announce(true);
}

QString CursorModel::printable(const QKeyEvent *event) {
  if (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier)) return QString();
  const QString t = event->text();
  if (t.size() < 1 || t.size() > 2) return QString();
  if (t.size() == 2 && !t.at(0).isHighSurrogate()) return QString();
  if (t.size() == 1) {
    const char16_t c = t.at(0).unicode();
    if (c < 0x20 || c == 0x7f || t.at(0).isSpace()) return QString();
  }
  return t;
}

bool CursorModel::handleKey(QKeyEvent *event) {
  const bool plain = !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
  const int k = event->key();
  const QString t = plain ? event->text() : QString();
  bool done = true;
  if (k == Qt::Key_Down || t == QLatin1String("j")) move(0, 1);
  else if (k == Qt::Key_Up || t == QLatin1String("k")) move(0, -1);
  else if (k == Qt::Key_Right || t == QLatin1String("l")) move(1, 0);
  else if (k == Qt::Key_Left || t == QLatin1String("h")) move(-1, 0);
  else if (k == Qt::Key_PageDown) page(1);
  else if (k == Qt::Key_PageUp) page(-1);
  else if (k == Qt::Key_Home) home();
  else if (k == Qt::Key_End) end();
  else if (k == Qt::Key_Tab || k == Qt::Key_Backtab)
    moveSection((event->modifiers() & Qt::ShiftModifier) || k == Qt::Key_Backtab ? -1 : 1);
  else if (k == Qt::Key_Return || k == Qt::Key_Enter || k == Qt::Key_Space) emit activated(m_section, m_index);
  else if (k == Qt::Key_Escape) emit closeRequested();
  else if (k == Qt::Key_Delete || t == QLatin1String("x") || t == QLatin1String("X")) emit deleteRequested(m_section, m_index);
  else {
    const QString typed = printable(event);
    if (!typed.isEmpty()) emit textTyped(typed);
    else done = false;
  }
  if (done) event->accept();
  return done;
}

// --------------------------------------------------------------------------------- the pointer

void CursorModel::resetGate() {
  m_primed = false;
  m_initialAllowed = false;
  m_last = QPointF();
}

void CursorModel::allowInitialSample() {
  resetGate();
  m_initialAllowed = true;
}

bool CursorModel::pointerMoved(QWidget *item, const QPointF &pos, const QString &section, int index) {
  if (!item) {
    resetGate();
    return false;
  }
  QWidget *target = m_reference ? m_reference.data() : item;
  const QPointF point = target == item ? pos : QPointF(target->mapFromGlobal(item->mapToGlobal(pos)));
  const bool first = !m_primed;
  const bool didMove = !first ? std::abs(point.x() - m_last.x()) > 1 || std::abs(point.y() - m_last.y()) > 1 : m_initialAllowed;
  if (first || didMove) m_last = point;
  m_primed = true;
  m_initialAllowed = false;
  if (!didMove) return false;
  if (isAt(section, index)) return true;
  set(section, index);
  announce(false);
  return true;
}

void CursorModel::attach(Control *item, const QString &section, int index) {
  if (!item) return;
  item->setHasCursor(isAt(section, index));
  QPointer<Control> guard(item);
  connect(this, &CursorModel::changed, item, [this, guard, section, index] {
    if (guard) guard->setHasCursor(isAt(section, index));
  });
  if (auto *row = qobject_cast<ListRow *>(item))
    connect(row, &ListRow::pointerMoved, this, [this, row, section, index](const QPointF &p) { pointerMoved(row, p, section, index); });
}

void CursorModel::attach(TileGrid *grid, const QString &section) {
  if (!grid) return;
  auto sync = [this, grid, section] { grid->setCursorIndex(cursorIn(section) ? m_index : -1); };
  sync();
  connect(this, &CursorModel::changed, grid, sync);
  connect(grid, &TileGrid::pointerMoved, this, [this, grid, section](int i, const QPointF &p) { pointerMoved(grid, p, section, i); });
}

}  // namespace Omarchy
