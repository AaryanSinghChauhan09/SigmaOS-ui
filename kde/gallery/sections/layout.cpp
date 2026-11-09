#include "gallery.h"

using namespace Omarchy;

namespace {

// Text centered in its box as Qt Quick's anchors.centerIn puts it.
class Centered : public QWidget {
public:
  explicit Centered(const QString &text) : m_text(text) { follow(this); }

protected:
  void paintEvent(QPaintEvent *) override {
    const Tokens &tk = shown();
    QPainter p(this);
    const QFont f = tk.font("body");
    const QSizeF s = Paint::textSize(f, m_text);
    Paint::text(p, QPointF(Paint::centered(width(), s.width()), Paint::centered(height(), s.height())), f, tk.textFaint(), m_text);
  }

private:
  QString m_text;
};

// The frame drawn over the split view: a hairline in `track` at the theme radius.
class Frame : public QWidget {
public:
  explicit Frame(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    follow(this);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    const Tokens &tk = shown();
    QPainter p(this);
    Paint::surface(p, QRectF(rect()), tk.radius(), Qt::transparent, Border::flat(tk.track(), tk.spacing("hairline")));
  }
};

class Box : public QWidget {
public:
  Box(SplitView *split, Frame *frame) : m_split(split), m_frame(frame) {}

protected:
  void resizeEvent(QResizeEvent *) override {
    m_split->setGeometry(rect());
    m_frame->setGeometry(rect());
  }

private:
  SplitView *m_split;
  Frame *m_frame;
};

void build(Column *body, QWidget *) {
  using G = QString;
  auto g = Gallery::glyph;

  auto *split = new SplitView;
  auto *sidebar = new Sidebar;
  sidebar->setSections({{G("Library"),
                         {{G("all"), G("All files"), g(0xF024B)},
                          {G("recent"), G("Recent"), g(0xF02DA)},
                          {G("starred"), G("Starred"), g(0xF04CE)},
                          {G("trash"), G("Trash"), g(0xF0A7A)}}},
                        {G("Places"), {{G("documents"), G("Documents"), g(0xF0219)}, {G("downloads"), G("Downloads"), g(0xF01DA)}}}});
  sidebar->setSelectedId(G("recent"));
  split->addPane(sidebar, 200, 120);
  split->addPane(new Centered(G("No file selected")), -1, -1, -1, true);

  auto *frame = new Frame(nullptr);
  auto *box = new Box(split, frame);
  split->setParent(box);
  frame->setParent(box);
  frame->raise();
  box->setFixedSize(720, 220);
  body->addWidget(box);

  auto *calendar = new Calendar;
  calendar->setMonth(2026, 11);
  calendar->setToday(QDate(2026, 11, 8));
  calendar->setSelected(QDate(2026, 11, 14));
  body->addWidget(calendar);
}

Gallery::Register reg("layout", "Layout", build);

}  // namespace
