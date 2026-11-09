#include "gallery.h"

using namespace Omarchy;

namespace {

// Rows reordered by their grips: the second under the cursor, a drop slot between the third
// and fourth.
class Rows : public QWidget {
public:
  explicit Rows(QWidget *parent = nullptr) : QWidget(parent) {
    follow(this);
    const int row = shown().spacing("popupRowHeight");
    setFixedSize(360, row * 4);
    for (int i = 0; i < 4; ++i) {
      auto *grip = new DragHandle(this);
      grip->setHasCursor(i == 1);
      const QSize s = grip->sizeHint();
      grip->setGeometry(shown().spacing("sm"), i * row + int(Paint::centered(row, s.height())), s.width(), s.height());
      m_grips << grip;
    }
    auto *drop = new DropIndicator(this);
    drop->placeAt(0, row * 3, 360);
  }

protected:
  void paintEvent(QPaintEvent *) override {
    static const char *const labels[] = {"Workspaces", "Clock", "Battery", "Network"};
    const Tokens &tk = shown();
    QPainter p(this);
    const int row = tk.spacing("popupRowHeight");
    const QFont body = tk.font("body");
    for (int i = 0; i < 4; ++i) {
      const QRectF r(0, i * row, width(), row);
      if (i == 1) Paint::surface(p, r, tk.radius(), tk.fill(State::Hover));
      const QWidget *grip = m_grips[i];
      Paint::text(p, QPointF(grip->x() + grip->width() + tk.spacing("sm"), r.y() + Paint::centered(row, Paint::lineHeight(body))), body,
                  tk.foreground(), QString::fromLatin1(labels[i]));
    }
  }

private:
  QList<QWidget *> m_grips;
};

void build(Column *body, QWidget *parent) {
  auto *timeline = new Timeline(parent);
  timeline->setDuration(30);
  timeline->setPosition(12);
  timeline->setTracks({{{0, 7.5, QStringLiteral("Intro")}, {8, 18, QStringLiteral("Demo")}, {18.5, 30, QStringLiteral("Outro")}},
                       {{2, 12, QStringLiteral("Voiceover")}, {14, 26, QStringLiteral("Music")}}});
  timeline->setSelected(0, 1);
  timeline->setFixedSize(720, 160);
  QObject::connect(timeline, &Timeline::seek, timeline, &Timeline::setPosition);
  QObject::connect(timeline, &Timeline::clipPressed, timeline, &Timeline::setSelected);
  body->addWidget(timeline);

  auto *scrubber = new Scrubber(parent);
  scrubber->setPosition(42);
  scrubber->setDuration(130);
  scrubber->setFixedWidth(360);
  body->addWidget(scrubber);

  Row *row = Gallery::row(body, parent, Gallery::sp("huge"), Qt::AlignTop);
  row->addWidget(new Rows, Qt::AlignTop);
  QWidget *image = Gallery::painted(QSize(240, 140), [](QPainter &p, const QRect &r) { Paint::surface(p, QRectF(r), 0, shown().fill(State::Normal)); });
  auto *crop = new CropFrame(image);
  crop->setGeometry(image->rect());
  row->addWidget(image, Qt::AlignTop);
}

Gallery::Register reg("timeline", "Timeline", build);

}  // namespace
