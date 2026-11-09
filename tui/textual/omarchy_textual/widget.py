"""`KitWidget`: a Textual widget drawn by the kit's cell components.

Every kit widget builds a cell component from its own state (`component()`), and is drawn by the
nearest kit widget above it that isn't inside another one: the *root*. The root paints its whole
tree into one cell buffer, in order, the way the reference (Ratatui) paints a frame: so a child's
colors composite over what its parent painted, a scrim darkens what is under it, and the cells are
exactly the reference's. The children are still ordinary Textual widgets: each is placed (absolute
position, width and height) where its component was drawn, takes focus, keys, clicks and hover
itself, posts its own messages, and shows its part of the root's buffer.

A kit widget outside any other kit widget (in a `Vertical`, a `Grid`, the screen) is a root and
sizes itself (`width: auto; height: auto`) to its component's natural size.
"""

from __future__ import annotations

from functools import lru_cache
from typing import Iterable

from rich.color import Color as RichColor
from rich.segment import Segment
from rich.style import Style
from textual import events
from textual.geometry import Region, Size
from textual.strip import Strip
from textual.widget import Widget

from .cells.base import Draw
from .cells.buffer import BOLD, CROSSED, DIM, ITALIC, REVERSE, UNDERLINE, Buffer, Rect
from .cells.paint import char_width
from .color import Rgba

_DEFAULT = RichColor.default()


@lru_cache(maxsize=4096)
def _rich(c) -> RichColor:
    if c is None:
        return _DEFAULT
    if isinstance(c, int):
        return RichColor.from_ansi(c)
    return RichColor.from_rgb(c.r, c.g, c.b)


@lru_cache(maxsize=8192)
def _style(fg, bg, mod: int) -> Style:
    return Style(
        color=_rich(fg),
        bgcolor=_rich(bg),
        bold=bool(mod & BOLD),
        dim=bool(mod & DIM),
        italic=bool(mod & ITALIC),
        underline=bool(mod & UNDERLINE),
        reverse=bool(mod & REVERSE),
        strike=bool(mod & CROSSED),
    )


def strip_of(buf: Buffer, y: int, x0: int, x1: int) -> Strip:
    """Row `y` of a finished buffer, columns `x0`..`x1`, as a Textual strip."""
    segs: list[Segment] = []
    cells = buf.row(y)
    run: list[str] = []
    key = None
    x = x0
    width = 0
    while x < x1:
        c = cells[x]
        w = sum(char_width(ch) for ch in c.sym)
        sym = c.sym if w > 0 else " "
        if w == 2 and x + 1 >= x1:
            sym, w = " ", 1
        k = (c.fg, c.bg, c.mod)
        if k != key:
            if run:
                segs.append(Segment("".join(run), _style(*key)))
            run = []
            key = k
        run.append(sym)
        width += max(1, w)
        x += max(1, w)
    if run:
        segs.append(Segment("".join(run), _style(*key)))
    return Strip(segs, width)


class _Frame:
    __slots__ = ("buf", "hits", "places")

    def __init__(self, buf: Buffer, hits: list, places: dict):
        self.buf = buf
        self.hits = hits
        self.places = places


class Placed(Draw):
    """A widget's component, recording where it was drawn so the widget can be placed there."""

    def __init__(self, widget: "KitWidget", inner: Draw):
        self.widget = widget
        self.inner = inner

    def size(self, t) -> tuple[int, int]:
        return self.inner.size(t)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        t.hit(self.widget._place_key, area)
        self.inner.draw(t, area, buf)


class StockSlot(Draw):
    """A stock Textual widget inside a kit container: room for it at its size, drawn by Textual."""

    def __init__(self, widget: Widget):
        self.widget = widget
        self.key = _PlaceKey(widget)

    def size(self, t) -> tuple[int, int]:
        st = self.widget.styles
        w = st.width.cells if st.width is not None and st.width.is_cells else None
        h = st.height.cells if st.height is not None and st.height.is_cells else None
        if w is None:
            w = self.widget.get_content_width(Size(96, 50), Size(96, 50))
        if h is None:
            h = self.widget.get_content_height(Size(w, 50), Size(w, 50), w)
        return (int(w), int(h))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        t.hit(self.key, area)


class _PlaceKey:
    __slots__ = ("widget",)

    def __init__(self, widget: Widget):
        self.widget = widget


class KitWidget(Widget):
    """A widget the kit draws. Subclasses implement `component()` (a cell `Draw` built from their
    state, children's components inside it via `self.part(child)`), and call `self.kit_refresh()`
    when their state changes (reactives declared with `kit_reactive` do it themselves)."""

    DEFAULT_CSS = """
    KitWidget {
        width: auto;
        height: auto;
    }
    """

    #: Pin the hover-cursor look (a specimen showing the state; the gallery's "Hover").
    pinned_cursor: bool = False
    _kit_refreshing: bool = False

    def __init__(self, *children: Widget, name: str | None = None, id: str | None = None,
                 classes: str | None = None, disabled: bool = False, focusable: bool | None = None):
        super().__init__(*children, name=name, id=id, classes=classes, disabled=disabled)
        if focusable is not None:
            self.can_focus = focusable
        self._place_key = _PlaceKey(self)
        self._frame: _Frame | None = None
        self._frame_size: Size | None = None
        self._kit_refreshing = False

    # The theme -----------------------------------------------------------------------------------

    @property
    def kit(self):
        """The app's kit controller (`omarchy_textual.follow`), made on first use."""
        from .app import kit_of

        return kit_of(self.app)

    @property
    def theme_(self):
        """The kit `Theme` (tokens in effect, what is on screen, the revision)."""
        return self.kit.theme

    # What subclasses implement -------------------------------------------------------------------

    def component(self) -> Draw:
        raise NotImplementedError

    @property
    def hot(self) -> bool:
        """The hover-cursor look: focus, the pointer, or pinned (keyboard first: they look alike)."""
        return self.pinned_cursor or self.has_focus or (self.mouse_hover and not self.disabled)

    def part(self, child: Widget) -> Draw:
        """`child`'s component, placed: what a container puts in its layout for a child widget."""
        if isinstance(child, KitWidget):
            return Placed(child, child.component())
        return StockSlot(child)

    def parts(self, children: Iterable[Widget] | None = None) -> list[Draw]:
        return [self.part(c) for c in (self.children if children is None else children) if c.display]

    # The tree ------------------------------------------------------------------------------------

    @property
    def kit_root(self) -> "KitWidget":
        """The kit widget that draws this one: the outermost kit ancestor in an unbroken chain."""
        node: KitWidget = self
        while isinstance(node.parent, KitWidget):
            node = node.parent
        return node

    @property
    def is_root(self) -> bool:
        return not isinstance(self.parent, KitWidget)

    def refresh(self, *regions, repaint: bool = True, layout: bool = False, recompose: bool = False):
        """Any repaint of a kit widget (a reactive changed, focus, hover, the theme) draws its root
        again: the root's frame holds every cell of its tree."""
        if repaint and not self._kit_refreshing and self.is_attached:
            root = self.kit_root
            if root._kit_refreshing:
                return super().refresh(*regions, repaint=repaint, layout=layout, recompose=recompose)
            root._frame = None
            root._kit_refreshing = True
            try:
                if root is not self:
                    root.refresh(layout=layout)
                for w in root.walk_children(KitWidget):
                    if w is not self:
                        w._kit_refreshing = True
                        try:
                            w.refresh()
                        finally:
                            w._kit_refreshing = False
            finally:
                root._kit_refreshing = False
        return super().refresh(*regions, repaint=repaint, layout=layout, recompose=recompose)

    def kit_refresh(self, layout: bool = True) -> None:
        """This widget's state changed: draw its root again (and relayout, its size may change)."""
        self.refresh(layout=layout)

    # Sizing (as a root) --------------------------------------------------------------------------

    def get_content_width(self, container: Size, viewport: Size) -> int:
        return self.component().size(self.theme_)[0]

    def get_content_height(self, container: Size, viewport: Size, width: int) -> int:
        return self.component().size(self.theme_)[1]

    # Drawing -------------------------------------------------------------------------------------

    def _base_bg(self) -> Rgba | None:
        """What a root draws over: None for the page (the theme background), else the Textual
        background behind it."""
        base, _ = self.background_colors
        if base.a == 0:
            return None
        rgba = Rgba(base.r, base.g, base.b)
        if rgba == self.theme_.bg().opaque():
            return None
        return rgba

    def kit_frame(self) -> _Frame:
        """The root's composed buffer (drawn now if anything changed)."""
        root = self.kit_root
        if root is not self:
            return root.kit_frame()
        size = self.size
        if self._frame is not None and self._frame_size == size:
            return self._frame
        t = self.theme_
        w, h = size.width, size.height
        buf = Buffer(Rect(0, 0, w, h), bg=self._base_bg())
        saved = t.hits
        t.clear_hits()
        Placed(self, self.component()).draw(t, Rect(0, 0, w, h), buf)
        hits = t.hits
        t.hits = saved
        t.finish(buf)
        places = {}
        for k, r in hits:
            if isinstance(k, _PlaceKey):
                places[k.widget] = r
        frame = self._frame = _Frame(buf, hits, places)
        self._frame_size = size
        self._kit_refreshing = True
        try:
            self._place(places)
        finally:
            self._kit_refreshing = False
        self._frame = frame
        return frame

    def _place(self, places: dict) -> None:
        """Put every descendant where its component was drawn (absolute, relative to its parent)."""
        for w in self.walk_children(Widget, with_self=False):
            parent = w.parent
            # Only widgets drawn by this root (stock widgets inside kit containers included).
            if not isinstance(parent, KitWidget):
                continue
            r = places.get(w)
            st = w.styles
            if r is None:
                if w.display and isinstance(w, KitWidget):
                    st.visibility = "hidden"
                continue
            pr = places.get(parent, Rect(0, 0, 0, 0)) if parent is not self else Rect(0, 0, 0, 0)
            ox, oy = r.x - pr.x, r.y - pr.y
            if st.position != "absolute":
                st.position = "absolute"
            if (st.offset.x.value, st.offset.y.value) != (ox, oy):
                st.offset = (ox, oy)
            if st.width is None or not st.width.is_cells or st.width.value != r.width:
                st.width = r.width
            if st.height is None or not st.height.is_cells or st.height.value != r.height:
                st.height = r.height
            if st.visibility != "visible":
                st.visibility = "visible"

    def _my_rect(self, frame: _Frame) -> Rect | None:
        if self.is_root:
            return frame.buf.area
        return frame.places.get(self)

    def render_lines(self, crop: Region) -> list[Strip]:
        frame = self.kit_frame()
        r = self._my_rect(frame)
        blank = Strip.blank(crop.width)
        if r is None:
            return [blank] * crop.height
        out = []
        buf = frame.buf
        for y in range(crop.y, crop.y + crop.height):
            by = r.y + y
            if not (0 <= by < buf.area.height) or y >= r.height:
                out.append(blank)
                continue
            x0 = r.x + crop.x
            x1 = min(r.x + min(crop.x + crop.width, r.width), buf.area.width)
            s = strip_of(buf, by, x0, x1) if x1 > x0 else Strip([], 0)
            if s.cell_length < crop.width:
                s = s.extend_cell_length(crop.width)
            out.append(s)
        return out

    # Hits ----------------------------------------------------------------------------------------

    def root_offset(self, event: events.MouseEvent):
        """The pointer's cell in the root's frame (None outside it)."""
        root = self.kit_root
        r = root.content_region
        x, y = event.screen_x - r.x, event.screen_y - r.y
        if not (0 <= x < r.width and 0 <= y < r.height):
            return None
        from textual.geometry import Offset

        return Offset(x, y)

    def hit_at_event(self, event: events.MouseEvent) -> object | None:
        """The topmost part drawn under the pointer (in the root's frame), or None."""
        root = self.kit_root
        frame = root.kit_frame()
        off = self.root_offset(event)
        if off is None:
            return None
        for k, r in reversed(frame.hits):
            if isinstance(k, _PlaceKey):
                continue
            if r.contains(off.x, off.y):
                return k
        return None

    def part_at(self, event: events.MouseEvent) -> object | None:
        """The part of this widget under the pointer: the `x` of a hit keyed `(self, x)`, `True`
        for the widget's own key, else None."""
        root = self.kit_root
        frame = root.kit_frame()
        off = self.root_offset(event)
        if off is None:
            return None
        for k, r in reversed(frame.hits):
            if not r.contains(off.x, off.y):
                continue
            if k is self:
                return True
            if isinstance(k, tuple) and len(k) == 2 and k[0] is self:
                return k[1]
        return None

    def rect_of(self, key: object) -> Rect | None:
        """Where `key` was drawn in the root's frame."""
        for k, r in reversed(self.kit_root.kit_frame().hits):
            if k == key:
                return r
        return None

    def local_x(self, event: events.MouseEvent) -> int | None:
        """The pointer's column in the root's frame."""
        off = self.root_offset(event)
        return None if off is None else off.x

    # Focus and hover redraw ----------------------------------------------------------------------

    def _on_focus(self, event: events.Focus) -> None:
        self.kit_refresh(layout=False)

    def _on_blur(self, event: events.Blur) -> None:
        self.kit_refresh(layout=False)

    def watch_mouse_hover(self, value: bool) -> None:
        self.kit_refresh(layout=False)

    def on_resize(self, event: events.Resize) -> None:
        if self.is_root:
            self._frame = None


def kit_reactive(default, layout: bool = True, **kw):
    """A reactive whose change draws the widget's root again (and relayouts it: a kit widget's
    size follows its state)."""
    from textual.reactive import reactive

    return reactive(default, layout=layout, **kw)
