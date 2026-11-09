"""Menus, keys and the chrome apps move around with (spec/terminal.md, Navigation). A port of
`omarchy-ratatui`'s `ui/navigation.rs`, cell for cell."""

from __future__ import annotations

from typing import Iterable, Sequence

from .base import Draw, Look, glyphs
from .buffer import Buffer, Rect
from .controls import ButtonGroup, IconButton, control
from .paint import Ink, clear, fill, frame, hline, put, text, text_fit, text_right, vline, width

__all__ = [
    "ContextMenu",
    "IconRail",
    "KeyCombo",
    "Keycap",
    "Menu",
    "MenuItem",
    "MenuSection",
    "MenuSeparator",
    "ShortcutSheet",
    "Tabs",
    "Toolbar",
    "ToolbarSeparator",
    "key_label",
    "split_combo",
    "toolbar_separator",
]

_NAMED = {
    "super": "Super", "meta": "Super", "win": "Super", "cmd": "Super", "logo": "Super",
    "ctrl": "Ctrl", "control": "Ctrl",
    "alt": "Alt", "option": "Alt",
    "shift": "⇧",
    "enter": "⏎", "return": "⏎",
    "backspace": "⌫",
    "esc": "Esc", "escape": "Esc",
    "delete": "Del", "del": "Del",
    "up": "↑", "down": "↓", "left": "←", "right": "→",
    "pageup": "PgUp", "pagedown": "PgDn",
    "tab": "Tab", "space": "Space", "home": "Home", "end": "End",
}


def key_label(key: str) -> str:
    """How a key name is drawn on a cap (the same table in every binding)."""
    k = key.strip()
    named = _NAMED.get(k.lower())
    if named:
        return named
    if len(k) == 1:
        return k.upper()
    return k


def split_combo(combo: str) -> list[str]:
    """"Ctrl+Shift+S" as its keys; a lone `+` is the plus key ("Ctrl++")."""
    out: list[str] = []
    cur = ""
    i = 0
    while i < len(combo):
        c = combo[i]
        if c == "+" and cur:
            out.append(cur)
            cur = ""
        elif c == "+":
            out.append("+")
            if i + 1 < len(combo) and combo[i + 1] == "+":
                i += 1
        else:
            cur += c
        i += 1
    if cur:
        out.append(cur)
    return out


class Keycap(Draw):
    """One key: ` Label ` in the normal fill between the normal border's edges."""

    def __init__(self, key: str):
        self.key_name = key

    def size(self, t) -> tuple[int, int]:
        return (width(key_label(self.key_name)) + 4, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        control(t, buf, area.x, area.y, area.width, Look(c.normal.fill, c.normal.border.color, t.fg(), False))
        text(t, buf, area.x + 2, area.y, key_label(self.key_name), max(0, area.width - 4), t.fg())


class KeyCombo(Draw):
    """A combination, "Ctrl+Shift+P", drawn as Keycaps side by side."""

    def __init__(self, combo: str):
        self.combo = combo

    def caps(self) -> list[Keycap]:
        return [Keycap(k) for k in split_combo(self.combo)]

    def size(self, t) -> tuple[int, int]:
        return (sum(c.size(t)[0] for c in self.caps()), 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        x = area.x
        for c in self.caps():
            w = c.size(t)[0]
            c.draw(t, Rect(x, area.y, w, 1).intersection(area), buf)
            x += w


# Menu -----------------------------------------------------------------------------------------


class MenuItem:
    """One row of a `Menu`."""

    def __init__(self, label: str, icon: str = "", shortcut: str = "", check: bool | None = None,
                 radio: bool | None = None, submenu: bool = False, disabled: bool = False,
                 destructive: bool = False, cursor: bool = False, key: object = None):
        self.label, self.icon, self.shortcut = label, icon, shortcut
        self.check, self.radio, self.submenu = check, radio, submenu
        self.disabled, self.destructive, self.cursor, self.key = disabled, destructive, cursor, key

    def mark(self) -> str | None:
        if self.check is True:
            return glyphs.CHECK
        if self.radio is True:
            return glyphs.DOT
        if self.submenu:
            return glyphs.SUBMENU
        return None

    def has_mark_slot(self) -> bool:
        return self.check is not None or self.radio is not None or self.submenu


class MenuSeparator:
    """A `track` rule between a menu's rows."""


class MenuSection:
    """A bold `textDim` caption between a menu's rows."""

    def __init__(self, label: str):
        self.label = label


class Menu(Draw):
    """A menu on the `menu` surface: a box in the menu border color, one row per entry:
    `[glyph ]label ··· hint [mark]`, the cursor row in `menu.selectedBackground` with
    `menu.selectedText`; separators are `track` rules, sections SectionHeaders."""

    def __init__(self, *entries: "MenuItem | MenuSeparator | MenuSection", min_width: int = 20):
        self.entries: list = list(entries)
        self.min_width = min_width

    def item(self, i: MenuItem) -> "Menu":
        self.entries.append(i)
        return self

    def separator(self) -> "Menu":
        self.entries.append(MenuSeparator())
        return self

    def section(self, label: str) -> "Menu":
        self.entries.append(MenuSection(label))
        return self

    def items(self) -> list[MenuItem]:
        return [e for e in self.entries if isinstance(e, MenuItem)]

    def columns(self) -> tuple[bool, bool]:
        its = self.items()
        return (any(i.icon for i in its), any(i.has_mark_slot() for i in its))

    def row_width(self, i: MenuItem) -> int:
        icons, marks = self.columns()
        return (2 + (2 if icons else 0) + width(i.label) + (0 if not i.shortcut else 2 + width(i.shortcut))
                + (2 if marks else 0))

    @staticmethod
    def handle_key(cursor: int, enabled: Sequence[bool], key: str) -> int | None:
        """Move a menu's cursor (an index among its items, disabled ones skipped): Up/Down and k/j
        wrap, Home/End jump. The new cursor, or None for another key."""
        n = len(enabled)
        if n == 0 or not any(enabled):
            return None

        def step(frm: int, d: int) -> int:
            i = frm
            for _ in range(n):
                i = (i + d) % n
                if enabled[i]:
                    return i
            return frm

        if key in ("down", "j"):
            return step(cursor, 1)
        if key in ("up", "k"):
            return step(cursor, -1)
        if key == "home":
            return step(n - 1, 1)
        if key == "end":
            return step(0, -1)
        return None

    def size(self, t) -> tuple[int, int]:
        w = max(max((self.row_width(i) for i in self.items()), default=0), self.min_width)
        return (w + 2, len(self.entries) + 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        clear(buf, area)
        m = t.shown.surfaces.menu
        inner = frame(t, buf, area, m.border.color)
        icons, marks = self.columns()
        for n, e in enumerate(self.entries):
            y = inner.y + n
            if y >= inner.bottom:
                break
            if isinstance(e, MenuSeparator):
                hline(t, buf, inner.x, y, inner.width, "─", t.track())
            elif isinstance(e, MenuSection):
                text_fit(t, buf, inner.x + 1, y, e.label, max(0, inner.width - 2), Ink(t.dim(), bold=True))
            else:
                i = e
                row = Rect(inner.x, y, inner.width, 1)
                hot = i.cursor and not i.disabled
                if hot:
                    fill(t, buf, row, m.selected_background)
                if i.disabled:
                    ink = t.faint()
                elif i.destructive:
                    ink = t.urgent()
                elif hot:
                    ink = m.selected_text
                else:
                    ink = m.text
                x = row.x + 1
                if icons:
                    if i.icon:
                        put(t, buf, x, y, i.icon, ink)
                    x += 2
                right = max(0, row.right - (1 + (2 if marks else 0)))
                hint_w = 0 if not i.shortcut else width(i.shortcut) + 2
                text_fit(t, buf, x, y, i.label, max(0, right - (x + hint_w)), ink)
                if i.shortcut:
                    text_right(t, buf, right, y, i.shortcut, t.faint() if i.disabled else t.dim())
                mark = i.mark()
                if mark is not None:
                    mx = max(0, row.right - 2)
                    if i.disabled:
                        mink = Ink(t.faint())
                    elif i.submenu and i.check is None and i.radio is None:
                        mink = Ink(t.dim())
                    else:
                        mink = Ink(t.accent(), bold=True)
                    put(t, buf, mx, y, mark, mink)
                if i.key is not None and not i.disabled:
                    t.hit(i.key, row)


#: A Menu that opens at a point rather than under a trigger: the same component.
ContextMenu = Menu


# ShortcutSheet --------------------------------------------------------------------------------


class ShortcutSheet(Draw):
    """Grouped shortcuts in `columns` columns (4 apart): each group a SectionHeader, then rows of a
    KeyCombo column and the description two columns after it."""

    def __init__(self, groups: Iterable[tuple[str, Iterable[tuple[str, str]]]] = (), columns: int = 2):
        self.groups = [(title, list(rows)) for title, rows in groups]
        self.columns = max(1, columns)

    def group(self, title: str, rows: Iterable[tuple[str, str]]) -> "ShortcutSheet":
        self.groups.append((title, list(rows)))
        return self

    @staticmethod
    def group_size(t, rows: list[tuple[str, str]]) -> tuple[int, int, int]:
        combo = max((KeyCombo(c).size(t)[0] for c, _ in rows), default=0)
        desc = max((width(d) for _, d in rows), default=0)
        return (combo + 2 + desc, 1 + len(rows), combo)

    def layout(self) -> list[list[int]]:
        cols: list[list[int]] = [[] for _ in range(self.columns)]
        for i in range(len(self.groups)):
            cols[i % self.columns].append(i)
        return cols

    def col_w(self, t, col: list[int]) -> int:
        return max((max(self.group_size(t, self.groups[g][1])[0], width(self.groups[g][0])) for g in col), default=0)

    def size(self, t) -> tuple[int, int]:
        w = h = 0
        for ci, col in enumerate(self.layout()):
            cw = self.col_w(t, col)
            ch = sum(self.group_size(t, self.groups[g][1])[1] for g in col) + max(0, len(col) - 1)
            w += cw + (4 if ci > 0 else 0)
            h = max(h, ch)
        return (w, h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        x = area.x
        for col in self.layout():
            cw = self.col_w(t, col)
            y = area.y
            for g in col:
                title, rows = self.groups[g]
                _, gh, combo_w = self.group_size(t, rows)
                text_fit(t, buf, x, y, title, cw, Ink(t.dim(), bold=True))
                for i, (combo, desc) in enumerate(rows):
                    ry = y + 1 + i
                    kc = KeyCombo(combo)
                    kw = kc.size(t)[0]
                    kc.draw(t, Rect(x, ry, kw, 1).intersection(area), buf)
                    text_fit(t, buf, x + combo_w + 2, ry, desc, max(0, cw - (combo_w + 2)), t.fg())
                y += gh + 1
            x += cw + 4


# Toolbar --------------------------------------------------------------------------------------


class ToolbarSeparator(Draw):
    """A `track` rule `│` with a column clear each side: splits a toolbar's groups."""

    def size(self, t) -> tuple[int, int]:
        return (3, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        put(t, buf, area.x + 1, area.y, "│", t.track())


def toolbar_separator() -> ToolbarSeparator:
    return ToolbarSeparator()


class Toolbar(Draw):
    """Three slots on one row: leading at the left, center centered on the bar, trailing at the
    right, then the overflow IconButton (keyed `overflow_key`); a `track` rule along the bottom row
    when divided."""

    def __init__(self, width: int, leading: Iterable[Draw] = (), center: Iterable[Draw] = (),
                 trailing: Iterable[Draw] = (), overflow: bool = False, overflow_cursor: bool = False,
                 divided: bool = True, overflow_key: object = "toolbar-overflow"):
        self.width = width
        self.leading, self.center, self.trailing = list(leading), list(center), list(trailing)
        self.overflow, self.overflow_cursor, self.divided, self.overflow_key = overflow, overflow_cursor, divided, overflow_key

    @staticmethod
    def slot_w(t, ds: Sequence[Draw]) -> int:
        return sum(d.size(t)[0] for d in ds)

    def height(self, t) -> int:
        return max((d.size(t)[1] for d in [*self.leading, *self.center, *self.trailing]), default=1)

    def size(self, t) -> tuple[int, int]:
        return (self.width, self.height(t) + (1 if self.divided else 0))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        h = self.height(t)

        def place(x: int, ds: Sequence[Draw]) -> None:
            for d in ds:
                w, dh = d.size(t)
                d.draw(t, Rect(x, area.y + (h - dh) // 2, w, dh).intersection(area), buf)
                x += w

        place(area.x, self.leading)
        cw = self.slot_w(t, self.center)
        place(area.x + max(0, area.width - cw) // 2, self.center)
        ow = 5 if self.overflow else 0
        tw = self.slot_w(t, self.trailing)
        place(max(0, area.right - (tw + ow)), self.trailing)
        if self.overflow:
            IconButton("", cursor=self.overflow_cursor, key=self.overflow_key).draw(
                t, Rect(area.right - 5, area.y + (h - 1) // 2, 5, 1), buf)
        if self.divided:
            hline(t, buf, area.x, area.y + h, area.width, "─", t.track())


# Tabs, IconRail -------------------------------------------------------------------------------


class Tabs(Draw):
    """Text tabs ` Label ` over a `track` rule the control's width; the selected tab bold in
    `controls.selected.text` with its span of the rule `━` in accent. Two rows. Tabs keyed
    `(key, i)` (`"tab-<i>"` without a key)."""

    def __init__(self, labels: Iterable[str], current: int = 0, cursor: int | None = None, width: int | None = None,
                 key: object = None):
        self.labels, self.current, self.cursor, self.width, self.key = list(labels), current, cursor, width, key

    @staticmethod
    def handle_key(current: int, count: int, key: str) -> int | None:
        """Left/h, Right/l move `current` within `count`, Home/End jump; the new index, or None."""
        return ButtonGroup.handle_key(current, count, key)

    def tabs_w(self) -> int:
        return sum(width(l) + 2 for l in self.labels)

    def size(self, t) -> tuple[int, int]:
        return (self.width if self.width is not None else self.tabs_w(), 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        hline(t, buf, area.x, area.y + 1, area.width, "─", t.track())
        x = area.x
        for i, l in enumerate(self.labels):
            w = width(l) + 2
            r = Rect(x, area.y, w, 1).intersection(area)
            selected = i == self.current
            hot = self.cursor == i
            if hot:
                fill(t, buf, r, c.hover.fill)
            if selected:
                ink = Ink(c.selected.text, bold=True)
            elif hot:
                ink = Ink(t.fg())
            else:
                ink = Ink(t.dim())
            text(t, buf, x + 1, area.y, l, max(0, w - 2), ink)
            if selected:
                hline(t, buf, x, area.y + 1, min(w, max(0, area.right - x)), "━", t.accent())
            t.hit((self.key, i) if self.key is not None else f"tab-{i}", r)
            x += w


class IconRail(Draw):
    """A vertical tab rail: one row per item ` G `, the selected glyph accent with a `▎` accent bar
    at the rail's leading edge, the cursor item in the hover fill, a `track` rule `│` down its
    trailing edge. Five columns. Items keyed `(key, i)` (`"rail-<i>"` without a key)."""

    def __init__(self, items: Iterable[tuple[str, str]], current: int = 0, cursor: int | None = None,
                 height: int | None = None, key: object = None):
        self.items, self.current, self.cursor, self.height, self.key = list(items), current, cursor, height, key

    @staticmethod
    def handle_key(current: int, count: int, key: str) -> int | None:
        """Up/k, Down/j move `current`, Home/End jump; the new index, or None."""
        if key in ("up", "k"):
            return max(0, current - 1)
        if key in ("down", "j"):
            return min(max(0, count - 1), current + 1)
        if key == "home":
            return 0
        if key == "end":
            return max(0, count - 1)
        return None

    def size(self, t) -> tuple[int, int]:
        return (5, self.height if self.height is not None else len(self.items))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        vline(t, buf, area.x + 4, area.y, area.height, "│", t.track())
        for i, (g, _) in enumerate(self.items):
            y = area.y + i
            if y >= area.bottom:
                break
            r = Rect(area.x + 1, y, 3, 1)
            hot = self.cursor == i
            if hot:
                fill(t, buf, r, c.hover.fill)
            selected = i == self.current
            if selected:
                put(t, buf, area.x, y, "▎", t.accent())
            ink = t.accent() if selected else (t.fg() if hot else t.dim())
            put(t, buf, area.x + 2, y, g, ink)
            t.hit((self.key, i) if self.key is not None else f"rail-{i}", Rect(area.x, y, 4, 1))
