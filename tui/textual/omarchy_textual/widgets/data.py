"""Tables, trees, split panes, sidebars and the month grid as Textual widgets (spec/terminal.md,
Data). Each is focusable, takes its keys (j/k and the arrows, Home/End, Page Up/Down, Enter or
Space), takes clicks on its rows, and posts what changed (`Table.Selected`, `Tree.Toggled`, ...)."""

from __future__ import annotations

from typing import Iterable, Sequence

from textual import events
from textual.app import ComposeResult
from textual.message import Message
from textual.widget import Widget

from ..cells import data as D
from ..cells import layout as L
from ..cells.base import Draw
from ..cells.data import Column, Date, SidebarItem, SidebarSection, TreeNode
from ..widget import KitWidget, kit_reactive

__all__ = [
    "Calendar",
    "Column",
    "Date",
    "Sidebar",
    "SidebarItem",
    "SidebarSection",
    "SplitHandle",
    "SplitView",
    "Table",
    "Tree",
    "TreeNode",
]


def _activates(event: events.Key) -> bool:
    return event.key in ("enter", "space")


class _Changed(Message):
    """A data widget's message: `control` is the widget."""

    def __init__(self, widget: Widget) -> None:
        super().__init__()
        self.widget = widget

    @property
    def control(self) -> Widget:
        return self.widget


# Table -----------------------------------------------------------------------------------------


class Table(KitWidget, can_focus=True):
    """Columns (`Column(title, width=None, right=False)`) over rows of strings: a header (the
    sorted column's chevron), a rule, a row per item. The cursor row shows the hover fill, the
    selected row the selected fill. j/k, the arrows, Home/End and Page Up/Down move the cursor
    (`Table.Highlighted`); Enter, Space or a click select (`Table.Selected`)."""

    class Highlighted(_Changed):
        def __init__(self, table: "Table", row: int) -> None:
            super().__init__(table)
            self.row = row

    class Selected(_Changed):
        def __init__(self, table: "Table", row: int) -> None:
            super().__init__(table)
            self.row = row

    cursor = kit_reactive(None)
    selected = kit_reactive(None)
    sort = kit_reactive(None)

    def __init__(self, columns: Iterable[Column], rows: Iterable[Sequence[str]] = (), *, sort: tuple[int, bool] | None = None,
                 selected: int | None = None, cursor: int | None = None, width: int = 60, **kw):
        super().__init__(**kw)
        self.columns = list(columns)
        self.rows = [list(r) for r in rows]
        self.width = width
        self.set_reactive(Table.sort, sort)
        self.set_reactive(Table.selected, selected)
        self.set_reactive(Table.cursor, cursor)

    def component(self) -> Draw:
        return D.Table(self.columns, self.rows, sort=self.sort, selected=self.selected, cursor=self.cursor,
                       width=self.width, key=self)

    def move_cursor(self, row: int) -> None:
        if row != self.cursor:
            self.cursor = row
            self.post_message(self.Highlighted(self, row))

    def select(self, row: int) -> None:
        self.selected = row
        self.post_message(self.Selected(self, row))

    def on_key(self, event: events.Key) -> None:
        if _activates(event):
            event.stop()
            if self.cursor is not None:
                self.select(self.cursor)
            return
        n = D.Table.handle_key(self.cursor or 0, len(self.rows), event.key)
        if n is not None:
            event.stop()
            self.move_cursor(n)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.move_cursor(p)
            self.select(p)


# Tree ------------------------------------------------------------------------------------------


class Tree(KitWidget, can_focus=True):
    """A tree of `TreeNode`s: branches open with Right/l (or a click on the chevron) and fold with
    Left/h (`Tree.Toggled`); j/k and the arrows move the cursor; Enter, Space or a click on a row
    select (`Tree.Selected`)."""

    class Toggled(_Changed):
        def __init__(self, tree: "Tree", id: str, expanded: bool) -> None:
            super().__init__(tree)
            self.id = id
            self.expanded = expanded

    class Selected(_Changed):
        def __init__(self, tree: "Tree", id: str) -> None:
            super().__init__(tree)
            self.id = id

    expanded = kit_reactive(())
    selected = kit_reactive(None)
    cursor = kit_reactive(None)

    def __init__(self, nodes: Iterable[TreeNode], *, expanded: Iterable[str] = (), selected: str | None = None,
                 cursor: str | None = None, width: int = 28, **kw):
        super().__init__(**kw)
        self.nodes = list(nodes)
        self.width = width
        self.set_reactive(Tree.expanded, tuple(expanded))
        self.set_reactive(Tree.selected, selected)
        self.set_reactive(Tree.cursor, cursor)

    def component(self) -> Draw:
        return D.Tree(self.nodes, self.expanded, selected=self.selected, cursor=self.cursor, width=self.width, key=self)

    def visible(self) -> list[tuple[int, TreeNode]]:
        return D.Tree.visible(self.nodes, self.expanded)

    def toggle(self, id: str) -> None:
        is_open = id in self.expanded
        self.expanded = tuple(x for x in self.expanded if x != id) if is_open else (*self.expanded, id)
        self.post_message(self.Toggled(self, id, not is_open))

    def select(self, id: str) -> None:
        self.selected = id
        self.post_message(self.Selected(self, id))

    def on_key(self, event: events.Key) -> None:
        rows = self.visible()
        if not rows:
            return
        ids = [n.id for _, n in rows]
        i = ids.index(self.cursor) if self.cursor in ids else 0
        node = rows[i][1]
        is_open = node.id in self.expanded
        k = event.key
        if k in ("down", "j"):
            self.cursor = ids[min(i + 1, len(ids) - 1)]
        elif k in ("up", "k"):
            self.cursor = ids[max(0, i - 1)]
        elif k in ("right", "l") and node.children and not is_open:
            self.toggle(node.id)
        elif k in ("left", "h") and is_open:
            self.toggle(node.id)
        elif _activates(event):
            self.select(node.id)
        else:
            return
        event.stop()

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, tuple) and p[0] == "toggle":
            event.stop()
            self.toggle(p[1])
        elif isinstance(p, str):
            event.stop()
            self.cursor = p
            self.select(p)


# SplitView -------------------------------------------------------------------------------------


class SplitHandle(KitWidget, can_focus=True):
    """The handle between two panes of a `SplitView` (drawn by it): focus it and h/l (or the
    arrows) resize the pane before it; drag it with the mouse."""

    def __init__(self, index: int, **kw):
        super().__init__(**kw)
        self.index = index
        self._dragging = False

    @property
    def view(self) -> "SplitView":
        return self.parent  # type: ignore[return-value]

    def component(self) -> Draw:
        return L.Blank(1, 1)

    def on_key(self, event: events.Key) -> None:
        if event.key in ("left", "h"):
            self.view.resize(self.index, -1)
        elif event.key in ("right", "l"):
            self.view.resize(self.index, 1)
        else:
            return
        event.stop()

    def on_mouse_down(self, event: events.MouseDown) -> None:
        self._dragging = True
        self.capture_mouse()

    def on_mouse_up(self, event: events.MouseUp) -> None:
        if self._dragging:
            self._dragging = False
            self.release_mouse()

    def on_mouse_move(self, event: events.MouseMove) -> None:
        if self._dragging:
            self.view.drag_to(self.index, event.screen_x)


class SplitView(KitWidget):
    """Its child widgets side by side, `width` × `height`, a one-column handle between each pair
    (a focusable `SplitHandle`). `sizes[i]` fixes pane i's width (None: it shares the rest),
    `mins`/`maxes` bound what the handles may do. `handle_ids` names the handles."""

    class Resized(_Changed):
        def __init__(self, view: "SplitView", sizes: tuple) -> None:
            super().__init__(view)
            self.sizes = sizes

    sizes = kit_reactive(())

    def __init__(self, *panes: Widget, width: int, height: int, sizes: Sequence[int | None] = (),
                 mins: Sequence[int] = (), maxes: Sequence[int | None] = (), handle_ids: Sequence[str] = (), **kw):
        super().__init__(*panes, **kw)
        self.box = (width, height)
        self.set_reactive(SplitView.sizes, tuple(sizes))
        self.mins = tuple(mins)
        self.maxes = tuple(maxes)
        self.handle_ids = tuple(handle_ids)

    def compose(self) -> ComposeResult:
        n = len(self._pending_children)
        for i in range(max(0, n - 1)):
            yield SplitHandle(i, id=self.handle_ids[i] if i < len(self.handle_ids) else None)

    @property
    def panes(self) -> list[Widget]:
        return [c for c in self.children if not isinstance(c, SplitHandle)]

    @property
    def handles(self) -> list[SplitHandle]:
        return [c for c in self.children if isinstance(c, SplitHandle)]

    def _get(self, seq: tuple, i: int, default):
        return seq[i] if i < len(seq) else default

    def component(self) -> Draw:
        panes = [
            D.SplitPane(self.part(p), size=self._get(self.sizes, i, None), min=self._get(self.mins, i, 0), visible=bool(p.display))
            for i, p in enumerate(self.panes)
        ]
        handles = self.handles
        cursor = next((h.index for h in handles if h.hot or h._dragging), None)
        return D.SplitView(*self.box, panes, cursor=cursor, key=self, handles=[self.part(h) for h in handles])

    def resize(self, i: int, delta: int) -> None:
        cur = self._get(self.sizes, i, None)
        if cur is None:
            return
        self._set_size(i, cur + delta)

    def _set_size(self, i: int, size: int) -> None:
        lo = self._get(self.mins, i, 0)
        hi = self._get(self.maxes, i, None)
        size = max(lo, size)
        if hi is not None:
            size = min(hi, size)
        sizes = list(self.sizes) + [None] * max(0, i + 1 - len(self.sizes))
        if sizes[i] == size:
            return
        sizes[i] = size
        self.sizes = tuple(sizes)
        self.post_message(self.Resized(self, self.sizes))

    def drag_to(self, i: int, screen_x: int) -> None:
        """Handle `i` follows the pointer: the pane before it ends at the pointer's column."""
        if self._get(self.sizes, i, None) is None:
            return
        x = screen_x - self.content_region.x
        before = sum((self._get(self.sizes, j, 0) or 0) + 1 for j in range(i))
        self._set_size(i, x - before)


# Sidebar ---------------------------------------------------------------------------------------


class Sidebar(KitWidget, can_focus=True):
    """Sections of rows (`SidebarSection(title, [SidebarItem(id, label, glyph)])`), one selected:
    j/k or the arrows move the selection, a click picks a row (`Sidebar.Selected`). With focus
    the selected row shows the cursor's look. `collapsed` is the 5-column rail of glyphs."""

    class Selected(_Changed):
        def __init__(self, sidebar: "Sidebar", id: str) -> None:
            super().__init__(sidebar)
            self.id = id

    selected = kit_reactive(None)
    collapsed = kit_reactive(False)

    def __init__(self, sections: Iterable[SidebarSection], *, selected: str | None = None, collapsed: bool = False,
                 width: int = 25, **kw):
        super().__init__(**kw)
        self.sections = list(sections)
        self.width = width
        self.set_reactive(Sidebar.selected, selected)
        self.set_reactive(Sidebar.collapsed, collapsed)

    def component(self) -> Draw:
        return D.Sidebar(self.sections, selected=self.selected, cursor=self.selected if self.has_focus else None,
                         collapsed=self.collapsed, width=self.width, key=self)

    def select(self, id: str) -> None:
        if id != self.selected:
            self.selected = id
            self.post_message(self.Selected(self, id))

    def on_key(self, event: events.Key) -> None:
        ids = [it.id for s in self.sections for it in s.items]
        if not ids:
            return
        i = ids.index(self.selected) if self.selected in ids else 0
        if event.key in ("down", "j"):
            self.select(ids[min(i + 1, len(ids) - 1)])
        elif event.key in ("up", "k"):
            self.select(ids[max(0, i - 1)])
        else:
            return
        event.stop()

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, str):
            event.stop()
            self.select(p)


# Calendar --------------------------------------------------------------------------------------


class Calendar(KitWidget, can_focus=True):
    """A month (`year`, `month`) from Monday: `today` marked, the `selected` day filled. The arrows
    and h/j/k/l move the cursor a day or a week, Page Up/Down a month, the month following it;
    Enter, Space or a click select a day (`Calendar.Selected`); the chevrons step the month."""

    class Selected(_Changed):
        def __init__(self, calendar: "Calendar", date: Date) -> None:
            super().__init__(calendar)
            self.date = date

    year = kit_reactive(2026)
    month = kit_reactive(1)
    selected = kit_reactive(None)
    cursor = kit_reactive(None)

    def __init__(self, year: int, month: int, *, today: Date | None = None, selected: Date | None = None,
                 cursor: Date | None = None, **kw):
        super().__init__(**kw)
        self.today = today
        self.set_reactive(Calendar.year, year)
        self.set_reactive(Calendar.month, month)
        self.set_reactive(Calendar.selected, selected)
        self.set_reactive(Calendar.cursor, cursor)

    def component(self) -> Draw:
        return D.Calendar(self.year, self.month, today=self.today, selected=self.selected, cursor=self.cursor, key=self)

    def show(self, year: int, month: int) -> None:
        self.year, self.month = year, month

    def step_month(self, n: int) -> None:
        d = Date(self.year, self.month, 1).add_months(n)
        self.show(d.year, d.month)

    def select(self, d: Date) -> None:
        self.selected = d
        self.post_message(self.Selected(self, d))

    def on_key(self, event: events.Key) -> None:
        c = self.cursor or self.selected or Date(self.year, self.month, 1)
        if _activates(event):
            event.stop()
            self.select(c)
            return
        n = D.Calendar.handle_key(c, event.key)
        if n is not None:
            event.stop()
            self.cursor = n
            self.show(n.year, n.month)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if p in ("prev", "next"):
            event.stop()
            self.step_month(-1 if p == "prev" else 1)
        elif isinstance(p, Date):
            event.stop()
            self.select(p)
            self.show(p.year, p.month)
