"""SearchList: the type-ahead overlay (spec/terminal.md, Composed). A port of `omarchy-ratatui`'s
`ui/search.rs`, cell for cell."""

from __future__ import annotations

import enum
from typing import Iterable

from .base import Draw
from .buffer import Buffer, Rect
from .controls import wrap
from .feedback import EmptyState
from .lists import CursorModel, ListRow, ScrollArea, printable
from .paint import Ink, clear, frame, inset, text_fit, vline

__all__ = ["SearchItem", "SearchKey", "SearchList"]


class SearchItem:
    """One result: title, subtitle, glyph, value, detail."""

    def __init__(self, title: str, subtitle: str = "", glyph: str = "", value: str = "", detail: str = ""):
        self.title, self.subtitle, self.glyph, self.value, self.detail = title, subtitle, glyph, value, detail

    def __repr__(self) -> str:
        return f"SearchItem({self.title!r})"


class SearchKey(enum.Enum):
    """What a key did to a SearchList (`ACTIVATE`/`DELETE` act on the cursor's index)."""

    QUERY = "query"
    MOVED = "moved"
    ACTIVATE = "activate"
    DELETE = "delete"
    CLOSE = "close"
    IGNORED = "ignored"


class SearchList(Draw):
    """On the `menu` surface: a box in its border color, padded 2 × 1; the filter line (the query,
    or the placeholder in `textDim`), a blank row, then the body: ListRows under the cursor and, with
    the detail pane, two columns, a `│` rule in `track`, three columns, and the pane (the cursor
    item's title bold over its detail in `textDim`, wrapped). No items: an overlay EmptyState
    centered in the body. The filter line keyed `(key, "filter")`, rows `(key, ("row", i))`."""

    def __init__(self, key: object, query: str = "", items: Iterable[SearchItem] = (), cursor: int | None = 0,
                 offset: int = 0, detail: bool = False, placeholder: str | None = None,
                 empty: tuple[str, str] | None = None, width: int = 60, height: int = 22):
        self.key, self.query, self.items, self.cursor = key, query, list(items), cursor
        self.offset, self.detail, self.placeholder, self.empty = offset, detail, placeholder, empty
        self.width, self.height = width, height

    @staticmethod
    def matches(text: str, query: str) -> bool:
        """The plain case-insensitive substring test."""
        return query.lower() in text.lower()

    @staticmethod
    def handle_key(query: str, cursor: CursorModel, key: str, character: str | None = None) -> tuple[SearchKey, str]:
        """Keys, the filter line holding focus: printable keys and Space type, Backspace edits, Up,
        Down, Page Up/Down, Home/End move the cursor, Return activates, Delete asks to delete,
        Escape clears the query or, when empty, asks to close. Returns (what, the new query)."""
        s = printable(key, character)
        if s is not None:
            cursor.set("", 0)
            return SearchKey.QUERY, query + s
        if key == "space":
            cursor.set("", 0)
            return SearchKey.QUERY, query + " "
        if key == "backspace":
            cursor.set("", 0)
            return SearchKey.QUERY, query[:-1]
        if key == "escape":
            if query:
                cursor.set("", 0)
                return SearchKey.QUERY, ""
            return SearchKey.CLOSE, query
        if key in ("up", "down", "pageup", "pagedown", "home", "end"):
            cursor.active = True
            cursor.apply_key(key)
            return SearchKey.MOVED, query
        if key == "enter":
            return SearchKey.ACTIVATE, query
        if key == "delete":
            return SearchKey.DELETE, query
        return SearchKey.IGNORED, query

    def size(self, t) -> tuple[int, int]:
        return (self.width, self.height)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        clear(buf, area)
        m = t.shown.surfaces.menu
        inner = inset(frame(t, buf, area, m.border.color), 2, 1)
        if not self.query:
            p = self.placeholder if self.placeholder is not None else t.string("common.search", "Search...")
            text_fit(t, buf, inner.x, inner.y, p, inner.width, t.dim())
        else:
            text_fit(t, buf, inner.x, inner.y, self.query, inner.width, m.text)
        t.hit((self.key, "filter"), Rect(inner.x, inner.y, inner.width, 1))
        body = Rect(inner.x, inner.y + 2, inner.width, max(0, inner.height - 2))
        if not self.items:
            if not self.query:
                glyph, txt = self.empty if self.empty is not None else ("\U000f0349", "")
            else:
                glyph = "\U000f0349"
                txt = t.string("common.noMatchesFor", "No matches for “%1”").replace("%1", self.query)
            e = EmptyState(txt, glyph=glyph, overlay=True, width=body.width)
            _, eh = e.size(t)
            e.draw(t, Rect(body.x, body.y + max(0, body.height - eh) // 2, body.width, eh), buf)
            return
        list_w = body.width * 55 // 100 if self.detail else body.width
        lst = ScrollArea(list_w, max_rows=body.height, offset=self.offset)
        for i, it in enumerate(self.items):
            lst.child(ListRow(it.title, glyph=it.glyph, subtitle=it.subtitle, value=it.value,
                              cursor=self.cursor == i, width=list_w, key=(self.key, ("row", i))))
        _, lh = lst.size(t)
        lst.draw(t, Rect(body.x, body.y, list_w, lh), buf)
        if self.detail:
            rx = body.x + list_w + 2
            vline(t, buf, rx, body.y, body.height, "│", t.track())
            px = rx + 3
            pw = max(0, body.right - px)
            if self.cursor is not None and 0 <= self.cursor < len(self.items):
                it = self.items[self.cursor]
                text_fit(t, buf, px, body.y, it.title, pw, Ink(m.text, bold=True))
                for n, line in enumerate(wrap(it.detail, pw)):
                    y = body.y + 1 + n
                    if y >= body.bottom:
                        break
                    text_fit(t, buf, px, y, line, pw, t.dim())
