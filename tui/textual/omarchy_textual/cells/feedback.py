"""Levels, work in progress, emptiness and outcomes (spec/terminal.md, Feedback): Meter,
ProgressBar, Spinner, EmptyState, Banner, Toast. A port of `omarchy-ratatui`'s `ui/feedback.rs`,
cell for cell."""

from __future__ import annotations

from typing import Iterable

from ..color import Rgba, f32, rround
from ..tokens import cubic_bezier
from .base import Draw, Look, Tone, glyphs
from .buffer import Buffer, Rect
from .controls import Button, control, wrap
from .paint import Ink, clear, fill, frame, hline, put, text_center, text_fit, text_right

__all__ = ["Banner", "EmptyState", "Meter", "ProgressBar", "Spinner", "Toast"]

#: The curve of the spinners' pulse and the indeterminate bar's sweep.
EASE_IN_OUT = (0.42, 0.0, 0.58, 1.0)


# Meter, ProgressBar ------------------------------------------------------------------------------


class Meter(Draw):
    """A determinate level: an optional header row (label, value text at the end in `textDim`),
    then the bar: `round(value × w)` cells of `━` in the tone, the rest `━` in `track`; markers `╋`
    in the fill's color at `round(f × (w − 1))`."""

    def __init__(self, value: float, tone: Tone | str | None = None, alarming: bool = False,
                 markers: Iterable[float] = (), label: str = "", value_text: str = "", width: int = 28,
                 segment: tuple[int, int] | None = None):
        self.value = f32(min(1.0, max(0.0, value)))
        self.tone = Tone.of(tone)
        self.alarming = alarming
        self.markers = [f32(m) for m in markers]
        self.label, self.value_text, self.width, self.segment = label, value_text, width, segment

    def header(self) -> bool:
        return bool(self.label) or bool(self.value_text)

    def size(self, t) -> tuple[int, int]:
        return (self.width, 2 if self.header() else 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        y = area.y
        if self.header():
            text_fit(t, buf, area.x, y, self.label, area.width, t.fg())
            text_right(t, buf, area.right, y, self.value_text, t.dim())
            y += 1
        w = area.width
        ink = t.urgent() if self.alarming else self.tone.color(t, t.fg())
        hline(t, buf, area.x, y, w, "━", t.track())
        if self.segment is not None:
            frm, to = self.segment
        else:
            frm, to = 0, rround(f32(self.value * w))
        for i in range(frm, min(to, w)):
            put(t, buf, area.x + i, y, "━", ink)
        for m in self.markers:
            i = rround(f32(min(1.0, max(0.0, m)) * max(0, w - 1)))
            put(t, buf, area.x + i, y, "╋", ink)


class ProgressBar(Draw):
    """A Meter in accent by default, plus an indeterminate mode: a segment a quarter of the track
    wide sweeping across every 1.4 s; still (reduced motion, snapshots) it sits at
    `[round(3w/8), round(3w/8) + round(w/4))`."""

    def __init__(self, value: float = 0.0, indeterminate: bool = False, tone: Tone | str | None = None,
                 label: str = "", value_text: str = "", width: int = 28):
        self.indeterminate = indeterminate
        self.meter = Meter(0.0 if indeterminate else value, tone=Tone.INFO if tone is None else tone,
                           label=label, value_text=value_text, width=width)

    def animating(self, t) -> bool:
        """True while it animates."""
        return self.indeterminate and not t.reduce_motion()

    def size(self, t) -> tuple[int, int]:
        return self.meter.size(t)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        if not self.indeterminate:
            self.meter.draw(t, area, buf)
            return
        w = float(area.width)
        seg = float(rround(w / 4.0))
        if t.reduce_motion():
            start = float(rround(3.0 * w / 8.0))
        else:
            p = (t.clock() % 1400) / 1400.0
            e = cubic_bezier(EASE_IN_OUT, p)
            start = float(rround(-seg + (w + seg) * e))
        frm = int(max(0.0, start))
        to = int(min(w, max(0.0, start + seg)))
        m = self.meter
        Meter(0.0, tone=m.tone, label=m.label, value_text=m.value_text, width=area.width,
              segment=(frm, to)).draw(t, area, buf)


# Spinner -----------------------------------------------------------------------------------------


class Spinner(Draw):
    """Busy. Turning: the braille frames, one per 90 ms (a turn in 900 ms); still (reduced motion,
    snapshots) the first. Pulse: the glyph (`󰦖`) breathing between half and full opacity, 420 ms
    each way; still at half."""

    TURN_MS = 900
    PULSE_MS = 840

    def __init__(self, pulse: bool = False, glyph: str = glyphs.LOADING, color: Rgba | None = None):
        self.pulse, self.glyph, self.color = pulse, glyph, color

    @staticmethod
    def frame(clock: int) -> int:
        """The frame index (turning) at `clock` ms."""
        return (clock % Spinner.TURN_MS) // 90

    @staticmethod
    def opacity(clock: int) -> float:
        """The opacity (pulsing) at `clock` ms: 0.5 → 1 → 0.5, eased in and out."""
        p = (clock % Spinner.PULSE_MS) / 420.0
        x = p if p <= 1.0 else 2.0 - p
        return 0.5 + 0.5 * cubic_bezier(EASE_IN_OUT, x)

    def size(self, t) -> tuple[int, int]:
        return (1, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        ink = self.color if self.color is not None else t.fg()
        still = t.reduce_motion()
        if self.pulse:
            o = 0.5 if still else Spinner.opacity(t.clock())
            put(t, buf, area.x, area.y, self.glyph, ink.with_alpha(o))
        else:
            f = 0 if still else Spinner.frame(t.clock())
            put(t, buf, area.x, area.y, glyphs.SPINNER[f], ink)


# EmptyState --------------------------------------------------------------------------------------


class EmptyState(Draw):
    """What a list or panel shows when it has nothing: a centered column of glyph, title, hint and
    a bordered action Button (keyed `(key, "action")`), a blank row before the action. Panel: glyph
    and title `textDim`, hint `textFaint`. Overlay: glyph `menu.selectedText` at 0.8, title
    foreground at 0.7, hint `textDim`."""

    def __init__(self, title: str, glyph: str = "", hint: str = "", action: str = "", overlay: bool = False,
                 cursor: bool = False, width: int = 44, key: object = None):
        self.title, self.glyph, self.hint, self.action = title, glyph, hint, action
        self.overlay, self.cursor, self.width, self.key = overlay, cursor, width, key

    def lines(self) -> tuple[list[str], list[str]]:
        w = self.width
        return (wrap(self.title, w), wrap(self.hint, w) if self.hint else [])

    def size(self, t) -> tuple[int, int]:
        title, hint = self.lines()
        h = len(title) + len(hint)
        if self.glyph:
            h += 1
        if self.action:
            h += 2
        return (self.width, h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        s = t.shown
        if self.key is not None:
            t.hit(self.key, area)
        if self.overlay:
            glyph_ink, title_ink, hint_ink = s.surfaces.menu.selected_text.with_alpha(0.8), t.fg().with_alpha(0.7), t.dim()
        else:
            glyph_ink, title_ink, hint_ink = t.dim(), t.dim(), t.faint()
        y = area.y
        if self.glyph:
            text_center(t, buf, area.x, y, area.width, self.glyph, glyph_ink)
            y += 1
        title, hint = self.lines()
        for l in title:
            text_center(t, buf, area.x, y, area.width, l, title_ink)
            y += 1
        for l in hint:
            text_center(t, buf, area.x, y, area.width, l, hint_ink)
            y += 1
        if self.action:
            b = Button(self.action, bordered=True, cursor=self.cursor,
                       key=(self.key, "action") if self.key is not None else None)
            bw = b.size(t)[0]
            b.draw(t, Rect(area.x + max(0, area.width - bw) // 2, y + 1, bw, 1), buf)


# Banner ------------------------------------------------------------------------------------------

_TONE_GLYPH = {Tone.SUCCESS: "\U000f012c", Tone.WARNING: "\U000f002a", Tone.URGENT: "\U000f015a"}


class Banner(Draw):
    """An inline message, one row: the control frame with its edges in the tone, the normal fill,
    the tone glyph, the text, then an optional bordered action Button (`(key, "action")`) and a
    dismiss `󰅖` (`(key, "dismiss")`)."""

    def __init__(self, text: str, tone: Tone | str | None = None, glyph: str = "", action: str = "",
                 dismissible: bool = False, cursor: bool = False, width: int = 92, key: object = None):
        self.text = text
        self.tone = Tone.INFO if tone is None else Tone.of(tone)
        self.glyph, self.action, self.dismissible = glyph, action, dismissible
        self.cursor, self.width, self.key = cursor, width, key

    def tone_glyph(self) -> str:
        if self.glyph:
            return self.glyph
        return _TONE_GLYPH.get(self.tone, "\U000f02fd")

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        tone = Tone.INFO if self.tone is Tone.NEUTRAL else self.tone
        ink = tone.color(t, t.accent())
        control(t, buf, area.x, area.y, area.width, Look(t.shown.controls.normal.fill, ink, t.fg(), False))
        put(t, buf, area.x + 2, area.y, self.tone_glyph(), ink)
        right = max(0, area.right - 2)
        if self.key is not None:
            t.hit(self.key, area)
        if self.dismissible:
            right = max(0, right - 1)
            put(t, buf, right, area.y, glyphs.CLOSE, t.dim())
            if self.key is not None:
                t.hit((self.key, "dismiss"), Rect(right, area.y, 1, 1))
            right = max(0, right - 1)
        if self.action:
            b = Button(self.action, bordered=True, cursor=self.cursor,
                       key=(self.key, "action") if self.key is not None else None)
            bw = b.size(t)[0]
            right = max(0, right - bw)
            b.draw(t, Rect(right, area.y, bw, 1), buf)
            right = max(0, right - 1)
        text_fit(t, buf, area.x + 4, area.y, self.text, max(0, right - (area.x + 4)), t.fg())


# Toast -------------------------------------------------------------------------------------------


class Toast(Draw):
    """A transient message on the `notifications` surface: a box in its border color, the glyph,
    the bold title over the text (notifications text darker 1.15), an action Button
    (`(key, "action")`) at the end of the first row; the countdown is the bottom edge: its first
    `round(countdown × (w − 2))` cells `━` in `notifications.countdown`. Four rows (three without
    text)."""

    def __init__(self, title: str, text: str = "", glyph: str = "", action: str = "",
                 countdown: float | None = None, cursor: bool = False, width: int = 45, key: object = None):
        self.title, self.text, self.glyph, self.action = title, text, glyph, action
        self.countdown = None if countdown is None else f32(countdown)
        self.cursor, self.width, self.key = cursor, width, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, 3 if not self.text else 4)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        clear(buf, area)
        n = t.shown.surfaces.notifications
        inner = frame(t, buf, area, n.border.color)
        if self.cursor:
            fill(t, buf, inner, t.shown.controls.hover.fill)
        if self.key is not None:
            t.hit(self.key, area)
        x = inner.x + 1
        if self.glyph:
            put(t, buf, x, inner.y, self.glyph, n.text)
            x += 2
        right = max(0, inner.right - 1)
        if self.action:
            b = Button(self.action, bordered=True, key=(self.key, "action") if self.key is not None else None)
            bw = b.size(t)[0]
            right = max(0, right - bw)
            b.draw(t, Rect(right, inner.y, bw, 1), buf)
            right = max(0, right - 1)
        text_fit(t, buf, x, inner.y, self.title, max(0, right - x), Ink(n.text, bold=True))
        if self.text:
            text_fit(t, buf, x, inner.y + 1, self.text, max(0, right - x), n.text.darker(1.15))
        if self.countdown is not None and self.countdown >= 0.0:
            w = max(0, area.width - 2)
            k = rround(f32(min(1.0, max(0.0, self.countdown)) * w))
            hline(t, buf, area.x + 1, area.bottom - 1, k, "━", n.countdown)
