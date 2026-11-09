"""One-row controls (spec/terminal.md, Controls): `[edge][pad] content [pad][edge]`, the edge cells
reserved whenever any state shows a border, the fill across the whole width. A port of
`omarchy-ratatui`'s `ui/controls.rs`, cell for cell."""

from __future__ import annotations

from typing import Iterable, Sequence

from ..color import Rgba, rround, f32
from . import paint
from .base import Draw, Look, border_of, glyphs, look
from .buffer import Buffer, Rect
from .paint import Ink, edges, fill, put, text_center, text_fit, width

__all__ = [
    "Button",
    "ButtonGroup",
    "Checkbox",
    "ColorWell",
    "Dropdown",
    "DropdownList",
    "IconButton",
    "NumberField",
    "PasswordField",
    "Radio",
    "RadioGroup",
    "Slider",
    "SwatchStrip",
    "TextArea",
    "TextField",
    "Toggle",
    "ToggleSwitch",
    "wrap",
]


def control(t, buf: Buffer, x: int, y: int, w: int, l: Look) -> None:
    """A control's frame on row `y` over `w` cells: the fill across all of them, the edges on its
    first and last cell."""
    if w < 2:
        return
    fill(t, buf, Rect(x, y, w, 1), l.fill)
    edges(t, buf, x, y, w, l.border)


# Button, IconButton, ButtonGroup -------------------------------------------------------------


class Button(Draw):
    """Label, optional icon glyph. Borderless at rest unless `bordered`; `selected` fills and bolds."""

    def __init__(self, label: str, icon: str | None = None, bordered: bool = False, selected: bool = False,
                 cursor: bool = False, width: int | None = None, key: object = None):
        self.label, self.icon, self.bordered, self.selected = label, icon, bordered, selected
        self.cursor, self.width, self.key = cursor, width, key

    def size(self, t) -> tuple[int, int]:
        natural = 4 + width(self.label) + (2 if self.icon is not None else 0)
        return (self.width if self.width is not None else natural, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        l = look(t, self.cursor, self.selected, False, self.bordered)
        control(t, buf, area.x, area.y, area.width, l)
        inner = max(0, area.width - 4)
        content = f"{self.icon} {self.label}" if self.icon is not None else self.label
        text_center(t, buf, area.x + 2, area.y, inner, content, Ink(l.text, bold=l.bold))
        if self.key is not None:
            t.hit(self.key, area)


class IconButton(Draw):
    """One glyph: `[edge] G [edge]`, 5 columns."""

    def __init__(self, glyph: str, bordered: bool = False, cursor: bool = False, key: object = None):
        self.glyph, self.bordered, self.cursor, self.key = glyph, bordered, cursor, key

    def size(self, t) -> tuple[int, int]:
        return (5, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        if self.cursor:
            l = Look(c.hover.fill, border_of(c.hover.border) if self.bordered else None, t.fg(), False)
        else:
            l = look(t, False, False, False, self.bordered)
        control(t, buf, area.x, area.y, area.width, l)
        put(t, buf, area.x + 2, area.y, self.glyph, l.text)
        if self.key is not None:
            t.hit(self.key, area)


class ButtonGroup(Draw):
    """Bordered buttons one column apart, one selected: a segmented choice. Buttons keyed
    `(key, i)`."""

    def __init__(self, labels: Iterable[str], selected: int = 0, cursor: int | None = None, key: object = None):
        self.labels, self.selected, self.cursor, self.key = list(labels), selected, cursor, key

    @staticmethod
    def handle_key(selected: int, count: int, key: str) -> int | None:
        """Left/h and Right/l move `selected`, stopping at the ends; Home/End. The new index, or
        None when the key isn't one of these."""
        if key in ("left", "h"):
            return max(0, selected - 1)
        if key in ("right", "l"):
            return min(max(0, count - 1), selected + 1)
        if key == "home":
            return 0
        if key == "end":
            return max(0, count - 1)
        return None

    def buttons(self) -> list[Button]:
        return [
            Button(l, bordered=True, selected=i == self.selected, cursor=self.cursor == i,
                   key=(self.key, i) if self.key is not None else None)
            for i, l in enumerate(self.labels)
        ]

    def size(self, t) -> tuple[int, int]:
        bs = self.buttons()
        return (sum(b.size(t)[0] for b in bs) + max(0, len(bs) - 1), 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        x = area.x
        for b in self.buttons():
            w = b.size(t)[0]
            b.draw(t, Rect(x, area.y, w, 1).intersection(area), buf)
            x += w + 1


# ToggleSwitch, Toggle ------------------------------------------------------------------------


def switch_track(t, buf: Buffer, x: int, y: int, on: bool) -> None:
    """The four track cells at (x, y)."""
    c = t.shown.controls
    fill(t, buf, Rect(x, y, 4, 1), c.selected.fill if on else c.normal.fill)
    knob = c.selected.text if on else t.fg().darker(1.25)
    kx = x + 2 if on else x
    put(t, buf, kx, y, "█", knob)
    put(t, buf, kx + 1, y, "█", knob)


class ToggleSwitch(Draw):
    """The bare switch: four cells of track, the knob `██` at the start (off) or the end (on); with
    the ring's edges, 6 columns. `bare` is the 4 (inside a Toggle or a ListRow)."""

    def __init__(self, on: bool = False, cursor: bool = False, bare: bool = False, key: object = None):
        self.on, self.cursor, self.bare, self.key = on, cursor, bare, key

    def size(self, t) -> tuple[int, int]:
        return (4 if self.bare else 6, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        if self.bare:
            switch_track(t, buf, area.x, area.y, self.on)
        else:
            ring = border_of(t.shown.controls.hover.border) if self.cursor else None
            edges(t, buf, area.x, area.y, 6, ring)
            switch_track(t, buf, area.x + 1, area.y, self.on)
        if self.key is not None:
            t.hit(self.key, area)


class Toggle(Draw):
    """A labeled row with the switch at its end; the whole row is the control."""

    def __init__(self, title: str, description: str = "", on: bool = False, cursor: bool = False,
                 width: int = 45, key: object = None):
        self.title, self.description, self.on, self.cursor = title, description, on, cursor
        self.width, self.key = width, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1 if not self.description else 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        l = look(t, self.cursor, False, True, True)
        for dy in range(area.height):
            control(t, buf, area.x, area.y + dy, area.width, l)
        words = max(0, area.width - (4 + 4 + 1))
        text_fit(t, buf, area.x + 2, area.y, self.title, words, Ink(t.fg(), bold=True))
        if self.description and area.height > 1:
            text_fit(t, buf, area.x + 2, area.y + 1, self.description, words, t.dim())
        switch_track(t, buf, max(0, area.right - 6), area.y, self.on)
        if self.key is not None:
            t.hit(self.key, area)


# TextField, PasswordField, TextArea ------------------------------------------------------------


def field_text(t, buf: Buffer, x: int, y: int, w: int, s: str, placeholder: str, caret: int | None, ink: Rgba) -> None:
    """A field's text (or placeholder) and caret in `w` cells at (x, y)."""
    if not s:
        text_fit(t, buf, x, y, placeholder, w, t.faint())
    else:
        text_fit(t, buf, x, y, s, w, ink)
    if caret is not None:
        cx = x + min(width(s[:caret]), max(0, w - 1))
        paint.reverse(buf, cx, y)


class TextField(Draw):
    """A single-line input: normal fill and border at rest, the hover look with the cursor; the
    caret (reverse video) before char index `caret` while it has the keys."""

    def __init__(self, text: str = "", placeholder: str = "", width: int = 28, cursor: bool = False,
                 caret: int | None = None, key: object = None):
        self.text, self.placeholder, self.width, self.cursor = text, placeholder, width, cursor
        self.caret, self.key = caret, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        l = look(t, self.cursor, False, True, True)
        control(t, buf, area.x, area.y, area.width, l)
        field_text(t, buf, area.x + 2, area.y, max(0, area.width - 4), self.text, self.placeholder, self.caret, t.fg())
        if self.key is not None:
            t.hit(self.key, area)


class PasswordField(Draw):
    """A TextField for secrets: a lock, dots, an optional eye; with an error the urgent edges and
    the message under it."""

    def __init__(self, text: str = "", placeholder: str = "", width: int = 28, cursor: bool = False,
                 revealable: bool = False, revealed: bool = False, error: str = "", caret: int | None = None,
                 key: object = None):
        self.text, self.placeholder, self.width, self.cursor = text, placeholder, width, cursor
        self.revealable, self.revealed, self.error, self.caret, self.key = revealable, revealed, error, caret, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1 if not self.error else 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        s = t.shown
        l = look(t, self.cursor, False, True, True)
        erring = bool(self.error)
        if erring:
            l = l._replace(border=s.surfaces.polkit.border_error.color)
        control(t, buf, area.x, area.y, area.width, l)
        glyph_ink = s.surfaces.polkit.text_error if erring else t.dim()
        put(t, buf, area.x + 2, area.y, "", glyph_ink)
        eye = 2 if self.revealable else 0
        w = max(0, area.width - (6 + eye))
        shown = self.text if self.revealed else "•" * len(self.text)
        field_text(t, buf, area.x + 4, area.y, w, shown, self.placeholder, self.caret, t.fg())
        if self.revealable:
            put(t, buf, max(0, area.right - 3), area.y, "\U000f0209" if self.revealed else "\U000f0208", t.dim())
        if erring and area.height > 1:
            text_fit(t, buf, area.x + 2, area.y + 1, self.error, max(0, area.width - 2), s.surfaces.polkit.text_error)
        if self.key is not None:
            t.hit(self.key, area.replace(height=1))
            if self.revealable:
                t.hit((self.key, "eye"), Rect(max(0, area.right - 3), area.y, 1, 1))


def wrap(s: str, w: int) -> list[str]:
    """Word-wrap `s` to `w` cells (hard newlines kept; a word longer than a line stays whole)."""
    out: list[str] = []
    for para in s.split("\n"):
        line = ""
        for word in para.split(" "):
            candidate = word if not line else f"{line} {word}"
            if width(candidate) <= w or not line:
                line = candidate
            else:
                out.append(line)
                line = word
        out.append(line)
    return out


class TextArea(Draw):
    """Wrapped multi-line text in TextField's frame (edges on every row), `rows` (min, max) tall."""

    def __init__(self, text: str = "", placeholder: str = "", width: int = 60, rows: tuple[int, int] = (3, 6),
                 cursor: bool = False, caret: int | None = None, key: object = None):
        self.text, self.placeholder, self.width = text, placeholder, width
        self.min_rows, self.max_rows = rows[0], max(rows)
        self.cursor, self.caret, self.key = cursor, caret, key

    def lines(self) -> list[str]:
        return wrap(self.text, max(0, self.width - 4))

    def size(self, t) -> tuple[int, int]:
        n = min(max(len(self.lines()), self.min_rows), self.max_rows)
        return (self.width, n)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        l = look(t, self.cursor, False, True, True)
        for dy in range(area.height):
            control(t, buf, area.x, area.y + dy, area.width, l)
        inner = max(0, area.width - 4)
        if not self.text:
            text_fit(t, buf, area.x + 2, area.y, self.placeholder, inner, t.faint())
        lines = self.lines()
        for i, line in enumerate(lines[: area.height]):
            text_fit(t, buf, area.x + 2, area.y + i, line, inner, t.fg())
        if self.caret is not None and area.height > 0:
            # The caret at the end of the text (the field takes keys at its end).
            last = min(len(lines), area.height) - 1
            cx = area.x + 2 + min(width(lines[last]) if lines else 0, max(0, inner - 1))
            paint.reverse(buf, cx, area.y + max(0, last))
        if self.key is not None:
            t.hit(self.key, area)


# NumberField, Dropdown -------------------------------------------------------------------------

_I64_MIN, _I64_MAX = -(1 << 63), (1 << 63) - 1


class NumberField(Draw):
    """An integer stepper: an optional label row (`textDim`), then `[−]  value  [+]` in a field.
    The field keyed `key`, the steppers `(key, "dec")` and `(key, "inc")`."""

    def __init__(self, value: int, label: str = "", range: tuple[int, int] = (_I64_MIN, _I64_MAX), width: int = 15,
                 cursor: bool = False, key: object = None):
        self.value, self.label, self.width, self.cursor, self.key = value, label, width, cursor, key
        self.min, self.max = range

    @staticmethod
    def handle_key(value: int, lo: int, hi: int, key: str) -> int | None:
        """Up/k/+ and Down/j/− step within the range; the new value, or None."""
        if key in ("up", "right", "k", "l", "plus", "+"):
            return min(hi, value + 1)
        if key in ("down", "left", "j", "h", "minus", "-"):
            return max(lo, value - 1)
        return None

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1 if not self.label else 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        y = area.y
        if self.label:
            text_fit(t, buf, area.x, y, self.label, area.width, t.dim())
            y += 1
        l = look(t, self.cursor, False, True, True)
        control(t, buf, area.x, y, area.width, l)
        dec = t.fg() if self.value > self.min else t.faint()
        inc = t.fg() if self.value < self.max else t.faint()
        put(t, buf, area.x + 2, y, "−", dec)
        put(t, buf, max(0, area.right - 3), y, "+", inc)
        text_center(t, buf, area.x + 4, y, max(0, area.width - 8), str(self.value), t.fg())
        if self.key is not None:
            t.hit(self.key, Rect(area.x, y, area.width, 1))
            t.hit((self.key, "dec"), Rect(area.x + 1, y, 2, 1))
            t.hit((self.key, "inc"), Rect(max(0, area.right - 3), y, 2, 1))


def trigger(t, buf: Buffer, x: int, y: int, w: int, hot: bool, s: str, ink: Rgba) -> None:
    """A trigger row: field frame, text, chevron."""
    l = look(t, hot, False, True, True)
    control(t, buf, x, y, w, l)
    text_fit(t, buf, x + 2, y, s, max(0, w - 6), ink)
    put(t, buf, x + max(0, w - 3), y, glyphs.CHEVRON_DOWN, t.fg().darker(1.2))


class Dropdown(Draw):
    """A labeled single choice: label row (bold `textDim`), then the trigger. Its list
    (`DropdownList`) is drawn over the page, under the trigger. The trigger keyed `key`."""

    def __init__(self, value: str, label: str = "", width: int = 30, cursor: bool = False, key: object = None):
        self.value, self.label, self.width, self.cursor, self.key = value, label, width, cursor, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1 if not self.label else 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        y = area.y
        if self.label:
            text_fit(t, buf, area.x, y, self.label, area.width, Ink(t.dim(), bold=True))
            y += 1
        trigger(t, buf, area.x, y, area.width, self.cursor, self.value, t.fg())
        if self.key is not None:
            t.hit(self.key, Rect(area.x, y, area.width, 1))


class DropdownList(Draw):
    """A Dropdown's open list on the popups surface. Rows keyed `(key, i)`."""

    def __init__(self, options: Iterable[str], chosen: int | None = None, highlighted: int | None = None,
                 width: int = 30, key: object = None):
        self.options, self.chosen, self.highlighted = list(options), chosen, highlighted
        self.width, self.key = width, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, len(self.options) + 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        paint.clear(buf, area)
        s = t.shown
        inner = paint.frame(t, buf, area, s.surfaces.popups.border.color)
        for i, o in enumerate(self.options):
            y = inner.y + i
            if y >= inner.bottom:
                break
            if self.highlighted == i:
                fill(t, buf, Rect(inner.x, y, inner.width, 1), s.controls.hover.fill)
            chosen = self.chosen == i
            text_fit(t, buf, inner.x + 1, y, o, max(0, inner.width - 4), Ink(t.fg(), bold=chosen))
            if chosen:
                put(t, buf, max(0, inner.right - 2), y, glyphs.CHECK, Ink(t.accent(), bold=True))
            if self.key is not None:
                t.hit((self.key, i), Rect(inner.x, y, inner.width, 1))


# Slider ----------------------------------------------------------------------------------------


class Slider(Draw):
    """A thin track: played `━` in the foreground, the knob `●`, the rest `─` in `track`; notches
    `┼` on the rest and `╋` on the played part. Edges show with the cursor. (The value is an f32,
    as in every binding.)"""

    def __init__(self, value: float, notches: int = 0, width: int = 45, cursor: bool = False, key: object = None):
        self.value = f32(min(1.0, max(0.0, value)))
        self.notches, self.width, self.cursor, self.key = notches, width, cursor, key

    @staticmethod
    def value_at(area: Rect, x: int) -> float:
        """The value at column `x` of a slider drawn at `area`."""
        track = max(2, max(0, area.width - 2))
        return f32(min(1.0, max(0.0, f32(max(0, x - (area.x + 1)) / (track - 1)))))

    @staticmethod
    def handle_key(value: float, notches: int, key: str) -> float | None:
        """Left/h and Right/l step by a notch (or 5%), Home/End jump; the new value, or None."""
        step = f32(1.0 / (notches - 1)) if notches > 1 else f32(0.05)
        if key in ("left", "h"):
            v = f32(value - step)
        elif key in ("right", "l"):
            v = f32(value + step)
        elif key == "home":
            v = 0.0
        elif key == "end":
            v = 1.0
        else:
            return None
        if notches > 1:
            return min(1.0, max(0.0, f32(rround(f32(v / step)) * step)))
        return min(1.0, max(0.0, v))

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        ring = border_of(t.shown.controls.hover.border) if self.cursor else None
        edges(t, buf, area.x, area.y, area.width, ring)
        w = max(0, area.width - 2)
        if w < 2:
            return
        knob = rround(f32(self.value * (w - 1)))
        notches = set()
        if self.notches > 1:
            for n in range(self.notches):
                notches.add(rround(f32(f32(n / (self.notches - 1)) * (w - 1))))
        for i in range(w):
            x = area.x + 1 + i
            if i < knob:
                put(t, buf, x, area.y, "╋" if i in notches else "━", t.fg())
            elif i > knob:
                put(t, buf, x, area.y, "┼" if i in notches else "─", t.track())
        put(t, buf, area.x + 1 + knob, area.y, "●", t.fg())
        if self.key is not None:
            t.hit(self.key, area)


# Checkbox, Radio, RadioGroup -----------------------------------------------------------------


def check_box(t, buf: Buffer, x: int, y: int, checked: bool, exclusive: bool, hot: bool, disabled: bool) -> None:
    """The three-cell box at (x, y)."""
    c = t.shown.controls
    if hot and not disabled:
        fill_c, border = c.hover.fill, c.hover.border.color
    elif checked:
        fill_c, border = c.selected.fill, c.normal.border.color
    else:
        fill_c, border = c.normal.fill, c.normal.border.color
    if disabled:
        border = border.times_alpha(0.5)
    fill(t, buf, Rect(x, y, 3, 1), fill_c.times_alpha(0.5) if disabled else fill_c)
    l, r = ("(", ")") if exclusive else ("[", "]")
    put(t, buf, x, y, l, border)
    put(t, buf, x + 2, y, r, border)
    if checked:
        mark = glyphs.DOT if exclusive else glyphs.CHECK
        i = t.accent().times_alpha(0.5) if disabled else t.accent()
        put(t, buf, x + 1, y, mark, Ink(i, bold=True))


class Checkbox(Draw):
    """`[✓] Label` (Radio: `(●) Label`); a description goes on the row under the label."""

    def __init__(self, label: str, description: str = "", checked: bool = False, exclusive: bool = False,
                 cursor: bool = False, disabled: bool = False, key: object = None):
        self.label, self.description, self.checked, self.exclusive = label, description, checked, exclusive
        self.cursor, self.disabled, self.key = cursor, disabled, key

    def size(self, t) -> tuple[int, int]:
        w = 4 + max(width(self.label), width(self.description))
        return (w, 1 if not self.description else 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        check_box(t, buf, area.x, area.y, self.checked, self.exclusive, self.cursor, self.disabled)
        words = max(0, area.width - 4)
        i, dim = (t.faint(), t.faint()) if self.disabled else (t.fg(), t.dim())
        text_fit(t, buf, area.x + 4, area.y, self.label, words, i)
        if self.description and area.height > 1:
            text_fit(t, buf, area.x + 4, area.y + 1, self.description, words, dim)
        if self.key is not None and not self.disabled:
            t.hit(self.key, area)


def Radio(label: str, **kw) -> Checkbox:
    """A Checkbox with `exclusive`: `(●)`."""
    return Checkbox(label, exclusive=True, **kw)


class RadioGroup(Draw):
    """Radios stacked, one chosen. Radios keyed `(key, value)`."""

    def __init__(self, options: Iterable[tuple[str, str]], value: str, cursor: int | None = None, key: object = None):
        self.options, self.value, self.cursor, self.key = list(options), value, cursor, key

    @staticmethod
    def handle_key(value: str, values: Sequence[str], key: str) -> str | None:
        """Move `value` among `values` (arrows, j/k/h/l), stopping at the ends; the new value, or None."""
        i = values.index(value) if value in values else 0
        if key in ("down", "right", "j", "l"):
            return values[min(len(values) - 1, i + 1)]
        if key in ("up", "left", "k", "h"):
            return values[max(0, i - 1)]
        return None

    def size(self, t) -> tuple[int, int]:
        return (max((4 + width(l) for _, l in self.options), default=0), len(self.options))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        for i, (v, l) in enumerate(self.options):
            r = Radio(l, checked=v == self.value, cursor=self.cursor == i,
                      key=(self.key, v) if self.key is not None else None)
            r.draw(t, Rect(area.x, area.y + i, area.width, 1).intersection(area), buf)


# ColorWell, SwatchStrip ------------------------------------------------------------------------


class ColorWell(Draw):
    """A bordered well with the color inset `▏███▕`, and a ring outside it with the cursor: 7 columns."""

    def __init__(self, color: Rgba, cursor: bool = False, key: object = None):
        self.color, self.cursor, self.key = color, cursor, key

    def size(self, t) -> tuple[int, int]:
        return (7, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        if self.cursor:
            edges(t, buf, area.x, area.y, 7, c.hover.border.color)
        edges(t, buf, area.x + 1, area.y, 5, border_of(c.normal.border))
        for i in range(3):
            put(t, buf, area.x + 2 + i, area.y, "█", self.color)
        if self.key is not None:
            t.hit(self.key, area)


class SwatchStrip(Draw):
    """Color chips `██` one column apart; `▔▔` under the chosen (accent) or the cursor's (hover
    border). Two rows. Chips keyed `(key, i)`."""

    def __init__(self, colors: Iterable[Rgba], chosen: int | None = None, cursor: int | None = None, key: object = None):
        self.colors, self.chosen, self.cursor, self.key = list(colors), chosen, cursor, key

    def size(self, t) -> tuple[int, int]:
        return (max(0, len(self.colors) * 3 - 1), 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        for i, c in enumerate(self.colors):
            x = area.x + i * 3
            put(t, buf, x, area.y, "█", c)
            put(t, buf, x + 1, area.y, "█", c)
            mark = None
            if self.chosen == i:
                mark = t.accent()
            elif self.cursor == i:
                mark = t.shown.controls.hover.border.color
            if mark is not None:
                put(t, buf, x, area.y + 1, "▔", mark)
                put(t, buf, x + 1, area.y + 1, "▔", mark)
            if self.key is not None:
                t.hit((self.key, i), Rect(x, area.y, 2, 2))
