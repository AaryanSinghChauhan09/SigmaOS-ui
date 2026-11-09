#pragma once

// Omarchy::Style: a QProxyStyle over Fusion that draws Qt's stock widgets the Omarchy way
// (foreground glass, Hyprland's rounding, flat, focus like hover, accent only for what's
// chosen), from the live tokens. Omarchy::Widgets::apply() installs it; set it yourself with
// `app.setStyle(new Omarchy::Style)` for a widget tree of your own.

#include <QProxyStyle>

#include "Omarchy/Export.h"
#include "Omarchy/Theme.h"

namespace Omarchy {

class OMARCHY_EXPORT Style : public QProxyStyle {
  Q_OBJECT
public:
  Style();

  void polish(QWidget *w) override;
  void unpolish(QWidget *w) override;
  void polish(QPalette &pal) override;
  void polish(QApplication *app) override { QProxyStyle::polish(app); }

  int pixelMetric(PixelMetric m, const QStyleOption *opt = nullptr, const QWidget *w = nullptr) const override;
  int styleHint(StyleHint h, const QStyleOption *opt = nullptr, const QWidget *w = nullptr, QStyleHintReturn *ret = nullptr) const override;
  QSize sizeFromContents(ContentsType ct, const QStyleOption *opt, const QSize &size, const QWidget *w) const override;
  QRect subElementRect(SubElement se, const QStyleOption *opt, const QWidget *w) const override;
  QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt, SubControl sc, const QWidget *w) const override;
  void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w = nullptr) const override;
  void drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p, const QWidget *w = nullptr) const override;
  void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w = nullptr) const override;
  QPixmap standardPixmap(StandardPixmap sp, const QStyleOption *opt, const QWidget *w) const override;

protected:
  bool eventFilter(QObject *o, QEvent *e) override;
};

}  // namespace Omarchy
