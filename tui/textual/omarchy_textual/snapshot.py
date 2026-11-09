"""Snapshots (spec/terminal.md, Snapshots): a frame written as the canonical ANSI text every
terminal binding's `--snap` writes. Two sources: a cell `Buffer` (the kit's own frame), and
Textual's composed screen (the compositor's strips of segments), so a snapshot of a Textual app is
what Textual put on the screen, never a re-drawing of it.
"""

from __future__ import annotations

from typing import Iterable, NamedTuple

from rich.color import ColorType
from rich.segment import Segment

from .cells.buffer import BOLD, CROSSED, DIM, ITALIC, REVERSE, UNDERLINE, Buffer
from .cells.paint import char_width
from .color import Rgba

ESC = "\x1b"


class SnapCell(NamedTuple):
    text: str
    #: None for the default, (r, g, b), or an int index.
    fg: object
    bg: object
    mod: int


def _color(c) -> object:
    if c is None:
        return None
    if isinstance(c, Rgba):
        return (c.r, c.g, c.b)
    return c


def grid_from_buffer(buf: Buffer) -> list[list[SnapCell | None]]:
    """The buffer as rows of cells; None marks a wide character's second column."""
    a = buf.area
    rows = []
    for y in range(a.y, a.bottom):
        out: list[SnapCell | None] = []
        x = 0
        cells = buf.row(y)
        while x < a.width:
            c = cells[x]
            sw = sum(char_width(ch) for ch in c.sym)
            w = min(2, max(1, sw))
            if x + w > a.width:
                w = 1
            out.append(SnapCell(" " if sw == 0 else c.sym, _color(c.fg), _color(c.bg), c.mod))
            if w == 2:
                out.append(None)
            x += w
        rows.append(out)
    return rows


def _rich_color(c) -> object:
    if c is None or c.type == ColorType.DEFAULT:
        return None
    if c.type == ColorType.TRUECOLOR:
        t = c.triplet
        return (t.red, t.green, t.blue)
    return c.number


def grid_from_strips(strips: Iterable[Iterable[Segment]], cols: int, rows: int) -> list[list[SnapCell | None]]:
    """Textual's composed lines (strips of Rich segments) as rows of cells, exactly `cols` × `rows`
    (cropped, or padded with default blanks)."""
    out_rows = []
    for line in strips:
        if len(out_rows) == rows:
            break
        out: list[SnapCell | None] = []
        col = 0
        for seg in line:
            if seg.control:
                continue
            st = seg.style
            if st is None:
                fg = bg = None
                mod = 0
            else:
                fg = _rich_color(st.color)
                bg = _rich_color(st.bgcolor)
                mod = (
                    (BOLD if st.bold else 0)
                    | (DIM if st.dim else 0)
                    | (ITALIC if st.italic else 0)
                    | (UNDERLINE if st.underline else 0)
                    | (REVERSE if st.reverse else 0)
                    | (CROSSED if st.strike else 0)
                )
            for ch in seg.text:
                w = char_width(ch)
                if w == 0:
                    # A mark joins the cell before it.
                    for i in range(len(out) - 1, -1, -1):
                        if out[i] is not None:
                            out[i] = out[i]._replace(text=out[i].text + ch)
                            break
                    continue
                if col + w > cols:
                    break
                out.append(SnapCell(ch, fg, bg, mod))
                if w == 2:
                    out.append(None)
                col += w
        while col < cols:
            out.append(SnapCell(" ", None, None, 0))
            col += 1
        out_rows.append(out)
    while len(out_rows) < rows:
        out_rows.append([SnapCell(" ", None, None, 0)] * cols)
    return out_rows


_ATTRS = ((BOLD, ";1"), (DIM, ";2"), (ITALIC, ";3"), (UNDERLINE, ";4"), (REVERSE, ";7"), (CROSSED, ";9"))


def _sgr_color(c, base: int) -> str:
    if c is None:
        return ""
    if isinstance(c, int):
        return f";{base};5;{c}"
    return f";{base};2;{c[0]};{c[1]};{c[2]}"


def write_canonical(grid: list[list[SnapCell | None]]) -> str:
    """The canonical form (spec/terminal.md): per cell the style; a space without underline,
    reverse or crossed out takes the default foreground and no bold, faint or italic; a change of
    style writes `ESC[0` and the attributes and colors set; lines start and end in the default."""
    lines = []
    keep = UNDERLINE | REVERSE | CROSSED
    for row in grid:
        parts = []
        prev = (None, None, 0)
        for c in row:
            if c is None:
                continue
            fg, bg, mod = c.fg, c.bg, c.mod
            if c.text == " " and not (mod & keep):
                fg = None
                mod &= keep
            style = (fg, bg, mod)
            if style != prev:
                if style == (None, None, 0):
                    parts.append(f"{ESC}[0m")
                else:
                    s = "".join(code for bit, code in _ATTRS if mod & bit)
                    parts.append(f"{ESC}[0{s}{_sgr_color(fg, 38)}{_sgr_color(bg, 48)}m")
                prev = style
            parts.append(c.text)
        if prev != (None, None, 0):
            parts.append(f"{ESC}[0m")
        parts.append("\n")
        lines.append("".join(parts))
    return "".join(lines)


def buffer_ansi(buf: Buffer) -> str:
    """A finished buffer as canonical snapshot text."""
    return write_canonical(grid_from_buffer(buf))
