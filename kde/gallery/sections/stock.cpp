// Not a spec/gallery.md section: Qt's own widgets under Omarchy::Style, as any Qt Widgets or KDE
// app gets them from the one call.

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QTabBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  Row *r = Gallery::row(body, parent, Gallery::sp("controlGap"));
  r->addWidget(new QPushButton(QStringLiteral("Push")));
  auto *def = new QPushButton(QStringLiteral("Checked"));
  def->setCheckable(true);
  def->setChecked(true);
  r->addWidget(def);
  auto *flat = new QPushButton(QStringLiteral("Flat"));
  flat->setFlat(true);
  r->addWidget(flat);
  auto *off = new QPushButton(QStringLiteral("Disabled"));
  off->setEnabled(false);
  r->addWidget(off);
  auto *tool = new QToolButton;
  tool->setText(QStringLiteral("Tool"));
  tool->setAutoRaise(true);
  r->addWidget(tool);

  Row *r2 = Gallery::row(body, parent, Gallery::sp("controlGap"));
  auto *edit = new QLineEdit;
  edit->setPlaceholderText(QStringLiteral("Search…"));
  edit->setFixedWidth(200);
  r2->addWidget(edit);
  auto *combo = new QComboBox;
  combo->addItems({QStringLiteral("Tokyo Night"), QStringLiteral("Nord"), QStringLiteral("Gruvbox")});
  combo->setFixedWidth(200);
  r2->addWidget(combo);
  auto *spin = new QSpinBox;
  spin->setRange(0, 40);
  spin->setValue(10);
  spin->setAlignment(Qt::AlignCenter);
  spin->setFixedWidth(shown().spacing("numberFieldWidth"));
  r2->addWidget(spin);

  Row *r3 = Gallery::row(body, parent, Gallery::sp("xxl"));
  auto *check = new QCheckBox(QStringLiteral("Check"));
  check->setChecked(true);
  r3->addWidget(check);
  r3->addWidget(new QCheckBox(QStringLiteral("Unchecked")));
  auto *radio = new QRadioButton(QStringLiteral("Radio"));
  radio->setChecked(true);
  r3->addWidget(radio);
  r3->addWidget(new QRadioButton(QStringLiteral("Other")));
  auto *slider = new QSlider(Qt::Horizontal);
  slider->setRange(0, 100);
  slider->setValue(35);
  slider->setFixedWidth(200);
  r3->addWidget(slider);

  Row *r4 = Gallery::row(body, parent, Gallery::sp("xxl"));
  auto *bar = new QProgressBar;
  bar->setRange(0, 100);
  bar->setValue(60);
  bar->setFixedWidth(200);
  r4->addWidget(bar);
  auto *tabs = new QTabBar;
  tabs->addTab(QStringLiteral("General"));
  tabs->addTab(QStringLiteral("Appearance"));
  tabs->addTab(QStringLiteral("Keys"));
  tabs->setCurrentIndex(1);
  tabs->setDrawBase(false);
  r4->addWidget(tabs);

  Row *r5 = Gallery::row(body, parent, Gallery::sp("xxl"));
  auto *group = new QGroupBox(QStringLiteral("Group"));
  auto *gl = new QVBoxLayout(group);
  gl->addWidget(new QCheckBox(QStringLiteral("Inside a group")));
  group->setFixedWidth(200);
  r5->addWidget(group);
  auto *list = new QTreeWidget;
  list->setHeaderLabels({QStringLiteral("Name"), QStringLiteral("Size")});
  for (const char *n : {"Documents", "Pictures", "Music", "Videos", "Downloads", "Projects"}) {
    auto *it = new QTreeWidgetItem(list, {QString::fromLatin1(n), QStringLiteral("12 MB")});
    if (QLatin1String(n) == QLatin1String("Pictures")) list->setCurrentItem(it);
  }
  list->setFixedSize(260, 150);
  r5->addWidget(list);
  auto *menu = new QMenu;
  menu->addAction(QStringLiteral("Open"))->setShortcut(QKeySequence(QStringLiteral("Ctrl+O")));
  auto *checked = menu->addAction(QStringLiteral("Wrap lines"));
  checked->setCheckable(true);
  checked->setChecked(true);
  menu->addSeparator();
  menu->addMenu(QStringLiteral("Recent"));
  menu->addAction(QStringLiteral("Quit"));
  // A menu as it draws, held open in the page.
  menu->setParent(parent, Qt::Widget);
  menu->setActiveAction(menu->actions().first());
  menu->setFixedSize(menu->sizeHint());
  r5->addWidget(menu);
}

Gallery::Register reg("stock", "Stock widgets", build);

}  // namespace
