#pragma once

// Editing (spec/components/editing.md): Timeline, Scrubber, TrimHandle, CropFrame, DragHandle,
// DropIndicator and ColorPicker, drawn as the Qt Quick kit draws them. Handles and the playhead
// are accent ("hot accent" = a quarter of the foreground mixed in); clips, lanes and swatches
// are foreground glass; small parts round by min(4, radius).

#include <QList>
#include <QPointF>
#include <QStringList>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

class QPainter;

namespace Omarchy {

class TextField;

// An in/out handle. A trim cap (edge "left" / "right"): accent, space(8) wide, rounded on its
// outer side, with a grip line cut into it. A crop handle (crop = true; edge "left", "right",
// "top", "bottom" or "top-left", "top-right", "bottom-left", "bottom-right"): a space(4)-thick
// bar space(18) long, or an L bracket. Reports pointer travel since the press (moved) and arrow
// nudges (1 px, Shift 10 px, along the axes it moves).
class OMARCHY_EXPORT TrimHandle : public Control {
  Q_OBJECT
  Q_PROPERTY(QString edge READ edge WRITE setEdge)
  Q_PROPERTY(bool crop READ isCrop WRITE setCrop)

public:
  explicit TrimHandle(const QString &edge = QStringLiteral("left"), QWidget *parent = nullptr);

  QString edge() const { return m_edge; }
  void setEdge(const QString &e);
  bool isCrop() const { return m_crop; }
  void setCrop(bool on);
  // Accent and the background by default.
  QColor color() const;
  void setColor(const QColor &c);
  QColor gripColor() const;
  void setGripColor(const QColor &c);
  bool isDragging() const { return m_dragging; }
  // Pointer (with the grab margin), cursor, focus or drag.
  bool isHandleHot() const;
  // The handle's own size (implicit), and its height as a trim cap (default space(40)).
  QSize handleSize() const;
  void setCapHeight(int px);

  // A little more than the visible handle to grab: the widget extends `margin` past the handle
  // on every side; place it with setHandleGeometry().
  void setGrabMargin(int margin);
  int grabMargin() const { return m_margin; }
  void setHandleGeometry(const QRectF &visible);
  QRectF handleRect() const;
  // When the owner draws the handle (to keep its own stacking), the widget only takes input.
  void setDrawnByOwner(bool on);

  // Draws a handle of `edge` into `r`.
  static void paint(QPainter &p, const QRectF &r, const QString &edge, bool crop, bool hot, const QColor &color, const QColor &grip);

  QSize sizeHint() const override;

Q_SIGNALS:
  void pressed();
  void moved(qreal dx, qreal dy);
  void released();
  void nudged(qreal dx, qreal dy);
  void draggingChanged(bool dragging);

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  void enterEvent(QEnterEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;

private:
  void refresh();
  void updateCursor();
  QString m_edge;
  bool m_crop = false, m_dragging = false, m_drawnByOwner = false;
  QColor m_color, m_grip;
  int m_margin = 0, m_capHeight = 0;
  QPointF m_origin;
};

// A time ruler over clip tracks, with a playhead. Press or drag the ruler or an empty lane to
// seek; press a clip to select it; drag a trim handle (or nudge it with arrows) to trim; the
// wheel zooms around the pointer. Left/Right move the playhead a second (Shift: a major step),
// Home/End jump to the ends. It reports; the owner sets position, selection and clips.
class OMARCHY_EXPORT Timeline : public Control {
  Q_OBJECT
  Q_PROPERTY(qreal duration READ duration WRITE setDuration)
  Q_PROPERTY(qreal position READ position WRITE setPosition)

public:
  struct Clip {
    qreal start = 0, end = 0;
    QString label;
  };
  using Track = QList<Clip>;

  explicit Timeline(QWidget *parent = nullptr);

  qreal duration() const { return m_duration; }
  void setDuration(qreal s);
  qreal position() const { return m_position; }
  void setPosition(qreal s);
  QList<Track> tracks() const { return m_tracks; }
  void setTracks(const QList<Track> &tracks);
  int selectedTrack() const { return m_selTrack; }
  int selectedClip() const { return m_selClip; }
  void setSelected(int track, int clip);
  qreal zoom() const { return m_zoom; }
  qreal viewStart() const { return m_viewStart; }
  void setView(qreal zoom, qreal viewStart);
  qreal maxZoom() const { return m_maxZoom; }
  void setMaxZoom(qreal z) { m_maxZoom = z; }

  // Geometry, as the Qt Quick kit works it out.
  qreal pixelsPerSecond() const;
  qreal tickStep() const;
  int xAt(qreal time) const;
  qreal timeAt(qreal x) const;
  qreal rulerHeight() const;
  qreal laneHeight() const;
  qreal laneY(int track) const;
  QRectF clipRect(int track, int clip) const;
  QRectF playheadHeadRect() const;
  QString format(qreal seconds) const;
  // The trim handles of the selected clip (null when none is selected).
  TrimHandle *trimHandle(const QString &edge) const;
  bool isPlayheadHot() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  void seek(qreal time);
  void clipPressed(int track, int clip);
  void trimmed(int track, int clip, const QString &edge, qreal time);
  void viewChanged(qreal zoom, qreal viewStart);

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void wheelEvent(QWheelEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;
  void resizeEvent(QResizeEvent *) override { placeHandles(); }
  void themeMetricsChanged() override { placeHandles(); }

private:
  QPair<int, int> clipAt(const QPointF &p) const;
  bool onHead(const QPointF &p) const;
  void placeHandles();
  void handleMoved(const QString &edge, qreal dx);
  void handleNudged(const QString &edge, qreal dx);
  qreal m_duration = 30, m_position = 0, m_zoom = 1, m_viewStart = 0, m_maxZoom = 64, m_trimOrigin = 0;
  QList<Track> m_tracks;
  int m_selTrack = -1, m_selClip = -1, m_hoverTrack = -1, m_hoverClip = -1;
  bool m_headHover = false, m_headDrag = false, m_scrubbing = false;
  TrimHandle *m_left, *m_right;
};

// A media transport scrub: elapsed time, the Slider's bar (unplayed in track, played in the
// foreground) and the total. Press or drag to seek; Left/Right ±5 s (Shift ±30 s), Home/End.
class OMARCHY_EXPORT Scrubber : public Control {
  Q_OBJECT
  Q_PROPERTY(qreal position READ position WRITE setPosition)
  Q_PROPERTY(qreal duration READ duration WRITE setDuration)

public:
  explicit Scrubber(QWidget *parent = nullptr);

  qreal position() const { return m_position; }
  void setPosition(qreal s);
  qreal duration() const { return m_duration; }
  void setDuration(qreal s);
  // The knob's ring: the surface it sits on (popups.background by default).
  void setSurfaceColor(const QColor &c);
  QString format(qreal seconds) const;
  QRectF trackRect() const;
  QRectF knobRect() const;
  bool isScrubHot() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  void seek(qreal time);

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;

private:
  QRectF barRect() const;
  qreal timeAt(qreal x) const;
  qreal m_position = 0, m_duration = 0;
  QColor m_surface;
  bool m_barHover = false, m_dragging = false;
};

// A crop rectangle: a 1 px accent outline, rule-of-thirds guides in track and the eight crop
// handles inside its edges. No fill: it sits over the image.
class OMARCHY_EXPORT CropFrame : public Control {
  Q_OBJECT
public:
  explicit CropFrame(QWidget *parent = nullptr);

  // The handle drawn hot ("" for none).
  QString cursorEdge() const { return m_cursorEdge; }
  void setCursorEdge(const QString &edge);
  TrimHandle *handle(const QString &edge) const;
  static QStringList edges();

  QSize sizeHint() const override;

Q_SIGNALS:
  void pressed(const QString &edge);
  void moved(const QString &edge, qreal dx, qreal dy);
  void released(const QString &edge);
  void nudged(const QString &edge, qreal dx, qreal dy);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void themeMetricsChanged() override { place(); }

private:
  void place();
  QString m_cursorEdge;
  QList<TrimHandle *> m_handles;
};

// The grip a row is dragged by: a 2×3 grid of dots, textDim at rest, the foreground when hot.
// Reports pointer travel since the press; Up/Down ask to move the row by one.
class OMARCHY_EXPORT DragHandle : public Control {
  Q_OBJECT
public:
  explicit DragHandle(QWidget *parent = nullptr);
  bool isDragging() const { return m_dragging; }
  bool isGripHot() const;
  QSize sizeHint() const override;

Q_SIGNALS:
  void pressed();
  void moved(qreal dx, qreal dy);
  void released();
  void moveRequested(int delta);

protected:
  void paintEvent(QPaintEvent *) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void keyPressEvent(QKeyEvent *e) override;

private:
  bool m_dragging = false;
  QPointF m_origin;
};

// Where a dragged row will land: an accent line max(1, space(2)) thick, as wide as the list.
// Place it over the rows, centered on the boundary (it takes no layout space of its own).
class OMARCHY_EXPORT DropIndicator : public QWidget {
  Q_OBJECT
public:
  explicit DropIndicator(QWidget *parent = nullptr);
  static int thickness();
  // Centers it on the boundary at `y` (in its parent), `width` wide from `x`.
  void placeAt(int x, int y, int width);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *) override;
};

// An HSV picker: a saturation/value square over the hue, a hue strip, an optional alpha strip,
// a preview swatch with a hex field, and the theme's palette. Arrows move the square's knob
// (1 %, Shift 10 %), Up/Down a focused strip, Return/Space pick a focused swatch.
class OMARCHY_EXPORT ColorPicker : public Control {
  Q_OBJECT
  Q_PROPERTY(qreal hue READ hue WRITE setHue)
  Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation)
  Q_PROPERTY(qreal value READ value WRITE setValue)
  Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha)
  Q_PROPERTY(QColor color READ color)
  Q_PROPERTY(QString hex READ hex)

public:
  explicit ColorPicker(QWidget *parent = nullptr);

  qreal hue() const { return m_h; }
  void setHue(qreal h);
  qreal saturation() const { return m_s; }
  void setSaturation(qreal s);
  qreal value() const { return m_v; }
  void setValue(qreal v);
  qreal alpha() const { return m_a; }
  void setAlpha(qreal a);
  bool showAlpha() const { return m_showAlpha; }
  void setShowAlpha(bool on);
  void setSurfaceColor(const QColor &c);
  // Keys of the theme's colors.toml, in order; missing ones are skipped.
  QStringList paletteNames() const { return m_paletteNames; }
  void setPaletteNames(const QStringList &names);
  // The swatches shown ("#rrggbb").
  QStringList swatches() const { return m_swatches; }

  QColor color() const;
  QString hex() const;
  // From a QColor or "#rrggbb[aa]"; false (and nothing changes) when it isn't one.
  bool setColor(const QString &hex);
  bool setColor(const QColor &c);
  // HSV (0–1) to 8-bit channels, each rounded (the Rune binding's arithmetic).
  static void hsvToRgb(qreal h, qreal s, qreal v, int rgb[3]);

  // The parts, for tests and owners.
  QWidget *square() const { return m_square; }
  QWidget *hueStrip() const { return m_hueStrip; }
  QWidget *alphaStrip() const { return m_alphaStrip; }
  TextField *hexField() const { return m_field; }
  QWidget *swatch(int i) const { return m_swatchWidgets.value(i); }
  QRectF squareKnobRect() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  // Every user change.
  void edited(const QColor &color);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void themeMetricsChanged() override { place(); }

private:
  friend class PickerPart;
  void place();
  void reloadPalette();
  void changed();
  void sync();
  void pickSquare(const QPointF &p);
  void pickStrip(QWidget *strip, qreal fraction);
  void keyOnPart(QWidget *part, QKeyEvent *e);
  void chooseSwatch(int i);
  int knobSize() const;
  qreal m_h = 0, m_s = 0, m_v = 1, m_a = 1;
  bool m_showAlpha = true, m_editing = false;
  QColor m_surface;
  QStringList m_paletteNames, m_swatches;
  QWidget *m_square, *m_hueStrip, *m_alphaStrip;
  QList<QWidget *> m_swatchWidgets;
  TextField *m_field;
};

}  // namespace Omarchy
