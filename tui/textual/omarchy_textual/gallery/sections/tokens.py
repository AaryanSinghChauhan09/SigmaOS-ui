"""Tokens: the palette swatches, the control state chips and the type scale, each drawn by a
`Canvas` from the theme at draw time (so they fade)."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells import paint
from ...cells.buffer import Rect
from ...cells.paint import Ink
from ...widgets import Canvas, Col
from ..page import CONTENT, Section

NAMES = ["foreground", "background", "accent", "urgent", "muted"]


def swatches(t, area: Rect, buf) -> None:
    p = t.shown.palette
    colors = [p.foreground, p.background, p.accent, p.urgent, p.muted]
    border = t.shown.controls.normal.border.color
    for i, (n, c) in enumerate(zip(NAMES, colors)):
        x = area.x + i * 13
        # The swatch is its color as background, outlined in the normal border at its outer
        # columns, so the background swatch shows too.
        paint.fill(t, buf, Rect(x, area.y, 8, 2), c)
        for dy in range(2):
            paint.edges(t, buf, x, area.y + dy, 8, border)
        paint.text(t, buf, x, area.y + 2, n, 12, t.fg())


def chips(t, area: Rect, buf) -> None:
    c = t.shown.controls
    states = [
        ("normal", c.normal.fill, c.normal.border, t.fg(), False),
        ("hover-cursor", c.hover.fill, c.hover.border, t.fg(), False),
        ("focus", c.focus.fill, c.focus.border, t.fg(), False),
        ("selected", c.selected.fill, c.selected.border, c.selected.text, True),
    ]
    for i, (n, f, b, ink, bold) in enumerate(states):
        x = area.x + i * 16
        paint.fill(t, buf, Rect(x, area.y, 15, 1), f)
        if not b.is_none():
            paint.edges(t, buf, x, area.y, 15, b.color)
        paint.text_center(t, buf, x + 1, area.y, 13, n, Ink(ink, bold=bold))


def scale(t, area: Rect, buf) -> None:
    f = t.shown.font
    rows = [
        ("displayLarge", f.display_large, True, t.fg()),
        ("display", f.display, True, t.fg()),
        ("heading", f.heading, True, t.fg()),
        ("title", f.title, True, t.fg()),
        ("subtitle", f.subtitle, False, t.fg()),
        ("body", f.body, False, t.fg()),
        ("bodySmall", f.body_small, False, t.fg()),
        ("caption", f.caption, False, t.dim()),
    ]
    for i, (n, px, bold, ink) in enumerate(rows):
        y = area.y + i
        paint.text(t, buf, area.x, y, f"{n} {px}", 16, t.dim())
        paint.text(t, buf, area.x + 16, y, "The quick brown fox jumps over the lazy dog", 60, Ink(ink, bold=bold))


class Tokens(Section):
    NAME, TITLE = "tokens", "Tokens"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            yield Canvas(swatches, CONTENT, 3, id="swatches")
            yield Canvas(chips, CONTENT, 1, id="chips")
            yield Canvas(scale, CONTENT, 8, id="scale")
