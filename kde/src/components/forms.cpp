#include "Omarchy/Forms.h"

#include <QPainter>
#include <cmath>

#include "Omarchy/Paint.h"

namespace Omarchy {

static int ceilPx(qreal v) { return int(std::ceil(v - 1e-6)); }

// ------------------------------------------------------------------------------------ SettingRow

SettingRow::SettingRow(const QString &label, const QString &description, QWidget *parent)
    : Control(parent), m_label(label), m_description(description) {
  QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Fixed);
  sp.setHeightForWidth(true);
  setSizePolicy(sp);
  m_trailing = new QWidget(this);
  m_row = new Row(m_trailing);
  m_row->setSpacing(shown().spacing("md"));
}

void SettingRow::setLabel(const QString &t) {
  m_label = t;
  update();
}

void SettingRow::setDescription(const QString &t) {
  m_description = t;
  updateGeometry();
  place();
  update();
}

void SettingRow::addControl(QWidget *w) {
  w->setParent(m_trailing);
  m_row->addWidget(w);
  w->show();
  updateGeometry();
  place();
}

qreal SettingRow::wordsHeight(int width) const {
  const Tokens &tk = t();
  const int padX = tk.spacing("rowPaddingX");
  const qreal w = qMax<qreal>(0, width - padX * 2 - m_row->sizeHint().width() - tk.spacing("xxl"));
  qreal h = Paint::lineHeight(tk.font("body"));
  if (!m_description.isEmpty()) h += tk.spacing("xxs") + Paint::wrappedHeight(tk.font("caption"), m_description, qMax<qreal>(1, w));
  return h;
}

int SettingRow::heightForWidth(int w) const {
  const Tokens &tk = t();
  const qreal h = qMax<qreal>(qMax<qreal>(tk.spacing("controlHeight"), wordsHeight(w)), m_row->sizeHint().height());
  return ceilPx(h + tk.spacing("md") * 2);
}

QSize SettingRow::sizeHint() const {
  const int w = t().space(240);
  return QSize(w, heightForWidth(w));
}

void SettingRow::place() {
  const Tokens &tk = t();
  m_row->setSpacing(tk.spacing("md"));
  const QSize s = m_row->sizeHint();
  const int padX = tk.spacing("rowPaddingX");
  m_trailing->setGeometry(width() - padX - s.width(), int(Paint::centered(height(), s.height())), s.width(), s.height());
}

void SettingRow::paintEvent(QPaintEvent *) {
  const Tokens &tk = t();
  QPainter p(this);
  if (hasCursor()) Paint::surface(p, QRectF(rect()), tk.radius(), tk.fill(State::Hover));
  const int padX = tk.spacing("rowPaddingX");
  const qreal w = qMax<qreal>(0, width() - padX * 2 - m_row->sizeHint().width() - tk.spacing("xxl"));
  const qreal wh = wordsHeight(width());
  const qreal y = Paint::centered(height(), wh);
  const QFont body = tk.font("body");
  const qreal lh = Paint::lineHeight(body);
  Paint::textIn(p, QRectF(padX, y, w, lh), Qt::AlignLeft | Qt::AlignTop, body, isEnabled() ? tk.foreground() : tk.textFaint(), m_label);
  if (!m_description.isEmpty())
    Paint::wrapped(p, QRectF(padX, y + lh + tk.spacing("xxs"), w, 1e6), tk.font("caption"), isEnabled() ? tk.textDim() : tk.textFaint(), m_description);
}

}  // namespace Omarchy
