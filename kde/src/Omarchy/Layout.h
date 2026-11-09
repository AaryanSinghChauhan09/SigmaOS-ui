#pragma once

// Row and Column: box layouts with Qt Quick Layouts' rules, so a view built from kit widgets
// lands on the same pixels as its Qt Quick twin. A child gets its size hint (its fixed size when
// it has one); one whose size policy expands (QSizePolicy::Expanding, or setFill()) shares the
// room left along the main axis and fills the cross axis; others sit at the start of the cross
// axis, or centered in a Row (rounded to whole pixels as Qt Quick does). Height-for-width is
// honored in a Column.

#include <QLayout>
#include <QList>

#include "Omarchy/Export.h"

namespace Omarchy {

// Mark a widget to fill (Layout.fillWidth / fillHeight).
OMARCHY_EXPORT void setFill(QWidget *w, bool horizontal = true, bool vertical = false);

class OMARCHY_EXPORT BoxLayout : public QLayout {
  Q_OBJECT
public:
  explicit BoxLayout(Qt::Orientation o, QWidget *parent = nullptr);
  ~BoxLayout() override;

  void addItem(QLayoutItem *item) override;
  QLayoutItem *itemAt(int i) const override { return m_items.value(i); }
  QLayoutItem *takeAt(int i) override;
  int count() const override { return int(m_items.size()); }
  QSize sizeHint() const override;
  QSize minimumSize() const override;
  void setGeometry(const QRect &r) override;
  bool hasHeightForWidth() const override;
  int heightForWidth(int w) const override;
  Qt::Orientations expandingDirections() const override;

  void addWidget(QWidget *w, Qt::Alignment align = {});
  // A fixed gap (on top of the spacing) and a stretch that takes the room left.
  void addSpacing(int px);
  void addStretch();

private:
  QSize hint(int width) const;
  Qt::Orientation m_o;
  QList<QLayoutItem *> m_items;
};

class OMARCHY_EXPORT Row : public BoxLayout {
  Q_OBJECT
public:
  explicit Row(QWidget *parent = nullptr) : BoxLayout(Qt::Horizontal, parent) {}
};

class OMARCHY_EXPORT Column : public BoxLayout {
  Q_OBJECT
public:
  explicit Column(QWidget *parent = nullptr) : BoxLayout(Qt::Vertical, parent) {}
};

}  // namespace Omarchy
