"""The editing pieces as Textual widgets (spec/terminal.md, Editing): a Timeline and a Scrubber
that seek with the keys and the mouse, trim and crop handles, the drag grip and drop indicator,
and the ColorPicker."""

from __future__ import annotations

from typing import Iterable

from textual import events
from textual.message import Message
from textual.widget import Widget

from ..cells import editing as E
from ..cells.base import Draw
from ..cells.editing import Clip, Edge, Hsva
from ..widget import KitWidget, kit_reactive

__all__ = [
    "Clip",
    "ColorPicker",
    "CropFrame",
    "DragHandle",
    "DropIndicator",
    "Edge",
    "Hsva",
    "Scrubber",
    "Timeline",
    "TrimHandle",
]


class _Changed(Message):
    def __init__(self, widget: Widget) -> None:
        super().__init__()
        self.widget = widget

    @property
    def control(self) -> Widget:
        return self.widget


class Timeline(KitWidget, can_focus=True):
    """A ruler over lanes of `Clip`s with a playhead, `width` × `height` cells. Left/Right (h/l)
    seek a second, Shift+Left/Right a labeled step, Home/End the ends; a click on the ruler seeks
    (`Timeline.Seeked`), a click on a clip selects it (`Timeline.ClipSelected`)."""

    class Seeked(_Changed):
        def __init__(self, timeline: "Timeline", position: float) -> None:
            super().__init__(timeline)
            self.position = position

    class ClipSelected(_Changed):
        def __init__(self, timeline: "Timeline", track: int, index: int) -> None:
            super().__init__(timeline)
            self.track = track
            self.index = index

    position = kit_reactive(0.0)
    selected = kit_reactive(None)

    def __init__(self, duration: float, tracks: Iterable[Iterable[Clip]] = (), *, position: float = 0.0,
                 selected: tuple[int, int] | None = None, width: int = 90, height: int = 10, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.duration = max(duration, 0.001)
        self.tracks = [list(c) for c in tracks]
        self.box = (width, height)
        self.pinned_cursor = cursor
        self.set_reactive(Timeline.position, position)
        self.set_reactive(Timeline.selected, selected)

    def component(self) -> Draw:
        return E.Timeline(self.duration, self.position, self.tracks, selected=self.selected, cursor=self.hot,
                          size=self.box, key=self)

    def seek(self, position: float) -> None:
        if position != self.position:
            self.position = position
            self.post_message(self.Seeked(self, position))

    def on_key(self, event: events.Key) -> None:
        p = E.Timeline.handle_key(self.position, self.duration, self.box[0], event.key)
        if p is not None:
            event.stop()
            self.seek(p)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, tuple) and p[0] == "clip":
            event.stop()
            self.selected = (p[1], p[2])
            self.post_message(self.ClipSelected(self, p[1], p[2]))
        elif p == "ruler":
            event.stop()
            r = self.rect_of((self, "ruler"))
            x = self.local_x(event)
            if r is not None and x is not None:
                self.seek(E.Timeline.time_at(x - r.x, self.duration, r.width))


class Scrubber(KitWidget, can_focus=True):
    """`0:42 ━━━━●──── 2:10`: Left/Right (h/l) seek 5 s, Shift 30 s, Home/End the ends; a click on
    the bar seeks (`Scrubber.Seeked`)."""

    class Seeked(_Changed):
        def __init__(self, scrubber: "Scrubber", position: float) -> None:
            super().__init__(scrubber)
            self.position = position

    position = kit_reactive(0.0)

    def __init__(self, position: float, duration: float, *, width: int = 45, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.duration = max(duration, 0.001)
        self.width = width
        self.pinned_cursor = cursor
        self.set_reactive(Scrubber.position, position)

    def component(self) -> Draw:
        return E.Scrubber(self.position, self.duration, width=self.width, cursor=self.hot, key=self)

    def seek(self, position: float) -> None:
        if position != self.position:
            self.position = position
            self.post_message(self.Seeked(self, position))

    def on_key(self, event: events.Key) -> None:
        p = E.Scrubber.handle_key(self.position, self.duration, event.key)
        if p is not None:
            event.stop()
            self.seek(p)

    def on_click(self, event: events.Click) -> None:
        if self.part_at(event) == "bar":
            event.stop()
            r = self.rect_of((self, "bar"))
            x = self.local_x(event)
            if r is not None and x is not None:
                f = max(0, x - r.x) / max(1, r.width - 1)
                self.seek(min(max(f * self.duration, 0.0), self.duration))


class TrimHandle(KitWidget):
    """An in/out handle: a trim cap (`crop=False`, `height` rows) or a crop corner or side."""

    def __init__(self, edge: Edge, *, crop: bool = False, height: int | None = None, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.edge, self.crop, self.height_ = edge, crop, height
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return E.TrimHandle(self.edge, crop=self.crop, cursor=self.hot, height=self.height_)


class CropFrame(KitWidget):
    """A crop rectangle `width` × `height`: the accent outline, thirds guides, eight handles."""

    def __init__(self, width: int, height: int, *, cursor: Edge | None = None, **kw):
        super().__init__(**kw)
        self.box = (width, height)
        self.cursor = cursor

    def component(self) -> Draw:
        return E.CropFrame(*self.box, cursor=self.cursor)


class DragHandle(KitWidget):
    """The grip `⠿` a row is dragged by (the foreground when hot)."""

    def __init__(self, *, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return E.DragHandle(cursor=self.hot)


class DropIndicator(KitWidget):
    """Where a dragged row will land: a row of `━` in accent, `width` wide."""

    def __init__(self, width: int, **kw):
        super().__init__(**kw)
        self.width = width

    def component(self) -> Draw:
        return E.DropIndicator(self.width)


class ColorPicker(KitWidget, can_focus=True):
    """An HSV picker (`Hsva`), `width` × `height` cells: the arrows (h/j/k/l) move the square's
    knob 1% (Shift 10%), a click on the square picks there, a click on a palette swatch takes its
    color (keeping the alpha). Posts `ColorPicker.Changed`."""

    class Changed(_Changed):
        def __init__(self, picker: "ColorPicker", color: Hsva) -> None:
            super().__init__(picker)
            self.color = color

    color = kit_reactive(Hsva(0.0, 0.0, 0.0, 1.0))

    def __init__(self, color: Hsva, *, alpha: bool = True, width: int = 38, height: int = 16,
                 palette: Iterable[str] = E.PALETTE, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.alpha = alpha
        self.box = (width, height)
        self.palette = list(palette)
        self.pinned_cursor = cursor
        self.set_reactive(ColorPicker.color, color)

    def _cells(self) -> E.ColorPicker:
        return E.ColorPicker(self.color, alpha=self.alpha, size=self.box, palette=self.palette, cursor=self.hot, key=self)

    def component(self) -> Draw:
        return self._cells()

    def pick(self, color: Hsva) -> None:
        if color != self.color:
            self.color = color
            self.post_message(self.Changed(self, color))

    def on_key(self, event: events.Key) -> None:
        c = E.ColorPicker.handle_key(self.color, event.key)
        if c is not None:
            event.stop()
            self.pick(c)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, tuple) and p[0] == "swatch":
            event.stop()
            colors = self._cells().swatches(self.theme_)
            if p[1] < len(colors):
                h = Hsva.from_hex(colors[p[1]].hex()[:7], self.color.h)
                if h is not None:
                    self.pick(h.with_(a=self.color.a))
        elif p == "square":
            event.stop()
            r = self.rect_of((self, "square"))
            off = self.root_offset(event)
            if r is not None and off is not None:
                s = (off.x - r.x) / max(1, r.width - 1)
                v = 1.0 - (off.y - r.y) / max(1, r.height - 1)
                self.pick(self.color.with_(s=min(max(s, 0.0), 1.0), v=min(max(v, 0.0), 1.0)))
