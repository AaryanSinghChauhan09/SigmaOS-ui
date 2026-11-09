"""Timeline (a Timeline, a Scrubber, a reorderable list, a crop frame) and Color (the ColorPicker)."""

from __future__ import annotations

from textual import events
from textual.app import ComposeResult
from textual.message import Message

from ...cells import editing as E
from ...cells import layout as L
from ...cells import paint
from ...cells.base import Draw
from ...cells.buffer import Rect
from ...widget import KitWidget, kit_reactive
from ...widgets import Canvas, Col, Row
from ...widgets.editing import Clip, ColorPicker, Hsva, Scrubber
from ...widgets.editing import Timeline as TimelineWidget
from ..page import Section

DRAG_ROWS = ["Workspaces", "Clock", "Battery", "Network"]


class DragList(KitWidget, can_focus=True):
    """A list whose rows are dragged by their grips `⠿`, the drop slot marked by a DropIndicator
    row: j/k (or the arrows, or a click) move the cursor (`DragList.Moved`)."""

    class Moved(Message):
        def __init__(self, lst: "DragList", row: int) -> None:
            super().__init__()
            self.list = lst
            self.row = row

        @property
        def control(self) -> "DragList":
            return self.list

    cursor = kit_reactive(0)

    def __init__(self, rows: list[str], *, cursor: int = 0, drop: int = 3, width: int = 45, **kw):
        super().__init__(**kw)
        self.rows = list(rows)
        self.drop = drop
        self.width = width
        self.set_reactive(DragList.cursor, cursor)

    def component(self) -> Draw:
        rows, drop, cursor = self.rows, self.drop, self.cursor

        def draw(t, area, buf):
            y = area.y
            for i, label in enumerate(rows):
                if i == drop:
                    E.DropIndicator(area.width).draw(t, Rect(area.x, y, area.width, 1), buf)
                    y += 1
                r = Rect(area.x, y, area.width, 1)
                if i == cursor:
                    paint.fill(t, buf, r, t.shown.controls.hover.fill)
                E.DragHandle(cursor=i == cursor).draw(t, Rect(area.x + 1, y, 1, 1), buf)
                paint.text_fit(t, buf, area.x + 3, y, label, area.width - 4, t.fg())
                t.hit((self, i), r)
                y += 1

        return L.Custom(self.width, len(rows) + 1, draw)

    def move(self, i: int) -> None:
        if i != self.cursor:
            self.cursor = i
            self.post_message(self.Moved(self, i))

    def on_key(self, event: events.Key) -> None:
        if event.key in ("down", "j"):
            self.move(min(self.cursor + 1, len(self.rows) - 1))
        elif event.key in ("up", "k"):
            self.move(max(0, self.cursor - 1))
        else:
            return
        event.stop()

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.move(p)


def _crop(t, area, buf) -> None:
    paint.fill(t, buf, area, t.shown.controls.normal.fill)
    E.CropFrame(area.width, area.height).draw(t, area, buf)


class Timeline(Section):
    NAME, TITLE = "timeline", "Timeline"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            yield TimelineWidget(
                30.0,
                [
                    [Clip(0.0, 7.5, "Intro"), Clip(8.0, 18.0, "Demo"), Clip(18.5, 30.0, "Outro")],
                    [Clip(2.0, 12.0, "Voiceover"), Clip(14.0, 26.0, "Music")],
                ],
                position=12.0, selected=(0, 1), width=90, height=10, id="timeline",
            )
            yield Scrubber(42.0, 130.0, width=45, id="scrubber")
            with Row(gap=2):
                yield DragList(DRAG_ROWS, cursor=1, id="drag")
                yield Canvas(_crop, 30, 9, id="crop")


class Color(Section):
    NAME, TITLE = "color", "Color"

    def compose(self) -> ComposeResult:
        with Col(gap=0):
            yield ColorPicker(Hsva.from_hex("#7aa2f7", 0.0), width=38, height=16, id="picker")
