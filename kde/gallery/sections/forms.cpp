#include "gallery.h"

using namespace Omarchy;

namespace {

void build(Column *body, QWidget *parent) {
  // A column of setting rows.
  auto *rows = new QWidget(parent);
  rows->setFixedWidth(480);
  auto *rc = new Column(rows);
  rc->setSpacing(0);
  auto *night = new SettingRow(QStringLiteral("Night light"), QStringLiteral("Warm the screen after sunset"));
  auto *nightSwitch = new ToggleSwitch;
  nightSwitch->setChecked(true);
  night->addControl(nightSwitch);
  setFill(night);
  rc->addWidget(night);
  auto *theme = new SettingRow(QStringLiteral("Theme"), QStringLiteral("Applies to every app"));
  auto *dropdown = new Dropdown;
  dropdown->setFixedWidth(180);
  dropdown->setShowLabel(false);
  dropdown->setOptions({{QStringLiteral("tokyo-night"), QStringLiteral("Tokyo Night")}, {QStringLiteral("nord"), QStringLiteral("Nord")}});
  dropdown->setValue(QStringLiteral("tokyo-night"));
  theme->addControl(dropdown);
  setFill(theme);
  rc->addWidget(theme);
  auto *gaps = new SettingRow(QStringLiteral("Gaps"), QStringLiteral("Space between windows"));
  auto *number = new NumberField;
  number->setRange(0, 40);
  number->setValue(10);
  gaps->addControl(number);
  setFill(gaps);
  rc->addWidget(gaps);
  body->addWidget(rows);

  // Checkboxes in their states, and a radio group.
  Row *choices = Gallery::row(body, parent, Gallery::sp("huge") * 2);
  Column *checks = Gallery::column(choices, parent, Gallery::sp("lg"), Qt::AlignTop);
  checks->addWidget(new Checkbox(QStringLiteral("Show seconds")));
  auto *date = new Checkbox(QStringLiteral("Show date"));
  date->setChecked(true);
  checks->addWidget(date);
  auto *weekday = new Checkbox(QStringLiteral("Show weekday"));
  weekday->setHasCursor(true);
  checks->addWidget(weekday);
  auto *week = new Checkbox(QStringLiteral("Show week number"));
  week->setEnabled(false);
  checks->addWidget(week);
  auto *clock = new Checkbox(QStringLiteral("Twelve-hour clock"), QStringLiteral("Show AM and PM"));
  clock->setChecked(true);
  checks->addWidget(clock);
  auto *radios = new RadioGroup;
  radios->setOptions(QList<Choice>{{QStringLiteral("top"), QStringLiteral("Top"), {}},
                                   {QStringLiteral("bottom"), QStringLiteral("Bottom"), {}},
                                   {QStringLiteral("hidden"), QStringLiteral("Hidden"), {}}});
  radios->setValue(QStringLiteral("bottom"));
  choices->addWidget(radios, Qt::AlignTop);

  // Password fields: empty, revealable, and with an error.
  Row *secrets = Gallery::row(body, parent, Gallery::sp("controlGap"));
  auto *empty = new PasswordField;
  empty->setPlaceholderText(QStringLiteral("Password"));
  empty->setFixedWidth(220);
  secrets->addWidget(empty, Qt::AlignTop);
  auto *revealable = new PasswordField;
  revealable->setText(QStringLiteral("hunter22"));
  revealable->setRevealable(true);
  revealable->setFixedWidth(220);
  secrets->addWidget(revealable, Qt::AlignTop);
  auto *wrong = new PasswordField;
  wrong->setText(QStringLiteral("letmein"));
  wrong->setError(QStringLiteral("Wrong password, try again"));
  wrong->setFixedWidth(220);
  secrets->addWidget(wrong, Qt::AlignTop);

  auto *notes = new TextArea(QStringLiteral("Back up your dotfiles\nbefore switching themes,\nthen run omarchy update."));
  notes->setFixedWidth(480);
  body->addWidget(notes);
}

Gallery::Register reg("forms", "Forms", build);

}  // namespace
