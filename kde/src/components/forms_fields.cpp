// PasswordField and TextArea: TextField's chrome around a password line edit and a plain-text
// editor.

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPointer>
#include <QKeyEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextLayout>
#include <QVariantAnimation>
#include <cmath>

#include "Omarchy/Forms.h"
#include "Omarchy/Inputs.h"
#include "Omarchy/Paint.h"
#include "Omarchy/Style.h"

namespace Omarchy {

static int ceilPx(qreal v) { return int(std::ceil(v - 1e-6)); }

static QString glyph(char32_t cp) { return QString::fromUcs4(&cp, 1); }

// The input surface: focus > hover-cursor > normal glass and border (TextField's).
static State inputState(bool focus, bool hot) { return focus ? State::Focus : hot ? State::Hover : State::Normal; }

// --------------------------------------------------------------------------------- PasswordField

// Omarchy's style with Qt Quick's password character (•) and no mask delay, for the field.
class PasswordStyle : public Style {
public:
  int styleHint(StyleHint h, const QStyleOption *opt, const QWidget *w, QStyleHintReturn *ret) const override {
    if (h == SH_LineEdit_PasswordCharacter) return 0x2022;
    if (h == SH_LineEdit_PasswordMaskDelay) return 0;
    return Style::styleHint(h, opt, w, ret);
  }
};

static QStyle *passwordStyle() {
  static QPointer<QStyle> style;
  if (!style) {
    style = new PasswordStyle;
    style->setParent(qApp);
  }
  return style;
}

// The line edit: TextField's chrome, drawn in polkit's error colors when there's an error, with
// the lock before the dots and the eye at the end.
class PasswordEdit : public TextField {
public:
  explicit PasswordEdit(PasswordField *owner) : TextField(owner), m_owner(owner) {
    setMouseTracking(true);
    setEchoMode(QLineEdit::Password);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    // Qt Quick's passwordCharacter "•", where the style would give "●".
    setStyle(passwordStyle());
  }

  QRectF lockRect() const {
    const Tokens &tk = shown();
    const QFont f = tk.font("body");
    const QSizeF s = Paint::textSize(f, glyph(U''));
    const int x = tk.spacing("controlPaddingX") + tk.border(state()).left;
    return QRectF(x, Paint::centered(height(), s.height()), s.width(), s.height());
  }
  QRectF eyeRect() const {
    if (!m_owner->m_revealable) return QRectF();
    const Tokens &tk = shown();
    const QFont f = tk.font("body");
    const QSizeF s = Paint::textSize(f, eyeGlyph());
    const int right = tk.spacing("controlPaddingX") + tk.border(state()).right;
    return QRectF(width() - right - s.width(), Paint::centered(height(), s.height()), s.width(), s.height());
  }
  QString eyeGlyph() const { return m_owner->m_revealed ? glyph(U'\U000F0209') : glyph(U'\U000F0208'); }
  bool eyeHit(const QPointF &p) const {
    const QRectF r = eyeRect();
    const int xs = shown().spacing("xs");
    return r.isValid() && r.adjusted(-xs, -xs, xs, xs).contains(p);
  }

  // The padding before and after the text: the lock and the eye with controlGap to the text.
  void margins() {
    const Tokens &tk = shown();
    const QFont f = tk.font("body");
    const Border b = tk.border(State::Normal);
    const int px = tk.spacing("controlPaddingX"), py = tk.spacing("inputPaddingY"), gap = tk.spacing("controlGap");
    const qreal lock = Paint::textWidth(f, glyph(U''));
    const qreal eye = m_owner->m_revealable ? Paint::textWidth(f, eyeGlyph()) + gap : 0;
    // QLineEdit keeps 2 px beside and 1 px above its text; top-aligned, the line box then sits
    // at the padding as Qt Quick's does (centering would put it half a pixel lower).
    setTextMargins(int(std::lround(b.left + px + lock + gap)) - 2, b.top + py - 1, int(std::lround(b.right + px + eye)) - 2, b.bottom + py + 1);
  }

protected:
  State state() const { return inputState(hasFocus(), underMouse() || hasCursor()); }

  void paintEvent(QPaintEvent *e) override {
    const Tokens &tk = shown();
    {
      QPainter p(this);
      const State st = state();
      Border b = tk.border(st);
      const QColor error = tk.color(QStringLiteral("surfaces.polkit.textError"));
      if (m_owner->hasError()) b = Border::flat(tk.border(QStringLiteral("surfaces.polkit.borderError")).color, qMax(1, b.top));
      Paint::surface(p, QRectF(rect()), tk.radius(), tk.fill(st), b);
      if (m_owner->m_flash > 0)
        Paint::surface(p, QRectF(rect()), tk.radius(), Paint::alpha(error, tk.fill(State::Selected).alphaF() * m_owner->m_flash));
    }
    QLineEdit::paintEvent(e);
    QPainter p(this);
    const QFont f = tk.font("body");
    const QColor error = tk.color(QStringLiteral("surfaces.polkit.textError"));
    Paint::text(p, lockRect().topLeft(), f, m_owner->hasError() ? error : tk.textDim(), glyph(U''));
    if (m_owner->m_revealable) Paint::text(p, eyeRect().topLeft(), f, m_eyeHot ? tk.foreground() : tk.textDim(), eyeGlyph());
  }
  void mousePressEvent(QMouseEvent *e) override {
    if (e->button() == Qt::LeftButton && eyeHit(e->position())) {
      m_eyePressed = true;
      e->accept();
      return;
    }
    TextField::mousePressEvent(e);
  }
  void mouseReleaseEvent(QMouseEvent *e) override {
    if (m_eyePressed) {
      m_eyePressed = false;
      if (eyeHit(e->position())) m_owner->setRevealed(!m_owner->m_revealed);
      e->accept();
      return;
    }
    TextField::mouseReleaseEvent(e);
  }
  void mouseMoveEvent(QMouseEvent *e) override {
    const bool hot = eyeHit(e->position());
    if (hot != m_eyeHot) {
      m_eyeHot = hot;
      setCursor(hot ? Qt::PointingHandCursor : Qt::IBeamCursor);
      update();
    }
    if (!m_eyePressed) TextField::mouseMoveEvent(e);
  }
  void leaveEvent(QEvent *e) override {
    m_eyeHot = false;
    TextField::leaveEvent(e);
  }

private:
  PasswordField *m_owner;
  bool m_eyeHot = false, m_eyePressed = false;
};

PasswordField::PasswordField(QWidget *parent) : QWidget(parent) {
  follow(this);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  m_field = new PasswordEdit(this);
  setFocusProxy(m_field);
  connect(m_field, &QLineEdit::returnPressed, this, &PasswordField::accepted);
  connect(m_field, &QLineEdit::textChanged, this, &PasswordField::textChanged);
  // After TextField's own, which resets the margins.
  connect(Theme::instance(), &Theme::metricsChanged, this, &PasswordField::retheme);

  // polkit's shake: −8, +8, 0 over 35, 50 and 55 ms; and a flash of the error color.
  m_shakeAnim = new QVariantAnimation(this);
  m_shakeAnim->setStartValue(0.0);
  m_shakeAnim->setEndValue(0.0);
  connect(m_shakeAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
    m_shake = v.toReal();
    place();
  });
  m_flashAnim = new QVariantAnimation(this);
  m_flashAnim->setStartValue(1.0);
  m_flashAnim->setEndValue(0.0);
  m_flashAnim->setEasingCurve(QEasingCurve::OutQuad);
  connect(m_flashAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
    m_flash = v.toReal();
    m_field->update();
  });
  retheme();
}

void PasswordField::retheme() {
  m_field->margins();
  updateGeometry();
  place();
  update();
}

QString PasswordField::text() const { return m_field->text(); }
void PasswordField::setText(const QString &t) { m_field->setText(t); }
QString PasswordField::placeholderText() const { return m_field->placeholderText(); }
void PasswordField::setPlaceholderText(const QString &t) { m_field->setPlaceholderText(t); }
bool PasswordField::hasCursor() const { return m_field->hasCursor(); }
void PasswordField::setHasCursor(bool on) { m_field->setHasCursor(on); }
TextField *PasswordField::field() const { return m_field; }
QRectF PasswordField::eyeRect() const { return m_field->eyeRect(); }

void PasswordField::setRevealable(bool on) {
  m_revealable = on;
  retheme();
}

void PasswordField::setRevealed(bool on) {
  if (m_revealed == on) return;
  m_revealed = on;
  m_field->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
  m_field->margins();
  m_field->update();
  emit revealedChanged(on);
}

void PasswordField::setError(const QString &message) {
  m_error = message;
  updateGeometry();
  update();
  m_field->update();
  if (!message.isEmpty()) shake();
}

void PasswordField::shake() {
  const Tokens &tk = shown();
  const int ms = tk.duration(140);
  m_shakeAnim->stop();
  m_flashAnim->stop();
  if (ms <= 0) {
    m_shake = 0;
    m_flash = 0;
    place();
    m_field->update();
    return;
  }
  const qreal s = tk.space(8);
  m_shakeAnim->setDuration(ms);
  m_shakeAnim->setKeyValueAt(35.0 / 140, -s);
  m_shakeAnim->setKeyValueAt(85.0 / 140, s);
  m_shakeAnim->start();
  m_flashAnim->setDuration(tk.duration(400));
  m_flashAnim->start();
}

static qreal messageHeight(const QString &error, int width) {
  return Paint::wrappedHeight(shown().font("caption"), error, qMax(1, width));
}

int PasswordField::heightForWidth(int w) const {
  const int field = m_field->sizeHint().height();
  return hasError() ? ceilPx(field + shown().spacing("labelGap") + messageHeight(m_error, w)) : field;
}

QSize PasswordField::sizeHint() const {
  const int w = shown().space(240);
  return QSize(w, heightForWidth(w));
}

QSize PasswordField::minimumSizeHint() const { return QSize(0, heightForWidth(width() > 0 ? width() : shown().space(240))); }

void PasswordField::place() { m_field->setGeometry(int(std::lround(m_shake)), 0, width(), m_field->sizeHint().height()); }

void PasswordField::paintEvent(QPaintEvent *) {
  if (!hasError()) return;
  const Tokens &tk = shown();
  QPainter p(this);
  const qreal y = m_field->sizeHint().height() + tk.spacing("labelGap");
  Paint::wrapped(p, QRectF(0, y, width(), 1e6), tk.font("caption"), tk.color(QStringLiteral("surfaces.polkit.textError")), m_error);
}

// -------------------------------------------------------------------------------------- TextArea

// The editor's colors: text, placeholder, selection, nothing behind.
static void areaPalette(QPlainTextEdit *e) {
  const Tokens &tk = shown();
  QPalette pal = e->palette();
  for (const QPalette::ColorGroup g : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
    pal.setColor(g, QPalette::Text, g == QPalette::Disabled ? tk.textFaint() : tk.foreground());
    pal.setColor(g, QPalette::PlaceholderText, tk.textFaint());
    pal.setColor(g, QPalette::Highlight, tk.fill(State::Selection));
    pal.setColor(g, QPalette::HighlightedText, tk.foreground());
    pal.setColor(g, QPalette::Base, Qt::transparent);
    pal.setColor(g, QPalette::Window, Qt::transparent);
  }
  e->setPalette(pal);
}

TextArea::TextArea(QWidget *parent) : TextArea(QString(), parent) {}

TextArea::TextArea(const QString &text, QWidget *parent) : QWidget(parent) {
  follow(this);
  setAttribute(Qt::WA_Hover);
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  m_area = new QPlainTextEdit(this);
  m_area->setFrameShape(QFrame::NoFrame);
  m_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_area->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  m_area->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  m_area->viewport()->setAutoFillBackground(false);
  m_area->setAutoFillBackground(false);
  m_area->document()->setDocumentMargin(0);
  m_area->document()->setUseDesignMetrics(true);
  m_area->installEventFilter(this);
  setFocusProxy(m_area);
  m_area->setPlainText(text);
  connect(m_area, &QPlainTextEdit::textChanged, this, [this] {
    updateGeometry();
    update();
    emit textChanged();
  });
  connect(m_area->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] { update(); });
  connect(m_area->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this] { update(); });
  connect(Theme::instance(), &Theme::colorsChanged, m_area, [this] { areaPalette(m_area); });
  connect(Theme::instance(), &Theme::metricsChanged, this, &TextArea::retheme);
  retheme();
}

void TextArea::retheme() {
  m_area->setFont(shown().font("body"));
  m_area->document()->setDocumentMargin(0);
  areaPalette(m_area);
  updateGeometry();
  place();
  update();
}

QString TextArea::text() const { return m_area->toPlainText(); }
void TextArea::setText(const QString &t) { m_area->setPlainText(t); }
QString TextArea::placeholderText() const { return m_area->placeholderText(); }
void TextArea::setPlaceholderText(const QString &t) { m_area->setPlaceholderText(t); }

void TextArea::setMinRows(int n) {
  m_minRows = qMax(1, n);
  updateGeometry();
}
void TextArea::setMaxRows(int n) {
  m_maxRows = qMax(1, n);
  updateGeometry();
}
void TextArea::setHasCursor(bool on) {
  m_hasCursor = on;
  update();
}

// The padding inside the widest border: controlPaddingX × inputPaddingY.
QMargins TextArea::padding() const {
  const Tokens &tk = shown();
  const Border b = tk.border(State::Normal);
  const int px = tk.spacing("controlPaddingX"), py = tk.spacing("inputPaddingY");
  return QMargins(b.left + px, b.top + py, b.right + px, b.bottom + py);
}

// Wrapped lines at the editor's width, laid out as the editor lays them out.
int TextArea::lineCount() const {
  const QMargins m = padding();
  const qreal w = qMax(1, (width() > 0 ? width() : sizeHint().width()) - m.left() - m.right());
  const QFont f = shown().font("body");
  QTextOption opt;
  opt.setUseDesignMetrics(true);
  opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  int lines = 0;
  for (QTextBlock b = m_area->document()->begin(); b.isValid(); b = b.next()) {
    QTextLayout layout(b.text(), f);
    layout.setTextOption(opt);
    layout.beginLayout();
    for (;;) {
      QTextLine line = layout.createLine();
      if (!line.isValid()) break;
      line.setLineWidth(w);
      ++lines;
    }
    layout.endLayout();
  }
  return qMax(1, lines);
}

int TextArea::rows() const { return qBound(m_minRows, lineCount(), qMax(m_minRows, m_maxRows)); }

QSize TextArea::sizeHint() const {
  const QMargins m = padding();
  const qreal line = Paint::lineHeight(shown().font("body"));
  const int w = width() > 0 ? width() : shown().space(240);
  return QSize(w, ceilPx(rows() * line) + m.top() + m.bottom());
}

void TextArea::place() { m_area->setGeometry(rect().marginsRemoved(padding())); }

void TextArea::paintEvent(QPaintEvent *) {
  const Tokens &tk = shown();
  QPainter p(this);
  const State st = inputState(m_area->hasFocus(), m_hovered || m_hasCursor);
  Paint::surface(p, QRectF(rect()), tk.radius(), tk.fill(st), tk.border(st));
  // The scrollbar only when the text overflows: a thin textFaint thumb at the right edge.
  const QScrollBar *bar = m_area->verticalScrollBar();
  if (lineCount() > m_maxRows && bar->maximum() > bar->minimum()) {
    const int thick = tk.spacing("scrollbarThickness");
    const qreal track = height();
    const qreal total = bar->maximum() - bar->minimum() + bar->pageStep();
    const qreal len = qMax<qreal>(thick * 2, track * bar->pageStep() / total);
    const qreal y = (track - len) * (bar->value() - bar->minimum()) / qMax(1, bar->maximum() - bar->minimum());
    Paint::surface(p, QRectF(width() - thick, y, thick, len), tk.smallRadius(), tk.textFaint());
  }
}

bool TextArea::event(QEvent *e) {
  if (e->type() == QEvent::Enter || e->type() == QEvent::HoverEnter) {
    m_hovered = true;
    update();
  } else if (e->type() == QEvent::Leave || e->type() == QEvent::HoverLeave) {
    m_hovered = false;
    update();
  }
  return QWidget::event(e);
}

bool TextArea::eventFilter(QObject *o, QEvent *e) {
  if (o == m_area && (e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut)) update();
  return QWidget::eventFilter(o, e);
}

}  // namespace Omarchy
