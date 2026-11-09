"""Surfaces as Textual widgets (spec/terminal.md, Surfaces): `Card` and `Pane` are containers
(`with Card(title="Network"): ...`) laying their children out as the kit does (padding 2 × 1,
children one row apart); `Heading` takes trailing widgets; `Dialog` and `ConfirmDialog` choose with
the keys or a click and post `Dialog.Chosen`."""

from __future__ import annotations

from typing import Iterable

from textual import events
from textual.message import Message
from textual.widget import Widget

from .. import cells as C
from ..cells.base import Draw
from ..cells.surfaces import HeadingLevel, Side
from ..widget import KitWidget, kit_reactive

__all__ = [
    "Card",
    "ConfirmDialog",
    "Dialog",
    "Heading",
    "HeadingLevel",
    "MenuRow",
    "Pane",
    "SectionHeader",
    "Separator",
    "Side",
    "Tooltip",
]


class SectionHeader(KitWidget):
    """The caption that labels a group of rows: bold, `textDim`."""

    text = kit_reactive("")

    def __init__(self, text: str, **kw):
        super().__init__(**kw)
        self.set_reactive(SectionHeader.text, text)

    def component(self) -> Draw:
        return C.SectionHeader(self.text)


class Separator(KitWidget):
    """A rule of `─` in `track`, `width` columns."""

    width = kit_reactive(30)

    def __init__(self, width: int = 30, **kw):
        super().__init__(**kw)
        self.set_reactive(Separator.width, width)

    def component(self) -> Draw:
        return C.Separator(self.width)


class Card(KitWidget):
    """The raised surface panels and popups are drawn on: a box in the popups border color, its
    `title` in the top edge, the children one row apart inside."""

    title = kit_reactive("")
    width = kit_reactive(45)

    def __init__(self, *children: Widget, title: str = "", width: int = 45, **kw):
        super().__init__(*children, **kw)
        self.set_reactive(Card.title, title)
        self.set_reactive(Card.width, width)

    def component(self) -> Draw:
        return C.Card(*self.parts(), title=self.title, width=self.width)


class Pane(KitWidget):
    """A flat surface inside a window: padding, an optional heading (`title`, `subtitle`) with
    `actions` (widgets) at its end, the children one row apart. Plain, `divider` (a `track` rule on
    a `Side`) or `bordered` (the controls' normal glass in a box)."""

    title = kit_reactive("")
    subtitle = kit_reactive("")

    def __init__(self, *children: Widget, title: str = "", subtitle: str = "", actions: Iterable[Widget] = (),
                 divider: Side | str | None = None, bordered: bool = False, width: int = 30, **kw):
        self._actions = list(actions)
        super().__init__(*self._actions, *children, **kw)
        self.set_reactive(Pane.title, title)
        self.set_reactive(Pane.subtitle, subtitle)
        self.divider = Side(divider) if isinstance(divider, str) else divider
        self.bordered = bordered
        self.width = width

    def component(self) -> Draw:
        actions = [c for c in self.children if any(c is a for a in self._actions)]
        rest = [c for c in self.children if not any(c is a for a in self._actions)]
        return C.Pane(*self.parts(rest), title=self.title, subtitle=self.subtitle, actions=self.parts(actions),
                      divider=self.divider, bordered=self.bordered, width=self.width)


class Heading(KitWidget):
    """A bold title line, an optional `textDim` `subtitle` under it, `trailing` widgets at the end of
    its `width`; `centered` centers the lines in the room before them."""

    text = kit_reactive("")
    subtitle = kit_reactive("")

    def __init__(self, text: str, subtitle: str = "", *, level: HeadingLevel | str = HeadingLevel.TITLE,
                 centered: bool = False, width: int | None = None, trailing: Iterable[Widget] = (), **kw):
        super().__init__(*trailing, **kw)
        self.set_reactive(Heading.text, text)
        self.set_reactive(Heading.subtitle, subtitle)
        self.level = HeadingLevel(level) if isinstance(level, str) else level
        self.centered = centered
        self.width = width

    def component(self) -> Draw:
        return C.Heading(self.text, self.subtitle, level=self.level, centered=self.centered, width=self.width,
                         trailing=self.parts())


class MenuRow(KitWidget, can_focus=True):
    """A list or menu row: the label, an accent check at the end when `checked`, the hover fill
    under the cursor (or `highlighted`). Enter, Space or a click post `MenuRow.Selected`."""

    class Selected(Message):
        def __init__(self, row: "MenuRow") -> None:
            super().__init__()
            self.row = row

        @property
        def control(self) -> "MenuRow":
            return self.row

    label = kit_reactive("")
    checked = kit_reactive(False)
    highlighted = kit_reactive(False)

    def __init__(self, label: str, *, checked: bool = False, highlighted: bool = False, width: int = 30, **kw):
        super().__init__(**kw)
        self.set_reactive(MenuRow.label, label)
        self.set_reactive(MenuRow.checked, checked)
        self.set_reactive(MenuRow.highlighted, highlighted)
        self.width = width

    def component(self) -> Draw:
        return C.MenuRow(self.label, checked=self.checked, highlighted=self.highlighted or self.hot,
                         width=self.width, key=self)

    def on_click(self, event: events.Click) -> None:
        event.stop()
        self.post_message(self.Selected(self))

    def on_key(self, event: events.Key) -> None:
        if event.key in ("enter", "space"):
            event.stop()
            self.post_message(self.Selected(self))


class Tooltip(KitWidget):
    """The tooltip surface: the text and, with a `shortcut`, its KeyCombo."""

    text = kit_reactive("")
    shortcut = kit_reactive("")

    def __init__(self, text: str, shortcut: str = "", **kw):
        super().__init__(**kw)
        self.set_reactive(Tooltip.text, text)
        self.set_reactive(Tooltip.shortcut, shortcut)

    def component(self) -> Draw:
        return C.Tooltip(self.text, self.shortcut)


class Dialog(KitWidget, can_focus=True):
    """A modal card on the scrim (over the whole area it is given): `title`, `message`, an optional
    `body` widget, then the secondary and primary buttons (`labels`; empty: the catalog's Cancel and
    Confirm), the `selected` one in the menu's selected look (urgent when `destructive`).
    Left/Right (h/l) move the selection; Enter chooses it, Escape the secondary; a click chooses a
    button. Each choice posts `Dialog.Chosen` (0 secondary, 1 primary)."""

    class Chosen(Message):
        def __init__(self, dialog: "Dialog", index: int) -> None:
            super().__init__()
            self.dialog = dialog
            self.index = index

        @property
        def accepted(self) -> bool:
            return self.index == 1

        @property
        def control(self) -> "Dialog":
            return self.dialog

    title = kit_reactive("")
    message = kit_reactive("")
    selected = kit_reactive(1)

    def __init__(self, title: str = "", message: str = "", *, body: Widget | None = None,
                 labels: tuple[str, str] = ("", ""), destructive: bool = False, selected: int = 1, width: int = 46,
                 scrim: bool = True, **kw):
        self._body = body
        super().__init__(*([body] if body is not None else []), **kw)
        self.set_reactive(Dialog.title, title)
        self.set_reactive(Dialog.message, message)
        self.set_reactive(Dialog.selected, selected)
        self.labels = labels
        self.destructive = destructive
        self.width = width
        self.scrim = scrim

    def component(self) -> Draw:
        body = self.part(self._body) if self._body is not None and self._body.parent is self else None
        return C.Dialog(self.title, self.message, body=body, labels=self.labels, destructive=self.destructive,
                        selected=self.selected, width=self.width, scrim=self.scrim, key=self)

    def choose(self, index: int) -> None:
        self.selected = index
        self.post_message(self.Chosen(self, index))

    def on_key(self, event: events.Key) -> None:
        n = C.Dialog.handle_key(self.selected, event.key) if event.key not in ("tab", "shift+tab") else None
        if n is not None:
            event.stop()
            self.selected = n
        elif event.key == "enter":
            event.stop()
            self.choose(self.selected)
        elif event.key == "escape":
            event.stop()
            self.choose(0)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if p == "cancel":
            event.stop()
            self.choose(0)
        elif p == "accept":
            event.stop()
            self.choose(1)


class ConfirmDialog(Dialog):
    """A Dialog with no title: the `message` and a destructive Confirm, selected."""

    def __init__(self, message: str, *, labels: tuple[str, str] = ("", ""), selected: int = 1, width: int = 46, **kw):
        super().__init__("", message, labels=labels, destructive=True, selected=selected, width=width, **kw)
