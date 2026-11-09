"""What every cell component shares: the `Draw` protocol, tones, the state ladder (`look`) and
the glyphs (spec/terminal.md, States)."""

from __future__ import annotations

import enum
from typing import NamedTuple

from ..color import TRANSPARENT, Rgba
from .buffer import Buffer, Rect


class Draw:
    """A kit component in cells: its natural size, and how it draws into an area (normally its
    natural size; wider areas stretch fills where that makes sense)."""

    #: Recorded with `Theme.hit` over the area drawn, when set.
    key: object = None

    def size(self, t) -> tuple[int, int]:
        raise NotImplementedError

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        raise NotImplementedError

    def render(self, t, area: Rect, buf: Buffer) -> None:
        """Draw clipped to the natural size (Ratatui's `On` widget)."""
        w, h = self.size(t)
        self.draw(t, Rect(area.x, area.y, min(area.width, max(w, 1)), min(area.height, max(h, 1))), buf)


class Tone(enum.Enum):
    """A component's color by meaning. `INFO` is accent, `ERROR` urgent."""

    NEUTRAL = "neutral"
    ACCENT = "accent"
    SUCCESS = "success"
    WARNING = "warning"
    URGENT = "urgent"
    MUTED = "muted"
    INFO = "accent"
    ERROR = "urgent"

    def color(self, t, neutral: Rgba) -> Rgba:
        if self is Tone.NEUTRAL:
            return neutral
        return {
            Tone.ACCENT: t.accent,
            Tone.SUCCESS: t.success,
            Tone.WARNING: t.warning,
            Tone.URGENT: t.urgent,
            Tone.MUTED: t.muted,
        }[self]()

    @staticmethod
    def of(v: "Tone | str | None") -> "Tone":
        if v is None:
            return Tone.NEUTRAL
        if isinstance(v, Tone):
            return v
        v = {"info": "accent", "error": "urgent"}.get(v, v)
        return Tone(v)


class Look(NamedTuple):
    """How a control looks in its state: the fill, the edges' color (None: no edges), its label ink
    and weight."""

    fill: Rgba
    border: Rgba | None
    text: Rgba
    bold: bool


def border_of(b) -> Rgba | None:
    """The border color of a state, when it has a width."""
    return None if b.is_none() else b.color


def look(t, hot: bool, selected: bool, rest_fill: bool, rest_border: bool) -> Look:
    """pressed > focus > hover-cursor > selected > normal, for a control that shows `rest_border`
    and `rest_fill` at rest."""
    c = t.shown.controls
    if hot:
        return Look(c.hover.fill, border_of(c.hover.border), c.selected.text if selected else t.fg(), selected)
    if selected:
        border = border_of(c.selected.border)
        if border is None and rest_border:
            border = border_of(c.normal.border)
        return Look(c.selected.fill, border, c.selected.text, True)
    return Look(
        c.normal.fill if rest_fill else TRANSPARENT,
        border_of(c.normal.border) if rest_border else None,
        t.fg(),
        False,
    )


class glyphs:
    """Spinner frames, chevrons and marks the components share."""

    CHECK = "✓"
    DOT = "●"
    CHEVRON_DOWN = "\U000f0140"
    CHEVRON_RIGHT = "\U000f0142"
    CHEVRON_LEFT = "\U000f0141"
    CHEVRON_UP = "\U000f0143"
    SUBMENU = ""
    CLOSE = "\U000f0156"
    LOADING = "\U000f0996"
    SPINNER = ("⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏")


def g(cp: int) -> str:
    """The character at code point `cp` (Nerd Font glyphs by number)."""
    return chr(cp)
