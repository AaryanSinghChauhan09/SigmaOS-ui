"""Cell painting in the kit's terms (spec/terminal.md): colors with alpha composite over the cell
they land on (its painted background, else the theme background), box corners follow the radius,
controls frame themselves with eighth-block edges. Every component draws through these; an app's
own drawing (`Canvas`) can too.
"""

from __future__ import annotations

from functools import lru_cache
from typing import NamedTuple

from wcwidth import wcwidth

from ..color import Rgba, over
from .buffer import BOLD, CROSSED, ITALIC, REVERSE, UNDERLINE, Buffer, Rect

#: A control's left edge: U+258F LEFT ONE EIGHTH BLOCK.
EDGE_LEFT = "▏"
#: A control's right edge: U+2595 RIGHT ONE EIGHTH BLOCK.
EDGE_RIGHT = "▕"
ELLIPSIS = "…"


class Ink(NamedTuple):
    """A color (alpha composites over the cell's background) and attributes."""

    color: Rgba
    bold: bool = False
    italic: bool = False
    underline: bool = False

    def modifier(self) -> int:
        m = 0
        if self.bold:
            m |= BOLD
        if self.italic:
            m |= ITALIC
        if self.underline:
            m |= UNDERLINE
        return m


def ink(c: "Rgba | Ink", bold: bool | None = None) -> Ink:
    i = c if isinstance(c, Ink) else Ink(c)
    if bold is not None:
        i = i._replace(bold=bold)
    return i


@lru_cache(maxsize=8192)
def char_width(ch: str) -> int:
    """A character's cells: wcwidth, Nerd Font glyphs (private use) 1, controls 0."""
    w = wcwidth(ch)
    return w if w > 0 else 0


@lru_cache(maxsize=8192)
def width(s: str) -> int:
    """Display width in cells."""
    return sum(char_width(ch) for ch in s)


def fit(s: str, w: int) -> str:
    """`s` cut to `w` cells, ending in "…" when it had to be cut."""
    if width(s) <= w:
        return s
    if w <= 0:
        return ""
    out = []
    used = 0
    for ch in s:
        cw = char_width(ch)
        if used + cw > w - 1:
            break
        out.append(ch)
        used += cw
    return "".join(out) + ELLIPSIS


def under(t, buf: Buffer, x: int, y: int) -> Rgba:
    """The background a cell shows now: what was painted there, else the theme background."""
    c = buf.cell(x, y)
    if c is not None and isinstance(c.bg, Rgba):
        return c.bg
    return t.bg()


def fill(t, buf: Buffer, area: Rect, c: Rgba) -> None:
    """Composite `c` over the background of every cell in `area` (the characters stay)."""
    if c.a == 0:
        return
    a = area.intersection(buf.area)
    tbg = t.bg()
    for y in range(a.y, a.bottom):
        for x in range(a.x, a.right):
            cell = buf.cell(x, y)
            u = cell.bg if isinstance(cell.bg, Rgba) else tbg
            cell.bg = over(c, u)


def clear(buf: Buffer, area: Rect) -> None:
    """Blank the cells of `area`: spaces on the theme background (a surface drawn over something)."""
    a = area.intersection(buf.area)
    for y in range(a.y, a.bottom):
        for x in range(a.x, a.right):
            buf.cell(x, y).reset()


def put(t, buf: Buffer, x: int, y: int, sym: str, i: "Rgba | Ink") -> None:
    """Put one symbol (a character and any marks on it) at a cell in `ink`."""
    cell = buf.cell(x, y)
    if cell is None:
        return
    i = i if isinstance(i, Ink) else Ink(i)
    u = cell.bg if isinstance(cell.bg, Rgba) else t.bg()
    cell.sym = sym
    cell.fg = over(i.color, u)
    cell.mod = i.modifier()


def text(t, buf: Buffer, x: int, y: int, s: str, max_w: int, i: "Rgba | Ink") -> int:
    """Write `s` from (x, y), at most `max_w` cells; returns the cells written. Wide characters take
    two cells; marks join the character before them."""
    col = 0
    last: list | None = None
    for ch in s:
        if ord(ch) < 32 or 0x7F <= ord(ch) < 0xA0:
            continue
        cw = char_width(ch)
        if cw == 0:
            if last is not None:
                last[1] += ch
            continue
        if col + cw > max_w:
            break
        if last is not None:
            put(t, buf, last[0], y, last[1], i)
        last = [x + col, ch]
        if cw == 2:
            c = buf.cell(x + col + 1, y)
            if c is not None:
                c.sym = " "
        col += cw
    if last is not None:
        put(t, buf, last[0], y, last[1], i)
    return col


def text_fit(t, buf: Buffer, x: int, y: int, s: str, max_w: int, i: "Rgba | Ink") -> int:
    """`text` elided with "…" to `max_w` cells."""
    return text(t, buf, x, y, fit(s, max_w), max_w, i)


def text_right(t, buf: Buffer, right: int, y: int, s: str, i: "Rgba | Ink") -> int:
    """`text` right-aligned to end at `right` (exclusive)."""
    w = width(s)
    return text(t, buf, max(0, right - w), y, s, w, i)


def text_center(t, buf: Buffer, x: int, y: int, w: int, s: str, i: "Rgba | Ink") -> int:
    """`text` centered in `w` cells from `x` (an odd remainder goes right)."""
    s = fit(s, w)
    sw = width(s)
    return text(t, buf, x + max(0, w - sw) // 2, y, s, sw, i)


def edges(t, buf: Buffer, x: int, y: int, w: int, color: Rgba | None) -> None:
    """A control's edges: `▏` at `x` and `▕` at `x + w − 1` in `color`; None leaves the cells as
    they are (reserved, so a state change never relayouts)."""
    if color is not None and w >= 2:
        put(t, buf, x, y, EDGE_LEFT, color)
        put(t, buf, x + w - 1, y, EDGE_RIGHT, color)


def hline(t, buf: Buffer, x: int, y: int, w: int, sym: str, color: Rgba) -> None:
    for i in range(w):
        put(t, buf, x + i, y, sym, color)


def vline(t, buf: Buffer, x: int, y: int, h: int, sym: str, color: Rgba) -> None:
    for i in range(h):
        put(t, buf, x, y + i, sym, color)


def corners(t) -> tuple[str, str, str, str]:
    """The corners for the theme's radius: square at 0, rounded above."""
    return ("╭", "╮", "╰", "╯") if t.rounded() else ("┌", "┐", "└", "┘")


def frame(t, buf: Buffer, area: Rect, color: Rgba, title: tuple[str, Ink] | None = None) -> Rect:
    """A box border around `area` in `color` (light lines whatever the border width), corners by
    the radius, an optional title in the top edge (`┌ Title ───┐`). Returns the area inside."""
    if area.width < 2 or area.height < 2:
        return area
    tl, tr, bl, br = corners(t)
    x, y, r, b = area.x, area.y, area.right - 1, area.bottom - 1
    put(t, buf, x, y, tl, color)
    put(t, buf, r, y, tr, color)
    put(t, buf, x, b, bl, color)
    put(t, buf, r, b, br, color)
    hline(t, buf, x + 1, y, area.width - 2, "─", color)
    hline(t, buf, x + 1, b, area.width - 2, "─", color)
    vline(t, buf, x, y + 1, area.height - 2, "│", color)
    vline(t, buf, r, y + 1, area.height - 2, "│", color)
    if title is not None and area.width > 4:
        s = f" {fit(title[0], area.width - 4)} "
        text(t, buf, x + 1, y, s, area.width - 2, title[1])
    return Rect(area.x + 1, area.y + 1, area.width - 2, area.height - 2)


def scrim(t, buf: Buffer, area: Rect, c: Rgba) -> None:
    """Composite `c` (a scrim) over everything in `area`: backgrounds and ink alike."""
    if c.a == 0:
        return
    a = area.intersection(buf.area)
    tbg = t.bg()
    for y in range(a.y, a.bottom):
        for x in range(a.x, a.right):
            cell = buf.cell(x, y)
            u = cell.bg if isinstance(cell.bg, Rgba) else tbg
            cell.bg = over(c, u)
            if isinstance(cell.fg, Rgba):
                cell.fg = over(c, cell.fg)


def inset(area: Rect, dx: int, dy: int) -> Rect:
    """`area` inset by `dx` columns and `dy` rows on every side."""
    return Rect(
        area.x + min(dx, area.width // 2),
        area.y + min(dy, area.height // 2),
        max(0, area.width - 2 * dx),
        max(0, area.height - 2 * dy),
    )


def reverse(buf: Buffer, x: int, y: int) -> None:
    """Add reverse video to a cell (a caret)."""
    c = buf.cell(x, y)
    if c is not None:
        c.mod |= REVERSE


__all__ = [
    "CROSSED",
    "EDGE_LEFT",
    "EDGE_RIGHT",
    "ELLIPSIS",
    "Ink",
    "char_width",
    "clear",
    "corners",
    "edges",
    "fill",
    "fit",
    "frame",
    "hline",
    "ink",
    "inset",
    "put",
    "reverse",
    "scrim",
    "text",
    "text_center",
    "text_fit",
    "text_right",
    "under",
    "vline",
    "width",
]
