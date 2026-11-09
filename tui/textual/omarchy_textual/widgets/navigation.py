"""Navigation as Textual widgets (spec/terminal.md, Navigation): `Menu` (and `ContextMenu`) with the
keys every binding takes (Up/Down and j/k wrapping over enabled rows, Home/End, Enter), `Tabs` and
`IconRail` choosing with keys and clicks, the `Toolbar`'s slots of widgets, and the static
`Keycap`, `KeyCombo` and `ShortcutSheet`."""

from __future__ import annotations

from typing import Iterable

from textual import events
from textual.message import Message
from textual.widget import Widget

from .. import cells as C
from ..cells.base import Draw
from ..cells.navigation import MenuItem, MenuSection, MenuSeparator
from ..widget import KitWidget, kit_reactive

__all__ = [
    "ContextMenu",
    "IconRail",
    "KeyCombo",
    "Keycap",
    "Menu",
    "MenuItem",
    "MenuSection",
    "MenuSeparator",
    "ShortcutSheet",
    "Tabs",
    "Toolbar",
    "ToolbarSeparator",
]


class Keycap(KitWidget):
    """One key, its name normalized (`Shift` is ⇧, `Enter` ⏎, ...)."""

    key_name = kit_reactive("")

    def __init__(self, key: str, **kw):
        super().__init__(**kw)
        self.set_reactive(Keycap.key_name, key)

    def component(self) -> Draw:
        return C.Keycap(self.key_name)


class KeyCombo(KitWidget):
    """A combination, "Ctrl+Shift+P", as Keycaps side by side."""

    combo = kit_reactive("")

    def __init__(self, combo: str, **kw):
        super().__init__(**kw)
        self.set_reactive(KeyCombo.combo, combo)

    def component(self) -> Draw:
        return C.KeyCombo(self.combo)


class ShortcutSheet(KitWidget):
    """Grouped shortcuts (`[(title, [(combo, description), ...]), ...]`) in `columns` columns."""

    def __init__(self, groups: Iterable[tuple[str, Iterable[tuple[str, str]]]], columns: int = 2, **kw):
        super().__init__(**kw)
        self.groups = [(t, list(rows)) for t, rows in groups]
        self.columns = columns

    def component(self) -> Draw:
        return C.ShortcutSheet(self.groups, columns=self.columns)


class Menu(KitWidget, can_focus=True):
    """A menu on the `menu` surface. `entries` are `MenuItem`s, `MenuSeparator()`s and
    `MenuSection("…")`s; `cursor` is an index among the items. Up/Down and j/k move the cursor
    over enabled rows (wrapping), Home/End jump, Enter/Space or a click run the row: a checkable
    row flips, a radio row is chosen in its group, and `Menu.Selected` is posted. With
    `cursor_on_focus` the cursor shows only while the menu has the keys (a context menu)."""

    class Selected(Message):
        def __init__(self, menu: "Menu", index: int, item: MenuItem) -> None:
            super().__init__()
            self.menu = menu
            self.index = index
            self.item = item

        @property
        def control(self) -> "Menu":
            return self.menu

    cursor = kit_reactive(0)

    def __init__(self, *entries: MenuItem | MenuSeparator | MenuSection, cursor: int = 0,
                 cursor_on_focus: bool = False, min_width: int = 20, **kw):
        super().__init__(**kw)
        self.entries = list(entries)
        self.set_reactive(Menu.cursor, cursor)
        self.cursor_on_focus = cursor_on_focus
        self.min_width = min_width

    @property
    def items(self) -> list[MenuItem]:
        return [e for e in self.entries if isinstance(e, MenuItem)]

    def component(self) -> Draw:
        show = self.hot or not self.cursor_on_focus
        n = 0
        entries = []
        for e in self.entries:
            if isinstance(e, MenuItem):
                i = n
                n += 1
                e = MenuItem(e.label, icon=e.icon, shortcut=e.shortcut, check=e.check, radio=e.radio,
                             submenu=e.submenu, disabled=e.disabled, destructive=e.destructive,
                             cursor=show and i == self.cursor, key=(self, i))
            entries.append(e)
        return C.Menu(*entries, min_width=self.min_width)

    def run(self, index: int) -> None:
        """Run item `index`: flip a check, choose a radio in its group, post `Selected`."""
        items = self.items
        if not (0 <= index < len(items)) or items[index].disabled:
            return
        item = items[index]
        if item.check is not None:
            item.check = not item.check
        if item.radio is not None:
            groups: list[list[MenuItem]] = [[]]
            for e in self.entries:
                if isinstance(e, MenuItem):
                    groups[-1].append(e)
                else:
                    groups.append([])
            for group in groups:
                if any(g is item for g in group):
                    for g in group:
                        if g.radio is not None:
                            g.radio = g is item
        self.cursor = index
        self.refresh()
        self.post_message(self.Selected(self, index, item))

    def on_key(self, event: events.Key) -> None:
        if event.key in ("enter", "space"):
            event.stop()
            self.run(self.cursor)
            return
        n = C.Menu.handle_key(self.cursor, [not i.disabled for i in self.items], event.key)
        if n is not None:
            event.stop()
            self.cursor = n

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.run(p)


#: A Menu that opens at a point rather than under a trigger.
ContextMenu = Menu


class ToolbarSeparator(KitWidget):
    """A `track` rule `│` with a column clear each side."""

    def component(self) -> Draw:
        return C.ToolbarSeparator()


class Toolbar(KitWidget):
    """One row of three slots of widgets: `leading` at the left, `center` centered on the bar,
    `trailing` at the right, then the overflow button (`overflow`; a click posts
    `Toolbar.Overflow`); a `track` rule under it when `divided`."""

    class Overflow(Message):
        def __init__(self, toolbar: "Toolbar") -> None:
            super().__init__()
            self.toolbar = toolbar

        @property
        def control(self) -> "Toolbar":
            return self.toolbar

    def __init__(self, width: int, *, leading: Iterable[Widget] = (), center: Iterable[Widget] = (),
                 trailing: Iterable[Widget] = (), overflow: bool = False, divided: bool = True, **kw):
        self._slots = (list(leading), list(center), list(trailing))
        super().__init__(*self._slots[0], *self._slots[1], *self._slots[2], **kw)
        self.width = width
        self.overflow = overflow
        self.divided = divided

    def component(self) -> Draw:
        mounted = [c for c in self.children]

        def parts(ws: list[Widget]) -> list[Draw]:
            return self.parts([w for w in ws if any(w is m for m in mounted)])

        lead, center, trail = self._slots
        return C.Toolbar(self.width, leading=parts(lead), center=parts(center), trailing=parts(trail),
                         overflow=self.overflow, overflow_cursor=False, divided=self.divided,
                         overflow_key=(self, "overflow"))

    def on_click(self, event: events.Click) -> None:
        if self.part_at(event) == "overflow":
            event.stop()
            self.post_message(self.Overflow(self))


class Tabs(KitWidget, can_focus=True):
    """Text tabs over a `track` rule `width` wide; the `current` tab bold with its span of the rule
    in accent. Left/Right (h/l), Home/End or a click choose (`Tabs.Changed`); the cursor (hover
    fill) follows the pointer and the keys."""

    class Changed(Message):
        def __init__(self, tabs: "Tabs", index: int) -> None:
            super().__init__()
            self.tabs = tabs
            self.index = index

        @property
        def control(self) -> "Tabs":
            return self.tabs

    current = kit_reactive(0)
    cursor = kit_reactive(None)

    def __init__(self, labels: Iterable[str], current: int = 0, *, cursor: int | None = None,
                 width: int | None = None, **kw):
        super().__init__(**kw)
        self.labels = list(labels)
        self.set_reactive(Tabs.current, current)
        self.set_reactive(Tabs.cursor, cursor)
        self.width = width

    def component(self) -> Draw:
        return C.Tabs(self.labels, self.current, cursor=self.cursor, width=self.width, key=self)

    def choose(self, i: int) -> None:
        self.cursor = i
        if i != self.current:
            self.current = i
            self.post_message(self.Changed(self, i))

    def on_key(self, event: events.Key) -> None:
        n = C.Tabs.handle_key(self.current, len(self.labels), event.key)
        if n is not None:
            event.stop()
            self.choose(n)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.choose(p)

    def on_mouse_move(self, event: events.MouseMove) -> None:
        p = self.part_at(event)
        if isinstance(p, int) and p != self.cursor:
            self.cursor = p


class IconRail(KitWidget, can_focus=True):
    """A vertical rail of glyphs (`[(glyph, label), ...]`), the `current` one accent with a bar at
    the leading edge; the cursor (hover fill) on it while the rail has the keys. Up/Down (k/j),
    Home/End or a click choose (`IconRail.Changed`)."""

    class Changed(Message):
        def __init__(self, rail: "IconRail", index: int) -> None:
            super().__init__()
            self.rail = rail
            self.index = index

        @property
        def control(self) -> "IconRail":
            return self.rail

    current = kit_reactive(0)

    def __init__(self, items: Iterable[tuple[str, str]], current: int = 0, *, height: int | None = None, **kw):
        super().__init__(**kw)
        self.items = list(items)
        self.set_reactive(IconRail.current, current)
        self.height = height

    def component(self) -> Draw:
        return C.IconRail(self.items, self.current, cursor=self.current if self.has_focus else None,
                          height=self.height, key=self)

    def choose(self, i: int) -> None:
        if i != self.current:
            self.current = i
            self.post_message(self.Changed(self, i))

    def on_key(self, event: events.Key) -> None:
        n = C.IconRail.handle_key(self.current, len(self.items), event.key)
        if n is not None:
            event.stop()
            self.choose(n)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.choose(p)
