"""SearchList as a Textual widget (spec/terminal.md, Composed): the type-ahead list. It holds every
item and shows those whose title matches the query (`SearchList.matches`, case-insensitive);
typing edits the query (`value`), Up/Down/Page Up/Down/Home/End move the cursor, Return activates, Escape
clears the query (then posts `Closed`), the pointer moves the cursor when it reaches another cell,
a click activates a row.

    yield SearchList(items, query="term", detail=True, width=45, height=12)

    def on_search_list_activated(self, event: SearchList.Activated) -> None:
        event.item  # the SearchItem chosen
"""

from __future__ import annotations

from typing import Iterable

from textual import events
from textual.message import Message

from .. import cells as C
from ..cells.base import Draw
from ..cells.lists import CursorModel, ScrollArea as _ScrollAreaCells
from ..cells.search import SearchItem, SearchKey
from ..widget import KitWidget, kit_reactive

__all__ = ["SearchItem", "SearchList"]


class SearchList(KitWidget, can_focus=True):
    """The query line over the matching items as ListRows (with `detail`, a pane of the cursor
    item's detail beside them); no matches: an overlay EmptyState. `width` × `height` cells."""

    class Changed(Message):
        """The query changed (`value`; the widget's `matches` are the new results)."""

        def __init__(self, search: "SearchList", value: str) -> None:
            super().__init__()
            self.search = search
            self.value = value

        @property
        def control(self) -> "SearchList":
            return self.search

    class Activated(Message):
        """An item was chosen: `index` among the matches, `item`."""

        def __init__(self, search: "SearchList", index: int, item: SearchItem) -> None:
            super().__init__()
            self.search = search
            self.index = index
            self.item = item

        @property
        def control(self) -> "SearchList":
            return self.search

    class Deleted(Activated):
        pass

    class Closed(Message):
        """Escape on an empty query."""

        def __init__(self, search: "SearchList") -> None:
            super().__init__()
            self.search = search

        @property
        def control(self) -> "SearchList":
            return self.search

    #: The query.
    value = kit_reactive("")
    #: The first content row shown.
    top_row = kit_reactive(0)

    def __init__(self, items: Iterable[SearchItem] = (), *, query: str = "", detail: bool = False,
                 cursor: int | None = 0, placeholder: str | None = None, empty: tuple[str, str] | None = None,
                 width: int = 60, height: int = 22, **kw):
        super().__init__(**kw)
        self.all_items = list(items)
        self.detail, self.placeholder, self.empty = detail, placeholder, empty
        self.width, self.height = width, height
        self.set_reactive(SearchList.value, query)
        self.model = CursorModel.list(len(self.matches))
        if cursor is not None:
            self.model.set("", cursor)

    @property
    def matches(self) -> list[SearchItem]:
        return [i for i in self.all_items if C.SearchList.matches(i.title, self.value)]

    @property
    def cursor_index(self) -> int | None:
        return self.model.index() if self.model.active else None

    def component(self) -> Draw:
        return C.SearchList(self, self.value, self.matches, cursor=self.cursor_index, offset=self.top_row,
                            detail=self.detail, placeholder=self.placeholder, empty=self.empty,
                            width=self.width, height=self.height)

    def _body_rows(self) -> int:
        return max(0, self.height - 6)

    def _reveal(self) -> None:
        if self.model.active:
            self.top_row = _ScrollAreaCells.ensure_visible(self.top_row, self.model.index(), 1, self._body_rows())

    def on_key(self, event: events.Key) -> None:
        what, query = C.SearchList.handle_key(self.value, self.model, event.key, event.character)
        if what is SearchKey.IGNORED:
            return
        event.stop()
        event.prevent_default()
        if what is SearchKey.QUERY:
            self.value = query
            self.model.set_count("", len(self.matches))
            self.top_row = 0
            self.post_message(self.Changed(self, query))
        elif what is SearchKey.MOVED:
            self._reveal()
            self.refresh()
        elif what in (SearchKey.ACTIVATE, SearchKey.DELETE):
            items = self.matches
            i = self.model.index()
            if i < len(items):
                kind = self.Activated if what is SearchKey.ACTIVATE else self.Deleted
                self.post_message(kind(self, i, items[i]))
        elif what is SearchKey.CLOSE:
            self.post_message(self.Closed(self))

    def _row_at(self, event: events.MouseEvent) -> int | None:
        p = self.part_at(event)
        if isinstance(p, tuple) and len(p) == 2 and p[0] == "row":
            return p[1]
        return None

    def on_click(self, event: events.Click) -> None:
        i = self._row_at(event)
        if i is not None:
            event.stop()
            items = self.matches
            if i < len(items):
                self.post_message(self.Activated(self, i, items[i]))

    def on_mouse_move(self, event: events.MouseMove) -> None:
        i = self._row_at(event)
        if i is not None and self.model.pointer_moved(event.screen_x, event.screen_y, "", i):
            self.refresh()
