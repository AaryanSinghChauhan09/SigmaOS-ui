"""Rows and the pieces lists are made of, as Textual widgets (spec/terminal.md, Lists): `ListRow`,
`ScrollArea` (a list with the keyboard cursor, `CursorModel`, and the overlaid bar), `KeyValue`,
`Hero`, `Badge`, `StatusDot`, `Avatar`, `TileGrid`.

    with ScrollArea(width=53, max_rows=7, cursor=0):
        yield ListRow("Basecamp", glyph="\\U000f0928", status=("Connected", "success"), current=True)
        yield ListRow("Messages", leading=Avatar("Messages", size="small"), trailing=Badge("3", "accent", filled=True))

`ScrollArea.Activated` is posted when a row is chosen (Return, Space or a click),
`ListRow.ActionPressed` when a row's action is clicked.
"""

from __future__ import annotations

from pathlib import Path
from typing import Iterable

from textual import events
from textual.message import Message
from textual.widget import Widget

from .. import cells as C
from ..cells.base import Draw, Tone
from ..cells.lists import AvatarImage, AvatarSize, CursorKey, CursorModel, Tile
from ..color import Rgba
from ..widget import KitWidget, kit_reactive

__all__ = [
    "Avatar",
    "AvatarImage",
    "AvatarSize",
    "Badge",
    "CursorModel",
    "Hero",
    "KeyValue",
    "ListRow",
    "ScrollArea",
    "StatusDot",
    "Tile",
    "TileGrid",
]


def _tone(v):
    return None if v is None else (v[0], Tone.of(v[1]))


class ListRow(KitWidget):
    """One row of a list: `[glyph or leading]Title  subtitle/status ··· value [trailing] [end]`.
    `leading`, `trailing` and `expansion` are widgets (a small Avatar, a Badge, a bare
    ToggleSwitch; any widget under the main line). The `cursor` row takes the hover fill, the
    `current` one the selected fill; `disabled` rows are faint and take no clicks. Inside a
    `ScrollArea` the area's cursor drives `cursor`; alone, the pointer does."""

    class Selected(Message):
        """The row was clicked."""

        def __init__(self, row: "ListRow") -> None:
            super().__init__()
            self.row = row

        @property
        def control(self) -> "ListRow":
            return self.row

    class ActionPressed(Message):
        """The row's action (its end slot under the cursor) was clicked."""

        def __init__(self, row: "ListRow") -> None:
            super().__init__()
            self.row = row

        @property
        def control(self) -> "ListRow":
            return self.row

    title = kit_reactive("")
    subtitle = kit_reactive("")
    value = kit_reactive("")
    cursor = kit_reactive(False)
    current = kit_reactive(False)
    action_cursor = kit_reactive(False)

    def __init__(self, title: str, *, glyph: str = "", glyph_color: Rgba | None = None,
                 leading: Widget | None = None, subtitle: str = "", status: tuple[str, Tone | str] | None = None,
                 value: str = "", trailing: Widget | None = None,
                 trailing_glyph: tuple[str, Tone | str] | None = None, action: tuple[str, bool] | None = None,
                 action_cursor: bool = False, expansion: Widget | None = None, cursor: bool = False,
                 current: bool = False, width: int = 53, **kw):
        children = [w for w in (leading, trailing, expansion) if w is not None]
        super().__init__(*children, **kw)
        self.set_reactive(ListRow.title, title)
        self.set_reactive(ListRow.subtitle, subtitle)
        self.set_reactive(ListRow.value, value)
        self.set_reactive(ListRow.cursor, cursor)
        self.set_reactive(ListRow.current, current)
        self.set_reactive(ListRow.action_cursor, action_cursor)
        self.glyph, self.glyph_color = glyph, glyph_color
        self.leading, self.trailing, self.expansion = leading, trailing, expansion
        self.status = _tone(status)
        self.trailing_glyph = _tone(trailing_glyph)
        self.action = action
        self.width = width

    @property
    def in_list(self) -> bool:
        return isinstance(self.parent, ScrollArea)

    @property
    def hot(self) -> bool:
        if self.in_list:
            return self.cursor
        return self.cursor or (self.mouse_hover and not self.disabled)

    def component(self) -> Draw:
        slot = lambda w: self.part(w) if w is not None else None  # noqa: E731
        return C.ListRow(self.title, glyph=self.glyph, glyph_color=self.glyph_color, leading=slot(self.leading),
                         subtitle=self.subtitle, status=self.status, value=self.value, trailing=slot(self.trailing),
                         trailing_glyph=self.trailing_glyph, action=self.action, action_cursor=self.action_cursor,
                         expansion=slot(self.expansion), cursor=self.hot, current=self.current,
                         disabled=self.disabled, width=self.width, key=self)

    def on_click(self, event: events.Click) -> None:
        if self.disabled:
            return
        event.stop()
        r = self.rect_of((self, "action"))
        off = self.root_offset(event)
        if r is not None and off is not None and r.contains(off.x, off.y):
            self.post_message(self.ActionPressed(self))
        else:
            self.post_message(self.Selected(self))

    def on_mouse_move(self, event: events.MouseMove) -> None:
        if self.in_list and not self.disabled:
            self.parent.pointer_over(self, event)


class ScrollArea(KitWidget):
    """A vertical scroll view of its children, capped at `max_rows`, the bar over its last column
    while the content overflows. Over `ListRow` children it is a list: it keeps the keyboard cursor
    (`CursorModel`: j/k and the arrows, Page Up/Down, Home/End), scrolls to keep it in view,
    follows the pointer when it reaches another cell, and posts `Activated` on Return, Space or a
    click (`Deleted` on Delete or x)."""

    class Activated(Message):
        def __init__(self, area: "ScrollArea", index: int, row: Widget) -> None:
            super().__init__()
            self.area = area
            self.index = index
            self.row = row

        @property
        def control(self) -> "ScrollArea":
            return self.area

    class Deleted(Activated):
        pass

    #: The first content row shown.
    top_row = kit_reactive(0)

    def __init__(self, *children: Widget, width: int = 53, max_rows: int = 65535, gap: int = 0,
                 cursor: int | None = None, **kw):
        self._focusable = kw.get("focusable")
        super().__init__(*children, **kw)
        self.width, self.max_rows, self.gap = width, max_rows, gap
        self.model = CursorModel.list(0)
        self._initial_cursor = cursor

    def on_mount(self) -> None:
        self.model.set_count("", len(self.rows))
        if self._initial_cursor is not None:
            self.model.set("", self._initial_cursor)
        if self._focusable is None:
            self.can_focus = bool(self.rows)
        self._sync()

    @property
    def rows(self) -> list[ListRow]:
        return [c for c in self.children if isinstance(c, ListRow)]

    def component(self) -> Draw:
        return C.ScrollArea(self.width, *self.parts(), max_rows=self.max_rows, offset=self.top_row, gap=self.gap)

    def _sync(self) -> None:
        for i, r in enumerate(self.rows):
            r.cursor = self.model.is_at("", i)

    def _reveal(self) -> None:
        t = self.theme_
        top = 0
        rows = self.rows
        i = self.model.index()
        for c in self.children[: self.children.index(rows[i])] if rows else []:
            top += c.component().size(t)[1] + self.gap
        h = rows[i].component().size(t)[1] if rows else 1
        view = min(self.max_rows, self.component().size(t)[1])
        self.top_row = C.ScrollArea.ensure_visible(self.top_row, top, h, view)

    @property
    def cursor_index(self) -> int | None:
        return self.model.cursor_in("")

    def move_to(self, index: int) -> None:
        """Put the cursor on row `index` (and scroll it into view)."""
        self.model.set("", index)
        self._sync()
        self._reveal()

    def on_key(self, event: events.Key) -> None:
        rows = self.rows
        if not rows or event.key in ("tab", "shift+tab"):
            # One section: Tab walks the app's focus, not the list.
            return
        r = self.model.apply_key(event.key, event.character)
        if r is CursorKey.MOVED:
            event.stop()
            self._sync()
            self._reveal()
        elif r in (CursorKey.ACTIVATE, CursorKey.DELETE):
            event.stop()
            i = self.model.index()
            if not self.model.active:
                return
            kind = self.Activated if r is CursorKey.ACTIVATE else self.Deleted
            self.post_message(kind(self, i, rows[i]))

    def on_list_row_selected(self, event: ListRow.Selected) -> None:
        rows = self.rows
        if event.row in rows:
            event.stop()
            i = rows.index(event.row)
            self.move_to(i)
            self.post_message(self.Activated(self, i, event.row))

    def pointer_over(self, row: ListRow, event: events.MouseMove) -> None:
        rows = self.rows
        if row in rows and self.model.pointer_moved(event.screen_x, event.screen_y, "", rows.index(row)):
            self._sync()

    def on_mouse_scroll_down(self, event: events.MouseScrollDown) -> None:
        t = self.theme_
        c = C.ScrollArea(self.width, *self.parts(), max_rows=self.max_rows, gap=self.gap)
        self.top_row = min(self.top_row + 1, max(0, c.content_h(t) - c.size(t)[1]))
        event.stop()

    def on_mouse_scroll_up(self, event: events.MouseScrollUp) -> None:
        self.top_row = max(0, self.top_row - 1)
        event.stop()


class KeyValue(KitWidget):
    """Dim labels beside their values, `columns` pairs to a line. `pairs`: (label, value) or
    (label, value, tone)."""

    def __init__(self, pairs: Iterable[tuple] = (), *, columns: int = 1, **kw):
        super().__init__(**kw)
        self.pairs = list(pairs)
        self.columns = columns

    def component(self) -> Draw:
        return C.KeyValue(self.pairs, columns=self.columns)


class Hero(KitWidget):
    """A panel's head: the glyph, the bold title over the uppercased status (urgent on `error`),
    the `trailing` widget (a ToggleSwitch) at the end of the first row."""

    title = kit_reactive("")
    status = kit_reactive("")
    error = kit_reactive(False)

    def __init__(self, glyph: str, title: str, *, status: str = "", error: bool = False,
                 glyph_opacity: float = 1.0, trailing: Widget | None = None, width: int = 53, **kw):
        super().__init__(*([trailing] if trailing is not None else []), **kw)
        self.glyph, self.glyph_opacity, self.trailing, self.width = glyph, glyph_opacity, trailing, width
        self.set_reactive(Hero.title, title)
        self.set_reactive(Hero.status, status)
        self.set_reactive(Hero.error, error)

    def component(self) -> Draw:
        return C.Hero(self.glyph, self.title, status=self.status, error=self.error,
                      glyph_opacity=self.glyph_opacity,
                      trailing=self.part(self.trailing) if self.trailing is not None else None, width=self.width)


class Badge(KitWidget):
    """A count or short label, bold: neutral, toned (`tone`) or `filled`."""

    label = kit_reactive("")

    def __init__(self, label: str, tone: Tone | str = Tone.NEUTRAL, *, filled: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(Badge.label, label)
        self.tone, self.filled = Tone.of(tone), filled

    def component(self) -> Draw:
        return C.Badge(self.label, self.tone, filled=self.filled)


class StatusDot(KitWidget):
    """A dot `●` in its tone."""

    def __init__(self, tone: Tone | str = Tone.NEUTRAL, **kw):
        super().__init__(**kw)
        self.tone = Tone.of(tone)

    def component(self) -> Draw:
        return C.StatusDot(self.tone)


class Avatar(KitWidget):
    """A picture (`image`: an `AvatarImage` or a PNG path, in half-block pixels), else the `glyph`,
    else the initial of `name`; `size` small (2×1), medium (4×2) or large (6×3); `round` clears the
    corners of a picture; `status` adds a toned dot. `icon` (a freedesktop name) is accepted and
    not drawn: a terminal has no icons."""

    def __init__(self, name: str, *, image: AvatarImage | str | Path | None = None, icon: str | None = None,
                 glyph: str = "", size: AvatarSize | str = AvatarSize.MEDIUM, round: bool = False,
                 status: Tone | str | None = None, **kw):
        super().__init__(**kw)
        if isinstance(image, (str, Path)):
            image = AvatarImage.load(image)
        self.cell = C.Avatar(name, image=image, icon=icon, glyph=glyph, size=size, round=round, status=status)

    def component(self) -> Draw:
        return self.cell


class TileGrid(KitWidget, can_focus=True):
    """Tiles (`Tile(label, glyph=...)` or `Tile(label, avatar=C.Avatar(...))`) 11 × 4 cells each,
    `columns` to a row; the `cursor` tile takes the hover fill, the `current` one the selected fill.
    The arrows and h/j/k/l move the cursor, the pointer moves it, Return, Space or a click post
    `Selected`."""

    class Selected(Message):
        def __init__(self, grid: "TileGrid", index: int) -> None:
            super().__init__()
            self.grid = grid
            self.index = index

        @property
        def control(self) -> "TileGrid":
            return self.grid

    cursor = kit_reactive(None)
    current = kit_reactive(None)

    def __init__(self, tiles: Iterable[Tile], *, columns: int = 4, cursor: int | None = None,
                 current: int | None = None, **kw):
        super().__init__(**kw)
        self.tiles, self.columns = list(tiles), max(1, columns)
        self.set_reactive(TileGrid.cursor, cursor)
        self.set_reactive(TileGrid.current, current)

    def component(self) -> Draw:
        return C.TileGrid(self.tiles, columns=self.columns, cursor=self.cursor, current=self.current, key=self)

    def on_key(self, event: events.Key) -> None:
        i = self.cursor if self.cursor is not None else 0
        n, cols = len(self.tiles), self.columns
        k = event.key
        if k in ("right", "l") and i % cols < cols - 1:
            nxt = min(i + 1, n - 1)
        elif k in ("left", "h") and i % cols != 0:
            nxt = i - 1
        elif k in ("down", "j"):
            nxt = min(i + cols, n - 1)
        elif k in ("up", "k"):
            nxt = max(0, i - cols)
        elif k in ("enter", "space"):
            event.stop()
            self.post_message(self.Selected(self, i))
            return
        else:
            return
        event.stop()
        self.cursor = nxt

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.cursor = p
            self.post_message(self.Selected(self, p))

    def on_mouse_move(self, event: events.MouseMove) -> None:
        p = self.part_at(event)
        if isinstance(p, int) and p != self.cursor:
            self.cursor = p
