"""Tables, trees, split panes, sidebars and the month grid (spec/terminal.md, Data): a port of
`omarchy-ratatui`'s `ui/data.rs`, cell for cell. Rows share one ladder: the hover fill on the
cursor row, else the selected fill (bold, selected text) on the selected one, else nothing."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Iterable, Sequence

from ..color import TRANSPARENT, Rgba
from .base import Draw, glyphs
from .buffer import Buffer, Rect
from .paint import Ink, edges, fill, fit, hline, put, text, text_center, text_fit, vline, width

__all__ = [
    "Calendar",
    "Column",
    "Date",
    "Sidebar",
    "SidebarItem",
    "SidebarSection",
    "SplitPane",
    "SplitView",
    "Table",
    "Tree",
    "TreeNode",
    "row_look",
]


def row_look(t, cursor: bool, selected: bool) -> tuple[Rgba, Ink]:
    """The row fill and label ink for a row that may be the cursor and may be selected."""
    c = t.shown.controls
    if cursor:
        f = c.hover.fill
    elif selected:
        f = c.selected.fill
    else:
        f = TRANSPARENT
    ink = Ink(c.selected.text, bold=True) if selected else Ink(t.fg())
    return f, ink


# Table -----------------------------------------------------------------------------------------


@dataclass
class Column:
    """A column: a title, a fixed width in cells (else it shares the slack), right-aligned or not."""

    title: str
    width: int | None = None
    right: bool = False


class Table(Draw):
    """A header of bold `textDim` titles (the sorted one with a chevron), a `track` rule, then a
    row per item. Rows keyed `(key, r)`, headers `(key, ("col", i))`."""

    def __init__(self, columns: Iterable[Column], rows: Iterable[Sequence[str]] = (), sort: tuple[int, bool] | None = None,
                 selected: int | None = None, cursor: int | None = None, width: int = 60, key: object = None):
        self.columns = list(columns)
        self.rows = [list(r) for r in rows]
        self.sort, self.selected, self.cursor, self.width, self.key = sort, selected, cursor, width, key

    def widths(self, total: int) -> list[int]:
        """Fixed columns as given, the rest share `floor((width − fixed) / flex)`."""
        fixed = sum(c.width for c in self.columns if c.width is not None)
        flex = sum(1 for c in self.columns if c.width is None)
        share = max(0, total - fixed) // flex if flex else 0
        return [c.width if c.width is not None else share for c in self.columns]

    @staticmethod
    def handle_key(cursor: int, count: int, key: str) -> int | None:
        """Up/Down/k/j, Home/End, Page Up/Down move the cursor; the new row, or None."""
        last = max(0, count - 1)
        if key in ("down", "j"):
            return min(last, cursor + 1)
        if key in ("up", "k"):
            return max(0, cursor - 1)
        if key == "home":
            return 0
        if key == "end":
            return last
        if key == "pagedown":
            return min(last, cursor + 5)
        if key == "pageup":
            return max(0, cursor - 5)
        return None

    def size(self, t) -> tuple[int, int]:
        return (self.width, 2 + len(self.rows))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        ws = self.widths(area.width)
        x = area.x
        for i, (c, w) in enumerate(zip(self.columns, ws)):
            inner = max(0, w - 2)
            ink = Ink(t.dim(), bold=True)
            chevron = None
            if self.sort is not None and self.sort[0] == i:
                chevron = glyphs.CHEVRON_UP if self.sort[1] else glyphs.CHEVRON_DOWN
            tw = min(width(c.title), max(0, inner - (2 if chevron is not None else 0)))
            if c.right:
                start = x + 1 + inner - tw
                text_fit(t, buf, start, area.y, c.title, tw, ink)
                if chevron is not None:
                    put(t, buf, max(0, start - 2), area.y, chevron, t.dim())
            else:
                text_fit(t, buf, x + 1, area.y, c.title, tw, ink)
                if chevron is not None:
                    put(t, buf, x + 1 + tw + 1, area.y, chevron, t.dim())
            if self.key is not None:
                t.hit((self.key, ("col", i)), Rect(x, area.y, w, 1))
            x += w
        hline(t, buf, area.x, area.y + 1, area.width, "─", t.track())
        for r, row in enumerate(self.rows):
            y = area.y + 2 + r
            if y >= area.bottom:
                break
            f, ink = row_look(t, self.cursor == r, self.selected == r)
            fill(t, buf, Rect(area.x, y, area.width, 1), f)
            x = area.x
            for i, (c, w) in enumerate(zip(self.columns, ws)):
                s = row[i] if i < len(row) else ""
                inner = max(0, w - 2)
                if c.right:
                    s = fit(s, inner)
                    text(t, buf, x + 1 + inner - width(s), y, s, inner, ink)
                else:
                    text_fit(t, buf, x + 1, y, s, inner, ink)
                x += w
            if self.key is not None:
                t.hit((self.key, r), Rect(area.x, y, area.width, 1))


# Tree ------------------------------------------------------------------------------------------


@dataclass
class TreeNode:
    """A node: id, label, optional glyph, children."""

    id: str
    label: str
    glyph: str = ""
    children: list["TreeNode"] = field(default_factory=list)


class Tree(Draw):
    """FileTree-style rows: an inset of `1 + 2 × depth`, the chevron and a space on branches, the
    glyph and a space, the label. Rows keyed `(key, id)`, chevrons `(key, ("toggle", id))`."""

    def __init__(self, nodes: Iterable[TreeNode], expanded: Iterable[str] = (), selected: str | None = None,
                 cursor: str | None = None, width: int = 28, key: object = None):
        self.nodes = list(nodes)
        self.expanded = list(expanded)
        self.selected, self.cursor, self.width, self.key = selected, cursor, width, key

    @staticmethod
    def visible(nodes: Sequence[TreeNode], expanded: Sequence[str]) -> list[tuple[int, TreeNode]]:
        """The visible rows, depth first: (depth, node)."""
        out: list[tuple[int, TreeNode]] = []

        def walk(ns: Sequence[TreeNode], depth: int) -> None:
            for n in ns:
                out.append((depth, n))
                if n.children and n.id in expanded:
                    walk(n.children, depth + 1)

        walk(nodes, 0)
        return out

    def size(self, t) -> tuple[int, int]:
        return (self.width, len(Tree.visible(self.nodes, self.expanded)))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        for i, (depth, n) in enumerate(Tree.visible(self.nodes, self.expanded)):
            y = area.y + i
            if y >= area.bottom:
                break
            selected = self.selected == n.id
            f, ink = row_look(t, self.cursor == n.id, selected)
            fill(t, buf, Rect(area.x, y, area.width, 1), f)
            x = area.x + 1 + 2 * depth
            if n.children:
                is_open = n.id in self.expanded
                put(t, buf, x, y, glyphs.CHEVRON_DOWN if is_open else glyphs.CHEVRON_RIGHT, t.dim())
                if self.key is not None:
                    t.hit((self.key, n.id), Rect(area.x, y, area.width, 1))
                    t.hit((self.key, ("toggle", n.id)), Rect(x, y, 1, 1))
            elif self.key is not None:
                t.hit((self.key, n.id), Rect(area.x, y, area.width, 1))
            x += 2
            if n.glyph:
                put(t, buf, x, y, n.glyph, c.selected.text if selected else t.dim())
                x += 2
            text_fit(t, buf, x, y, n.label, max(0, area.right - (x + 1)), ink)


# SplitView -------------------------------------------------------------------------------------


class SplitPane:
    """A pane of a SplitView: content, a size in cells (None: it fills), a minimum."""

    def __init__(self, content: Draw, size: int | None = None, min: int = 0, visible: bool = True):
        self.content, self.size, self.min, self.visible = content, size, min, visible


class SplitView(Draw):
    """Panes side by side with a one-column handle between each pair: `│` in `track`, in the hover
    fill when hot. Handles keyed `(key, ("handle", i))`; `handles[i]`, when given, is drawn over
    handle `i` (a widget placed there)."""

    def __init__(self, width: int, height: int, panes: Iterable[SplitPane] = (), cursor: int | None = None,
                 key: object = None, handles: Sequence[Draw] | None = None):
        self.width, self.height = width, height
        self.panes = list(panes)
        self.cursor, self.key, self.handles = cursor, key, handles

    def pane(self, p: SplitPane) -> "SplitView":
        self.panes.append(p)
        return self

    def size(self, t) -> tuple[int, int]:
        return (self.width, self.height)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        panes = [p for p in self.panes if p.visible]
        handles = max(0, len(panes) - 1)
        fixed = sum(max(p.size, p.min) for p in panes if p.size is not None)
        flex = sum(1 for p in panes if p.size is None)
        share = max(0, area.width - (fixed + handles)) // flex if flex else 0
        x = area.x
        for i, p in enumerate(panes):
            w = max(p.size, p.min) if p.size is not None else share
            p.content.draw(t, Rect(x, area.y, w, area.height).intersection(area), buf)
            x += w
            if i + 1 < len(panes):
                hr = Rect(x, area.y, 1, area.height)
                if self.cursor == i:
                    fill(t, buf, hr, t.shown.controls.hover.fill)
                vline(t, buf, x, area.y, area.height, "│", t.track())
                if self.key is not None:
                    t.hit((self.key, ("handle", i)), hr)
                if self.handles is not None and i < len(self.handles):
                    self.handles[i].draw(t, hr, buf)
                x += 1


# Sidebar ---------------------------------------------------------------------------------------


@dataclass
class SidebarItem:
    id: str
    label: str
    glyph: str


@dataclass
class SidebarSection:
    title: str
    items: list[SidebarItem]


class Sidebar(Draw):
    """Sections a row apart: a header inset a column, then rows of ` G  Label`. Collapsed: the
    glyphs alone, a 5-column rail. Rows keyed `(key, id)`."""

    RAIL = 5

    def __init__(self, sections: Iterable[SidebarSection], selected: str | None = None, cursor: str | None = None,
                 collapsed: bool = False, width: int = 25, key: object = None):
        self.sections = list(sections)
        self.selected, self.cursor, self.collapsed, self.width, self.key = selected, cursor, collapsed, width, key

    def ids(self) -> list[str]:
        return [it.id for s in self.sections for it in s.items]

    def size(self, t) -> tuple[int, int]:
        n = len(self.sections)
        rows = sum(len(s.items) for s in self.sections)
        headers = 0 if self.collapsed else n
        return (Sidebar.RAIL if self.collapsed else self.width, rows + headers + max(0, n - 1))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        y = area.y
        for si, s in enumerate(self.sections):
            if si > 0:
                y += 1
            if not self.collapsed:
                text_fit(t, buf, area.x + 1, y, s.title, max(0, area.width - 2), Ink(t.dim(), bold=True))
                y += 1
            for it in s.items:
                if y >= area.bottom:
                    return
                selected = self.selected == it.id
                f, ink = row_look(t, self.cursor == it.id, selected)
                fill(t, buf, Rect(area.x, y, area.width, 1), f)
                put(t, buf, area.x + (2 if self.collapsed else 1), y, it.glyph, c.selected.text if selected else t.dim())
                if not self.collapsed:
                    text_fit(t, buf, area.x + 4, y, it.label, max(0, area.width - 5), ink)
                if self.key is not None:
                    t.hit((self.key, it.id), Rect(area.x, y, area.width, 1))
                y += 1


# Calendar --------------------------------------------------------------------------------------


@dataclass(frozen=True, order=True)
class Date:
    """A calendar date."""

    year: int
    month: int
    day: int

    @staticmethod
    def days_in_month(year: int, month: int) -> int:
        if month in (1, 3, 5, 7, 8, 10, 12):
            return 31
        if month in (4, 6, 9, 11):
            return 30
        return 29 if (year % 4 == 0 and year % 100 != 0) or year % 400 == 0 else 28

    def ordinal(self) -> int:
        """Days since 1970-01-01 (Howard Hinnant's days_from_civil)."""
        y = self.year - (1 if self.month <= 2 else 0)
        m, d = self.month, self.day
        era = (y if y >= 0 else y - 399) // 400
        yoe = y - era * 400
        doy = (153 * (m + (-3 if m > 2 else 9)) + 2) // 5 + d - 1
        doe = yoe * 365 + yoe // 4 - yoe // 100 + doy
        return era * 146097 + doe - 719468

    @staticmethod
    def from_ordinal(z: int) -> "Date":
        z += 719468
        era = (z if z >= 0 else z - 146096) // 146097
        doe = z - era * 146097
        yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365
        doy = doe - (365 * yoe + yoe // 4 - yoe // 100)
        mp = (5 * doy + 2) // 153
        d = doy - (153 * mp + 2) // 5 + 1
        m = mp + 3 if mp < 10 else mp - 9
        y = yoe + era * 400 + (1 if m <= 2 else 0)
        return Date(y, m, d)

    def weekday(self) -> int:
        """0 = Monday … 6 = Sunday."""
        return (self.ordinal() + 3) % 7

    def add_days(self, n: int) -> "Date":
        return Date.from_ordinal(self.ordinal() + n)

    def add_months(self, n: int) -> "Date":
        m0 = self.year * 12 + self.month - 1 + n
        y, m = m0 // 12, m0 % 12 + 1
        return Date(y, m, min(self.day, Date.days_in_month(y, m)))

    def iso(self) -> str:
        return f"{self.year:04}-{self.month:02}-{self.day:02}"


MONTHS = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October",
          "November", "December"]
WEEKDAYS = ["MO", "TU", "WE", "TH", "FR", "SA", "SU"]


class Calendar(Draw):
    """A month from Monday, 28 × 8: the header (`󰅁`, `MONTH YEAR`, `󰅂`), the weekdays, six weeks
    of 4-column day cells. Days keyed `(key, Date)`, the steppers `(key, "prev")`, `(key, "next")`."""

    def __init__(self, year: int, month: int, today: Date | None = None, selected: Date | None = None,
                 cursor: Date | None = None, key: object = None):
        self.year, self.month = year, month
        self.today, self.selected, self.cursor, self.key = today, selected, cursor, key

    @staticmethod
    def first_cell(year: int, month: int) -> Date:
        """The Monday on or before the 1st."""
        first = Date(year, month, 1)
        return first.add_days(-first.weekday())

    @staticmethod
    def handle_key(cursor: Date, key: str) -> Date | None:
        """Arrows (h/j/k/l) move a day or a week, Page Up/Down a month; the new cursor, or None."""
        moves = {"left": -1, "h": -1, "right": 1, "l": 1, "up": -7, "k": -7, "down": 7, "j": 7}
        if key in moves:
            return cursor.add_days(moves[key])
        if key == "pageup":
            return cursor.add_months(-1)
        if key == "pagedown":
            return cursor.add_months(1)
        return None

    def size(self, t) -> tuple[int, int]:
        return (28, 8)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        put(t, buf, area.x + 1, area.y, glyphs.CHEVRON_LEFT, t.fg())
        put(t, buf, area.x + 26, area.y, glyphs.CHEVRON_RIGHT, t.fg())
        title = f"{MONTHS[min(12, max(1, self.month)) - 1].upper()} {self.year}"
        text_center(t, buf, area.x + 3, area.y, 22, title, t.dim())
        if self.key is not None:
            t.hit((self.key, "prev"), Rect(area.x, area.y, 3, 1))
            t.hit((self.key, "next"), Rect(area.x + 25, area.y, 3, 1))
        for i, w in enumerate(WEEKDAYS):
            text(t, buf, area.x + 4 * i + 1, area.y + 1, w, 2, Ink(t.dim(), bold=True))
        start = Calendar.first_cell(self.year, self.month)
        for n in range(42):
            d = start.add_days(n)
            col, row = n % 7, n // 7
            x, y = area.x + col * 4, area.y + 2 + row
            if y >= area.bottom:
                break
            cell = Rect(x, y, 4, 1)
            selected = self.selected == d
            today = self.today == d
            if self.cursor == d:
                fill(t, buf, cell, c.hover.fill)
            elif selected:
                fill(t, buf, cell, c.selected.fill)
            if today:
                edges(t, buf, x, y, 4, t.accent())
            in_month = d.month == self.month
            if selected:
                ink = Ink(c.selected.text, bold=True)
            elif not in_month:
                ink = Ink(t.faint())
            elif col >= 5:
                ink = Ink(t.dim(), bold=today)
            else:
                ink = Ink(t.fg(), bold=today)
            text(t, buf, x + 1, y, f"{d.day:>2}", 2, ink)
            if self.key is not None:
                t.hit((self.key, d), cell)
