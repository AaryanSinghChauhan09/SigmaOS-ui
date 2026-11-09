#pragma once

// Forms (spec/components/forms.md): SettingRow, Checkbox, Radio, RadioGroup, PasswordField,
// TextArea, MultiSelect, ColorWell, SwatchStrip, Dialog.

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

namespace Omarchy {

// A settings line: a label (and an optional wrapped description) on the left, its control at
// the right edge (addControl), the hover fill under the cursor.
class OMARCHY_EXPORT SettingRow : public Control {
  Q_OBJECT
public:
  explicit SettingRow(const QString &label = QString(), const QString &description = QString(), QWidget *parent = nullptr);
  void setLabel(const QString &t);
  void setDescription(const QString &t);
  void addControl(QWidget *w);
  QSize sizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void themeMetricsChanged() override { place(); }

private:
  void place();
  qreal wordsHeight(int width) const;
  QString m_label, m_description;
  QWidget *m_trailing;
  Row *m_row;
};

}  // namespace Omarchy
