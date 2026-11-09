"""The cell grid every component draws into (spec/terminal.md, The grid): a rectangle of cells,
each a symbol, a foreground and background (an `Rgba`, or None for the terminal's default; after
`Theme.finish` below truecolor, an ANSI or xterm index as an int) and attributes.
"""

from __future__ import annotations

from dataclasses import dataclass

BOLD = 1
DIM = 2
ITALIC = 4
UNDERLINE = 8
REVERSE = 16
CROSSED = 32


@dataclass(frozen=True, slots=True)
class Rect:
    """`x`, `y`, `width`, `height` in cells (Ratatui's `Rect`)."""

    x: int = 0
    y: int = 0
    width: int = 0
    height: int = 0

    @property
    def right(self) -> int:
        return self.x + self.width

    @property
    def bottom(self) -> int:
        return self.y + self.height

    def intersection(self, o: "Rect") -> "Rect":
        x1, y1 = max(self.x, o.x), max(self.y, o.y)
        x2, y2 = min(self.right, o.right), min(self.bottom, o.bottom)
        return Rect(x1, y1, max(0, x2 - x1), max(0, y2 - y1))

    def contains(self, x: int, y: int) -> bool:
        return self.x <= x < self.right and self.y <= y < self.bottom

    def replace(self, **kw) -> "Rect":
        d = {"x": self.x, "y": self.y, "width": self.width, "height": self.height}
        d.update(kw)
        return Rect(**d)

    def is_empty(self) -> bool:
        return self.width == 0 or self.height == 0


class Cell:
    __slots__ = ("sym", "fg", "bg", "mod")

    def __init__(self, sym: str = " ", fg=None, bg=None, mod: int = 0):
        self.sym = sym
        self.fg = fg
        self.bg = bg
        self.mod = mod

    def reset(self) -> None:
        self.sym = " "
        self.fg = None
        self.bg = None
        self.mod = 0

    def copy(self) -> "Cell":
        return Cell(self.sym, self.fg, self.bg, self.mod)

    def __repr__(self) -> str:
        return f"Cell({self.sym!r}, {self.fg}, {self.bg}, {self.mod})"


class Buffer:
    """A `area`-sized grid of cells; writes outside it are dropped."""

    __slots__ = ("area", "cells")

    def __init__(self, area: Rect, bg=None):
        self.area = area
        self.cells = [Cell(bg=bg) for _ in range(area.width * area.height)]

    def cell(self, x: int, y: int) -> Cell | None:
        a = self.area
        if a.x <= x < a.x + a.width and a.y <= y < a.y + a.height:
            return self.cells[(y - a.y) * a.width + (x - a.x)]
        return None

    def row(self, y: int) -> list[Cell]:
        a = self.area
        i = (y - a.y) * a.width
        return self.cells[i : i + a.width]
