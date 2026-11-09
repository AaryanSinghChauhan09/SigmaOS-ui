"""Setting rows and the multi-choice dropdown (spec/terminal.md, Forms): a port of
`omarchy-ratatui`'s `ui/forms.rs`, cell for cell. Checkbox, Radio, RadioGroup, PasswordField,
TextArea, ColorWell and SwatchStrip are with the controls; Dialog with the surfaces."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Iterable, Sequence

from .base import Draw
from .buffer import Buffer, Rect
from .controls import check_box, trigger
from .paint import Ink, clear, fill, frame, hline, put, text_fit

__all__ = ["MultiSelect", "MultiSelectState", "SettingRow"]


class SettingRow(Draw):
    """A row of a settings list: the label (and a `textDim` description under it) at the start,
    any control at the end of the first row. No frame; the cursor row takes the hover fill."""

    def __init__(self, label: str, description: str = "", control: Draw | None = None, cursor: bool = False,
                 disabled: bool = False, width: int = 60, key: object = None):
        self.label, self.description, self.control = label, description, control
        self.cursor, self.disabled, self.width, self.key = cursor, disabled, width, key

    def size(self, t) -> tuple[int, int]:
        ch = self.control.size(t)[1] if self.control is not None else 1
        return (self.width, max(1 if not self.description else 2, ch))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        if self.cursor and not self.disabled:
            fill(t, buf, area, t.shown.controls.hover.fill)
        cw = self.control.size(t)[0] if self.control is not None else 0
        words = max(0, area.width - (cw + 3))
        ink, dim = (t.faint(), t.faint()) if self.disabled else (t.fg(), t.dim())
        text_fit(t, buf, area.x + 1, area.y, self.label, words, ink)
        if self.description:
            text_fit(t, buf, area.x + 1, area.y + 1, self.description, words, dim)
        if self.control is not None:
            w, h = self.control.size(t)
            self.control.draw(t, Rect(max(0, area.right - (w + 1)), area.y, w, h), buf)
        if self.key is not None:
            t.hit(self.key, area)


@dataclass
class MultiSelectState:
    """The keys' state of a MultiSelect: open, the search, the row under the keyboard (None: the
    keys are in the search)."""

    open: bool = False
    search: str = ""
    row: int | None = None


class MultiSelect(Draw):
    """The searchable multi-choice dropdown: a label row (bold `textDim`), the trigger (the chosen
    labels joined by ", ", "N selected" past three, "None selected" in `textFaint`), and when open
    the popup under it: a box in the popups border color with the search row, a rule, then a
    checkbox row per option. The trigger keyed `key`, the rows `(key, value)`."""

    def __init__(self, key: object, options: Iterable[tuple[str, str]], values: Iterable[str], label: str = "",
                 open: bool = False, cursor: bool = False, highlighted: int | None = None, search: str = "",
                 width: int = 33):
        self.key = key
        self.options = list(options)
        self.values = list(values)
        self.label, self.open, self.cursor = label, open, cursor
        self.highlighted, self.search, self.width = highlighted, search, width

    def shown(self) -> list[tuple[str, str]]:
        """The options the search shows (case-insensitive substring of the label)."""
        q = self.search.lower()
        return [o for o in self.options if q in o[1].lower()]

    def trigger_text(self, t) -> tuple[str, bool]:
        chosen = [l for v, l in self.options if v in self.values]
        n = len(chosen)
        if n == 0:
            return (t.string("common.noneSelected", "None selected"), False)
        if n > 3:
            s = t.string("multiSelect.selectedCount", "%1 selected")
            return (s.replace("%1", str(n)).replace("{count}", str(n)), True)
        return (", ".join(chosen), True)

    def popup_rows(self) -> int:
        return min(len(self.shown()), 6)

    @staticmethod
    def handle_key(state: MultiSelectState, options: Sequence[tuple[str, str]], key: str,
                   char: str | None = None) -> tuple[bool, str | None]:
        """The keys, as the GUI bindings take them (`key` a Textual key name, `char` the printable
        character). Closed: j, Down, Return or Space open. Open, in the search: typing filters,
        Return toggles the first match, Down goes into the list. In the list: j/k and Down/Up walk
        (Up off the first row goes back to the search), Return or Space toggles. Escape closes,
        clearing the search. Returns (handled, the value to toggle)."""
        q = state.search.lower()
        shown = [o for o in options if q in o[1].lower()]
        if not state.open:
            if key in ("j", "down", "enter", "space"):
                state.open = True
                state.row = None
                return (True, None)
            return (False, None)
        row = state.row
        if key == "escape":
            state.open = False
            state.search = ""
            state.row = None
            return (True, None)
        if row is None:
            if key == "enter":
                return (True, shown[0][0] if shown else None)
            if key == "down":
                state.row = 0 if shown else None
                return (True, None)
            if key == "backspace":
                state.search = state.search[:-1]
                return (True, None)
            if char:
                state.search += char
                return (True, None)
            return (False, None)
        if key in ("down", "j"):
            state.row = min(row + 1, max(0, len(shown) - 1))
            return (True, None)
        if key in ("up", "k"):
            state.row = row - 1 if row > 0 else None
            return (True, None)
        if key in ("enter", "space"):
            return (True, shown[row][0] if row < len(shown) else None)
        return (False, None)

    def size(self, t) -> tuple[int, int]:
        label = 1 if self.label else 0
        popup = 2 + 2 + max(1, self.popup_rows()) if self.open else 0
        return (self.width, label + 1 + popup)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        y = area.y
        if self.label:
            text_fit(t, buf, area.x, y, self.label, area.width, Ink(t.dim(), bold=True))
            y += 1
        s, chosen = self.trigger_text(t)
        trigger(t, buf, area.x, y, area.width, self.cursor, s, t.fg() if chosen else t.faint())
        t.hit(self.key, Rect(area.x, y, area.width, 1))
        if not self.open:
            return
        rows = max(1, self.popup_rows())
        pop = Rect(area.x, y + 1, area.width, rows + 4).intersection(area)
        clear(buf, pop)
        c = t.shown.controls
        inner = frame(t, buf, pop, t.shown.surfaces.popups.border.color)
        if self.highlighted is None:
            fill(t, buf, Rect(inner.x, inner.y, inner.width, 1), c.focus.fill)
        put(t, buf, inner.x + 1, inner.y, "\U000f0349", t.dim())
        if not self.search:
            text_fit(t, buf, inner.x + 3, inner.y, t.string("common.search", "Search..."), max(0, inner.width - 4), t.faint())
        else:
            text_fit(t, buf, inner.x + 3, inner.y, self.search, max(0, inner.width - 4), t.fg())
        hline(t, buf, inner.x, inner.y + 1, inner.width, "─", t.fg().with_alpha(0.10))
        shown = self.shown()
        if not shown:
            text_fit(t, buf, inner.x + 1, inner.y + 2, t.string("common.noOptions", "No options"), max(0, inner.width - 2), t.dim())
        for i, (v, l) in enumerate(shown[:rows]):
            ry = inner.y + 2 + i
            if self.highlighted == i:
                fill(t, buf, Rect(inner.x, ry, inner.width, 1), c.hover.fill)
            on = v in self.values
            check_box(t, buf, inner.x + 1, ry, on, False, False, False)
            text_fit(t, buf, inner.x + 5, ry, l, max(0, inner.width - 6), Ink(t.fg(), bold=on))
            t.hit((self.key, v), Rect(inner.x, ry, inner.width, 1))
