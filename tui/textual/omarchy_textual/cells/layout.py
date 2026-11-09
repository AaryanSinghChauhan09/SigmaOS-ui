"""Columns, rows and boxes of cell components at their natural sizes (Ratatui's
`omarchy_ratatui::layout`): what the kit's container widgets lay their children out with."""

from __future__ import annotations

from typing import Callable

from . import paint
from .base import Draw
from .buffer import Buffer, Rect


class Col(Draw):
    """Children top to bottom, `gap` rows apart, each at its natural size (or the column's width,
    `stretch`)."""

    def __init__(self, gap: int = 0, *children: Draw, width: int | None = None):
        self.gap = gap
        self.width = width
        self.items: list[tuple[Draw, bool]] = [(c, False) for c in children]

    def child(self, d: Draw) -> "Col":
        self.items.append((d, False))
        return self

    def stretch(self, d: Draw) -> "Col":
        self.items.append((d, True))
        return self

    def size(self, t) -> tuple[int, int]:
        w = h = 0
        for i, (d, _) in enumerate(self.items):
            cw, ch = d.size(t)
            w = max(w, cw)
            h += ch + (self.gap if i > 0 else 0)
        return (self.width if self.width is not None else w, h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        y = area.y
        for d, stretch in self.items:
            cw, ch = d.size(t)
            w = area.width if stretch else min(cw, area.width)
            r = Rect(area.x, y, w, ch).intersection(area)
            if r.height > 0:
                d.draw(t, r.replace(height=min(ch, max(0, area.bottom - y))), buf)
            y += ch + self.gap


def col(gap: int = 0, *children: Draw, width: int | None = None) -> Col:
    return Col(gap, *children, width=width)


class Row(Draw):
    """Children left to right, `gap` columns apart, top-aligned (or centered) at natural sizes."""

    def __init__(self, gap: int = 0, *children: Draw, center: bool = False):
        self.gap = gap
        self.center_items = center
        self.items: list[Draw] = list(children)

    def child(self, d: Draw) -> "Row":
        self.items.append(d)
        return self

    def center(self) -> "Row":
        self.center_items = True
        return self

    def size(self, t) -> tuple[int, int]:
        w = h = 0
        for i, d in enumerate(self.items):
            cw, ch = d.size(t)
            w += cw + (self.gap if i > 0 else 0)
            h = max(h, ch)
        return (w, h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        _, h = self.size(t)
        x = area.x
        for d in self.items:
            cw, ch = d.size(t)
            dy = max(0, h - ch) // 2 if self.center_items else 0
            r = Rect(x, area.y + dy, cw, ch).intersection(area)
            if r.width > 0 and r.height > 0:
                d.draw(t, r, buf)
            x += cw + self.gap


def row(gap: int = 0, *children: Draw, center: bool = False) -> Row:
    return Row(gap, *children, center=center)


class Boxed(Draw):
    """A `w` × `h` box holding one child at its natural size: top-left, or centered."""

    def __init__(self, w: int, h: int, inner: Draw, center: bool = False):
        self.w, self.h, self.inner, self.center = w, h, inner, center

    def size(self, t) -> tuple[int, int]:
        return (self.w, self.h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        cw, ch = self.inner.size(t)
        cw, ch = min(cw, area.width), min(ch, area.height)
        dx, dy = ((area.width - cw) // 2, (area.height - ch) // 2) if self.center else (0, 0)
        self.inner.draw(t, Rect(area.x + dx, area.y + dy, cw, ch), buf)


def sized(w: int, h: int, inner: Draw) -> Boxed:
    return Boxed(w, h, inner)


def centered(w: int, h: int, inner: Draw) -> Boxed:
    return Boxed(w, h, inner, center=True)


class Filled(Draw):
    """`inner` given the whole `w` × `h` box (it stretches to fill)."""

    def __init__(self, w: int, h: int, inner: Draw):
        self.w, self.h, self.inner = w, h, inner

    def size(self, t) -> tuple[int, int]:
        return (self.w, self.h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        self.inner.draw(t, area, buf)


def filled(w: int, h: int, inner: Draw) -> Filled:
    return Filled(w, h, inner)


class Blank(Draw):
    """Empty space."""

    def __init__(self, w: int, h: int):
        self.w, self.h = w, h

    def size(self, t) -> tuple[int, int]:
        return (self.w, self.h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        pass


class Label(Draw):
    """One line of text; `ink(t)` picks its color from the theme at draw time (so it fades)."""

    def __init__(self, text: str, ink: Callable):
        self.text, self.ink = text, ink

    def size(self, t) -> tuple[int, int]:
        return (paint.width(self.text), 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        paint.text_fit(t, buf, area.x, area.y, self.text, area.width, self.ink(t))


def label(text: str, ink: Callable) -> Label:
    return Label(text, ink)


class Custom(Draw):
    """Anything drawn by `f(t, area, buf)`, at a fixed size."""

    def __init__(self, w: int, h: int, f: Callable[..., None]):
        self.w, self.h, self.f = w, h, f

    def size(self, t) -> tuple[int, int]:
        return (self.w, self.h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        self.f(t, area, buf)


def custom(w: int, h: int, f: Callable[..., None]) -> Custom:
    return Custom(w, h, f)
