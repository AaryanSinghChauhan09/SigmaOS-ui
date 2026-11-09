#pragma once

// Forms (spec/components/forms.md): SettingRow, Checkbox, Radio, RadioGroup, PasswordField,
// TextArea, MultiSelect, ColorWell, SwatchStrip, Dialog.

#include <QPlainTextEdit>
#include <QPointer>
#include <QStringList>
#include <functional>

#include "Omarchy/Control.h"
#include "Omarchy/Layout.h"

class QVariantAnimation;

namespace Omarchy {

class TextField;

// One choice of a RadioGroup or MultiSelect: a value, its label (the value when empty) and an
// optional description.
struct OMARCHY_EXPORT Choice {
  QString value, label, description;
  QString text() const { return label.isEmpty() ? value : label; }
  static QList<Choice> fromStrings(const QStringList &values);
};

// A settings line: a label (and an optional wrapped description) on the left, its control at
// the right edge (addControl), the hover fill under the cursor. Disabled dims the words.
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

// A check row: a small square box, then a label and an optional dim description. Checked shows
// the selected state with a bold accent ✓; `exclusive` draws a radio (a circle, an accent dot).
// A click, Space or Return emits clicked() and, unless setAutoToggle(false) (the caller owns the
// value, as in Qt Quick), flips it; toggled(bool) follows the state. Disabled ignores both.
class OMARCHY_EXPORT Checkbox : public Control {
  Q_OBJECT
  Q_PROPERTY(QString label READ label WRITE setLabel)
  Q_PROPERTY(QString description READ description WRITE setDescription)
  Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)
  Q_PROPERTY(bool exclusive READ isExclusive WRITE setExclusive)
public:
  explicit Checkbox(const QString &label = QString(), const QString &description = QString(), QWidget *parent = nullptr);

  QString label() const { return m_label; }
  void setLabel(const QString &t);
  QString description() const { return m_description; }
  void setDescription(const QString &t);
  bool isChecked() const { return m_checked; }
  void setChecked(bool on);
  bool isExclusive() const { return m_exclusive; }
  void setExclusive(bool on);
  void setAutoToggle(bool on) { m_auto = on; }

  // The box: space(16) square, beside the label line's middle.
  QRectF boxRect() const;
  QSizeF implicitSize() const;
  QSize sizeHint() const override;

Q_SIGNALS:
  void clicked();
  void toggled(bool checked);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void activate(Qt::MouseButton b) override;
  void changeEvent(QEvent *e) override;

private:
  qreal wordsHeight(qreal width) const;
  QString m_label, m_description;
  bool m_checked = false, m_exclusive = false, m_auto = true;
};

// A Checkbox drawn as a radio. A click checks it (never unchecks); group them in a RadioGroup.
class OMARCHY_EXPORT Radio : public Checkbox {
  Q_OBJECT
public:
  explicit Radio(const QString &label = QString(), const QString &description = QString(), QWidget *parent = nullptr);
};

// A column of Radios lg apart, one checked. Tab focuses the group; Up/Down/Left/Right and j/k
// move the choice; a click picks. `cursorIndex` forces the hover look on one row.
class OMARCHY_EXPORT RadioGroup : public Control {
  Q_OBJECT
  Q_PROPERTY(QString value READ value WRITE setValue NOTIFY changed)
public:
  explicit RadioGroup(QWidget *parent = nullptr);

  void setOptions(const QList<Choice> &options);
  void setOptions(const QStringList &values) { setOptions(Choice::fromStrings(values)); }
  QList<Choice> options() const { return m_options; }
  QString value() const { return m_value; }
  void setValue(const QString &v);
  int cursorIndex() const { return m_cursor; }
  void setCursorIndex(int i);
  int indexOfValue(const QString &v) const;
  // Picks option `index` as a click would (changed() when it moves).
  void choose(int index);
  Radio *radio(int index) const { return m_radios.value(index); }

  QSize sizeHint() const override;

Q_SIGNALS:
  void changed(const QString &value);

protected:
  void keyPressEvent(QKeyEvent *e) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  void focusInEvent(QFocusEvent *e) override;
  void focusOutEvent(QFocusEvent *e) override;
  void themeMetricsChanged() override { place(); }

private:
  void rebuild();
  void sync();
  void place();
  QList<Choice> m_options;
  QList<Radio *> m_radios;
  QString m_value;
  int m_cursor = -1;
};

// A TextField for secrets: dots (•) for the text, a lock glyph before them, an optional eye
// that reveals it, and an error state (polkit's colors, the message under the field). Setting
// an error flashes and shakes the field (both land at once under reduced motion).
class OMARCHY_EXPORT PasswordField : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText)
  Q_PROPERTY(bool revealed READ isRevealed WRITE setRevealed NOTIFY revealedChanged)
  Q_PROPERTY(QString error READ error WRITE setError)
public:
  explicit PasswordField(QWidget *parent = nullptr);

  QString text() const;
  void setText(const QString &t);
  QString placeholderText() const;
  void setPlaceholderText(const QString &t);
  bool isRevealable() const { return m_revealable; }
  void setRevealable(bool on);
  bool isRevealed() const { return m_revealed; }
  void setRevealed(bool on);
  QString error() const { return m_error; }
  void setError(const QString &message);
  bool hasError() const { return !m_error.isEmpty(); }
  bool hasCursor() const;
  void setHasCursor(bool on);
  // The line edit (an Omarchy::TextField in password echo mode).
  TextField *field() const;
  // Where the eye is in the field's coordinates (empty unless revealable).
  QRectF eyeRect() const;
  // Replays the error flash and shake.
  void shake();

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override;
  bool hasHeightForWidth() const override { return true; }
  int heightForWidth(int w) const override;

Q_SIGNALS:
  void accepted();
  void textChanged(const QString &text);
  void revealedChanged(bool revealed);

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }

private:
  friend class PasswordEdit;
  void place();
  void retheme();
  class PasswordEdit *m_field;
  QString m_error;
  bool m_revealable = false, m_revealed = false;
  qreal m_shake = 0, m_flash = 0;
  QVariantAnimation *m_shakeAnim, *m_flashAnim;
};

// A multi-line TextField: the same chrome, padding and type around a QPlainTextEdit that wraps
// its text; minRows lines tall at least, growing with its text to maxRows, then scrolling (a
// thin textFaint bar shows only then).
class OMARCHY_EXPORT TextArea : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
public:
  explicit TextArea(QWidget *parent = nullptr);
  explicit TextArea(const QString &text, QWidget *parent = nullptr);

  QString text() const;
  void setText(const QString &t);
  QString placeholderText() const;
  void setPlaceholderText(const QString &t);
  int minRows() const { return m_minRows; }
  void setMinRows(int n);
  int maxRows() const { return m_maxRows; }
  void setMaxRows(int n);
  bool hasCursor() const { return m_hasCursor; }
  void setHasCursor(bool on);
  QPlainTextEdit *area() const { return m_area; }
  // Laid-out lines (wrapped), and the rows shown: clamp(lines, minRows, maxRows).
  int lineCount() const;
  int rows() const;

  QSize sizeHint() const override;
  QSize minimumSizeHint() const override { return sizeHint(); }

Q_SIGNALS:
  void textChanged();

protected:
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  bool event(QEvent *e) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  void place();
  void retheme();
  QMargins padding() const;
  QPlainTextEdit *m_area;
  int m_minRows = 3, m_maxRows = 6;
  bool m_hasCursor = false, m_hovered = false;
};

// The searchable multi-choice dropdown: a labeled trigger reading the chosen labels (or "N
// selected" past three, or "None selected"), and a popup in the window's overlay below it: a
// search field, a rule, a checkbox row per option. On the trigger j, Down, Return or Space opens
// it; typing filters, Return toggles the first match, Down goes into the list; there j/k and
// Down/Up walk (Up off the first row goes back to the search), Return or Space toggles; Escape
// closes, clearing the search.
class OMARCHY_EXPORT MultiSelect : public Control {
  Q_OBJECT
  Q_PROPERTY(QStringList values READ values WRITE setValues NOTIFY changed)
public:
  explicit MultiSelect(const QString &label = QString(), QWidget *parent = nullptr);
  ~MultiSelect() override;

  QString label() const { return m_label; }
  void setLabel(const QString &t);
  void setShowLabel(bool on);
  void setOptions(const QList<Choice> &options);
  void setOptions(const QStringList &values) { setOptions(Choice::fromStrings(values)); }
  QList<Choice> options() const { return m_options; }
  QStringList values() const { return m_values; }
  void setValues(const QStringList &values);
  bool isSelected(const QString &value) const { return m_values.contains(value); }
  // Adds `value` to the selection or takes it out, and emits changed().
  void toggleValue(const QString &value);

  // Text overrides; empty = the catalog's (common.search, common.noOptions, common.noneSelected).
  void setPlaceholderText(const QString &t) { m_placeholder = t; }
  void setEmptyText(const QString &t) { m_empty = t; }
  void setNoSelectionText(const QString &t);
  // Read on the trigger when nothing is chosen, before noSelectionText.
  void setTriggerLabel(const QString &t);
  void setRefreshable(bool on);
  void setLoadingOptions(bool on);
  void setOptionsError(const QString &message);

  QString selectionLabel() const;
  bool isPopupOpen() const;
  void open();
  void close();
  void toggle();
  QWidget *popup() const;
  TextField *searchField() const;
  // The highlighted row among the filtered ones; -1 until the keys or the pointer reach one.
  int popupIndex() const;
  // Where filtered row `index` is in the popup's coordinates (empty when closed).
  QRectF popupRowRect(int index) const;
  QRectF triggerRect() const;

  QSize sizeHint() const override;

Q_SIGNALS:
  void changed(const QStringList &values);
  void refreshRequested();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void activate(Qt::MouseButton b) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  friend class MultiSelectPopup;
  qreal labelHeight() const;
  QString placeholderText() const;
  QString emptyText() const;
  QString noSelectionText() const;
  QString m_label, m_placeholder, m_empty, m_none, m_triggerLabel, m_error;
  bool m_showLabel = true, m_refreshable = false, m_loading = false;
  QList<Choice> m_options;
  QStringList m_values;
  QPointer<class MultiSelectPopup> m_popup;
};

// A button showing a color: the swatch inset in a bordered well, a cursor ring outside it.
// A click, Space or Return emits clicked() (the app opens its picker).
class OMARCHY_EXPORT ColorWell : public Control {
  Q_OBJECT
  Q_PROPERTY(QColor swatch READ swatch WRITE setSwatch)
public:
  explicit ColorWell(const QColor &swatch = QColor(), QWidget *parent = nullptr);
  // Invalid = the theme's accent.
  QColor swatch() const;
  void setSwatch(const QColor &c);
  // A color from the colors on screen (a theme role that follows fades).
  void setSwatch(std::function<QColor(const Tokens &)> swatch);
  QSize sizeHint() const override;

Q_SIGNALS:
  void clicked();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void activate(Qt::MouseButton b) override;

private:
  void give(bool pressed);
  QColor m_swatch;
  std::function<QColor(const Tokens &)> m_swatchFn;
  qreal m_scale = 1;
  QVariantAnimation *m_give;
};

// A row of preset colors, one chosen (an accent ring); the cursor (keys or pointer) wears the
// hover border. Left/Right (h/l) move the cursor, Space or Return pick; a click picks.
class OMARCHY_EXPORT SwatchStrip : public Control {
  Q_OBJECT
  Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY picked)
public:
  explicit SwatchStrip(QWidget *parent = nullptr);
  // Colors, or a function giving them from the colors on screen (theme roles that follow fades).
  void setColors(const QList<QColor> &colors);
  void setColors(std::function<QList<QColor>(const Tokens &)> colors);
  QList<QColor> colors() const;
  int currentIndex() const { return m_current; }
  void setCurrentIndex(int i);
  int cursorIndex() const { return m_cursor; }
  void setCursorIndex(int i);
  void pick(int index);
  QRectF cellRect(int index) const;
  int indexAt(const QPointF &p) const;
  QSize sizeHint() const override;

Q_SIGNALS:
  void picked(int index);

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void leaveEvent(QEvent *e) override;
  void activate(Qt::MouseButton b) override;

private:
  int cellSize() const;
  QList<QColor> m_colors;
  std::function<QList<QColor>(const Tokens &)> m_colorsFn;
  int m_current = -1, m_cursor = -1, m_hover = -1;
  QPointF m_pressAt;
};

// The kit's modal: the scrim over its parent and a centered card with a title, a message, a
// body (addWidget) and a button row: a secondary choice and a primary one, urgent when
// destructive. Escape rejects, Return takes the selected button, Left/Right/h/l/Tab move the
// selection; a click on the scrim rejects. ConfirmDialog draws the same card.
class OMARCHY_EXPORT Dialog : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool opened READ isOpened WRITE setOpened)
  Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex)
public:
  explicit Dialog(QWidget *parent = nullptr);
  explicit Dialog(const QString &title, const QString &message = QString(), QWidget *parent = nullptr);

  bool isOpened() const { return isVisible(); }
  void setOpened(bool on);
  // Shows it over its parent and focuses it (setOpened(true) does the same).
  void open() { setOpened(true); }
  void close() { setOpened(false); }
  void setTitle(const QString &t);
  void setMessage(const QString &t);
  void addWidget(QWidget *w);
  // Empty = the catalog's Cancel / Confirm.
  void setSecondaryText(const QString &t);
  void setPrimaryText(const QString &t);
  QString secondaryText() const;
  QString primaryText() const;
  void setDestructive(bool on);
  bool isDestructive() const { return m_destructive; }
  // 0 the secondary button, 1 the primary.
  int selectedIndex() const { return m_selected; }
  void setSelectedIndex(int i);
  // The dialog's keys, for a panel that keeps focus itself: true when it took the key.
  bool handleKey(QKeyEvent *e);

  QRectF cardRect() const;
  QRectF buttonRect(int index) const;

Q_SIGNALS:
  void rejected();
  void accepted();

protected:
  void paintEvent(QPaintEvent *) override;
  void keyPressEvent(QKeyEvent *e) override;
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;
  void mouseReleaseEvent(QMouseEvent *e) override;
  void resizeEvent(QResizeEvent *) override { place(); }
  bool event(QEvent *e) override;
  bool eventFilter(QObject *o, QEvent *e) override;

private:
  void place();
  qreal contentHeight(qreal width) const;
  QString m_title, m_message, m_secondary, m_primary;
  bool m_destructive = false, m_pressedOnScrim = false;
  int m_selected = 1;
  QWidget *m_body;
  Column *m_bodyColumn;
};

}  // namespace Omarchy
