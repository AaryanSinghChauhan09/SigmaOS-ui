"""Layout widgets: the kit's columns, rows and boxes (spec/terminal.md, Metrics and Gallery: groups
1 row apart, side-by-side items 1 or 2 columns apart). Use them like Textual's `Vertical` and
`Horizontal` (`with Col(gap=1): ...`); inside them every child sits at its natural size, exactly
where the reference puts it. A `Canvas` draws whatever its function paints."""

from __future__ import annotations

from typing import Callable

from textual.widget import Widget

from ..cells import layout as L
from ..cells.base import Draw
from ..cells.buffer import Buffer, Rect
from ..widget import KitWidget, kit_reactive


class Col(KitWidget):
    """Children top to bottom, `gap` rows apart, at their natural sizes (`width` fixes the
    column's width)."""

    gap = kit_reactive(0)

    def __init__(self, *children: Widget, gap: int = 0, width: int | None = None, **kw):
        super().__init__(*children, **kw)
        self.gap = gap
        self.fixed_width = width

    def component(self) -> Draw:
        return L.Col(self.gap, *self.parts(), width=self.fixed_width)


class Row(KitWidget):
    """Children left to right, `gap` columns apart, top-aligned (`center=True`: centered)."""

    gap = kit_reactive(0)

    def __init__(self, *children: Widget, gap: int = 0, center: bool = False, **kw):
        super().__init__(*children, **kw)
        self.gap = gap
        self.center = center

    def component(self) -> Draw:
        return L.Row(self.gap, *self.parts(), center=self.center)


class Box(KitWidget):
    """A fixed `width` × `height` box holding its one child at its natural size: top-left, or
    `center=True`; `fill=True` hands the child the whole box (it stretches)."""

    def __init__(self, child: Widget | None = None, *, width: int, height: int, center: bool = False,
                 fill: bool = False, **kw):
        super().__init__(*([child] if child is not None else []), **kw)
        self.box = (width, height)
        self.center = center
        self.fill = fill

    def component(self) -> Draw:
        w, h = self.box
        parts = self.parts()
        if not parts:
            return L.Blank(w, h)
        if self.fill:
            return L.Filled(w, h, parts[0])
        return L.Boxed(w, h, parts[0], center=self.center)


class _Stacked(Draw):
    def __init__(self, w: int, h: int, items: list[Draw]):
        self.w, self.h, self.items = w, h, items

    def size(self, t) -> tuple[int, int]:
        return (self.w, self.h)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        for d in self.items:
            d.draw(t, area, buf)


class Stack(KitWidget):
    """Children drawn one over another, each given the whole `width` × `height` box, in order: a
    Dialog over the Card it belongs to (its scrim darkens what is under it)."""

    def __init__(self, *children: Widget, width: int, height: int, **kw):
        super().__init__(*children, **kw)
        self.box = (width, height)

    def component(self) -> Draw:
        return _Stacked(self.box[0], self.box[1], self.parts())


class Spacer(KitWidget):
    """Empty space, `width` × `height`."""

    def __init__(self, width: int = 1, height: int = 1, **kw):
        super().__init__(**kw)
        self.box = (width, height)

    def component(self) -> Draw:
        return L.Blank(*self.box)


class Canvas(KitWidget):
    """`width` × `height` cells drawn by `draw(theme, area, buffer)` with the kit's paint
    functions (`omarchy_textual.cells.paint`): the app's own drawing, composited and faded like
    the kit's. Call `refresh()` when what it shows changes."""

    def __init__(self, draw: Callable[[object, Rect, Buffer], None], width: int, height: int, **kw):
        super().__init__(**kw)
        self.draw_fn = draw
        self.box = (width, height)

    def component(self) -> Draw:
        return L.Custom(self.box[0], self.box[1], self.draw_fn)


class Cells(KitWidget):
    """Any cell component (`omarchy_textual.cells`), as a widget: `Cells(lambda: cells.Badge("3"))`.
    The function runs at every draw (so the component reads the theme then)."""

    def __init__(self, make: Callable[[], Draw], **kw):
        super().__init__(**kw)
        self.make = make

    def component(self) -> Draw:
        return self.make()
