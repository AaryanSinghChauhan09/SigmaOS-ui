"""Color depth (spec/terminal.md, Color): every component paints 24-bit colors; the last step of
a frame writes the theme background as the terminal's default and, below truecolor, maps every
color down: to the nearest xterm-256 index (16–255), or to the ANSI index of the nearest theme
role, so the terminal's own (Omarchy-themed) palette supplies it.
"""

from __future__ import annotations

import enum
import os
from functools import lru_cache
from typing import Callable, NamedTuple

from .color import Rgba, over
from .tokens import Tokens


class Depth(enum.Enum):
    TRUE = "24bit"
    ANSI256 = "256"
    ANSI16 = "16"

    @staticmethod
    def detect(env: Callable[[str], str | None] | None = None) -> "Depth":
        """`OMARCHY_UI_COLORS` (`24bit`, `truecolor`, `256`, `16`) when set; else truecolor when
        `COLORTERM` is `truecolor` or `24bit`; else 256 when `TERM` mentions `256color`; else 16."""
        var = env or os.environ.get
        o = (var("OMARCHY_UI_COLORS") or "").strip()
        if o in ("24bit", "truecolor", "24"):
            return Depth.TRUE
        if o == "256":
            return Depth.ANSI256
        if o in ("16", "8"):
            return Depth.ANSI16
        if var("COLORTERM") in ("truecolor", "24bit"):
            return Depth.TRUE
        if "256color" in (var("TERM") or ""):
            return Depth.ANSI256
        return Depth.ANSI16


_LEVELS = (0, 95, 135, 175, 215, 255)


def xterm256(i: int) -> tuple[int, int, int]:
    """The xterm-256 color at `i` (16–255): the 6×6×6 cube, then the 24 grays."""
    if i >= 232:
        v = 8 + 10 * (i - 232)
        return (v, v, v)
    n = max(0, i - 16)
    return (_LEVELS[n // 36], _LEVELS[n // 6 % 6], _LEVELS[n % 6])


_XTERM = [(i, xterm256(i)) for i in range(16, 256)]


def _dist(a, b) -> int:
    return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 + (a[2] - b[2]) ** 2


@lru_cache(maxsize=4096)
def nearest256(c: tuple[int, int, int]) -> int:
    """The nearest xterm-256 index in 16–255 by squared sRGB distance; ties go to the lower index."""
    best_d, best = 1 << 30, 16
    for i, x in _XTERM:
        d = _dist(c, x)
        if d < best_d:
            best_d, best = d, i
    return best


class Role(NamedTuple):
    rgb: tuple[int, int, int]
    #: The ANSI index (0–15), or None for the terminal's default.
    index: int | None
    #: Also set faint (SGR 2): `textFaint`.
    faint: bool = False


def tables(t: Tokens) -> tuple[list[Role], list[Role]]:
    """The 16-color tables (spec/terminal.md): ink roles, then fill roles, each composited over the
    theme background. Earlier entries win exact ties."""
    bg = t.palette.background

    def o(c: Rgba) -> tuple[int, int, int]:
        x = over(c, bg)
        return (x.r, x.g, x.b)

    p, r, c = t.palette, t.roles, t.controls
    ink = [
        Role(o(p.foreground), None),
        Role(o(r.text_dim), 8),
        Role(o(r.text_faint), 8, True),
        Role(o(p.muted), 8),
        Role(o(p.accent), 4),
        Role(o(p.urgent), 1),
        Role(o(p.success), 2),
        Role(o(p.warning), 3),
        Role(o(p.background), 0),
        Role(o(c.normal.border.color), 8),
        Role(o(c.hover.border.color), 8),
        Role(o(r.track), 8),
    ]
    fill = [
        Role(o(p.background), None),
        Role(o(c.normal.fill), None),
        Role(o(c.selected.fill), None),
        Role(o(c.hover.fill), 8),
        Role(o(c.focus.fill), 8),
        Role(o(c.pressed.fill), 8),
        Role(o(p.accent), 4),
        Role(o(p.urgent), 1),
        Role(o(p.success), 2),
        Role(o(p.warning), 3),
        Role(o(p.foreground), 7),
    ]
    return ink, fill


def nearest_role(c: tuple[int, int, int], roles: list[Role]) -> Role:
    best_d, best = 1 << 30, roles[0]
    for r in roles:
        d = _dist(c, r.rgb)
        if d < best_d:
            best_d, best = d, r
    return best
