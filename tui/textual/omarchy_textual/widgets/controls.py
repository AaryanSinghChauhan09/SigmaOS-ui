"""The one-row controls as Textual widgets (spec/terminal.md, Basics and Forms): focusable, keyboard
first (focus looks like hover), clickable, each posting its own messages like Textual's stock
widgets do (`Button.Pressed`, `ToggleSwitch.Changed`, ...)."""

from __future__ import annotations

from typing import Iterable

from textual import events
from textual.message import Message

from .. import cells as C
from ..cells.base import Draw
from ..widget import KitWidget, kit_reactive


def activates(event: events.Key) -> bool:
    return event.key in ("enter", "space")


class Button(KitWidget, can_focus=True):
    """A button: `label`, an optional `icon` glyph; plain (no fill or edges at rest) unless
    `bordered`; `selected` fills it and bolds the label. Enter, Space or a click press it."""

    class Pressed(Message):
        def __init__(self, button: "Button") -> None:
            super().__init__()
            self.button = button

        @property
        def control(self) -> "Button":
            return self.button

    label = kit_reactive("")
    icon = kit_reactive(None)
    bordered = kit_reactive(False)
    selected = kit_reactive(False)
    width = kit_reactive(None)

    def __init__(self, label: str = "", *, icon: str | None = None, bordered: bool = False,
                 selected: bool = False, width: int | None = None, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(Button.label, label)
        self.set_reactive(Button.icon, icon)
        self.set_reactive(Button.bordered, bordered)
        self.set_reactive(Button.selected, selected)
        self.set_reactive(Button.width, width)
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.Button(self.label, icon=self.icon, bordered=self.bordered, selected=self.selected,
                        cursor=self.hot, width=self.width, key=self)

    def press(self) -> None:
        if not self.disabled:
            self.post_message(self.Pressed(self))

    def on_click(self, event: events.Click) -> None:
        event.stop()
        self.press()

    def on_key(self, event: events.Key) -> None:
        if activates(event):
            event.stop()
            self.press()


class IconButton(Button):
    """One glyph, 5 columns: `[edge] G [edge]`; the hover fill under the cursor, edges only when
    `bordered`."""

    def __init__(self, glyph: str, *, bordered: bool = False, cursor: bool = False, **kw):
        super().__init__(glyph, bordered=bordered, cursor=cursor, **kw)

    def component(self) -> Draw:
        return C.IconButton(self.label, bordered=self.bordered, cursor=self.hot, key=self)


class ButtonGroup(KitWidget, can_focus=True):
    """Bordered buttons one column apart, one `selected`: a segmented choice. Left/Right (h/l),
    Home/End or a click choose."""

    class Changed(Message):
        def __init__(self, group: "ButtonGroup", selected: int) -> None:
            super().__init__()
            self.group = group
            self.selected = selected

        @property
        def control(self) -> "ButtonGroup":
            return self.group

    selected = kit_reactive(0)

    def __init__(self, labels: Iterable[str], selected: int = 0, **kw):
        super().__init__(**kw)
        self.labels = list(labels)
        self.set_reactive(ButtonGroup.selected, selected)

    def component(self) -> Draw:
        return C.ButtonGroup(self.labels, self.selected, cursor=self.selected if self.has_focus else None, key=self)

    def choose(self, i: int) -> None:
        if i != self.selected:
            self.selected = i
            self.post_message(self.Changed(self, i))

    def on_key(self, event: events.Key) -> None:
        n = C.ButtonGroup.handle_key(self.selected, len(self.labels), event.key)
        if n is not None:
            event.stop()
            self.choose(n)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.choose(p)


class ToggleSwitch(KitWidget, can_focus=True):
    """The switch: a 4-cell track, the knob at the start (off) or the end (on), and the ring's
    edges with the cursor (6 columns; `bare` is the 4, for inside a row). Enter, Space or a click
    flip it."""

    class Changed(Message):
        def __init__(self, switch: "ToggleSwitch", value: bool) -> None:
            super().__init__()
            self.switch = switch
            self.value = value

        @property
        def control(self) -> "ToggleSwitch":
            return self.switch

    value = kit_reactive(False)

    def __init__(self, value: bool = False, *, bare: bool = False, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(ToggleSwitch.value, value)
        self.bare = bare
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.ToggleSwitch(self.value, cursor=self.hot, bare=self.bare, key=self)

    def toggle(self) -> None:
        self.value = not self.value
        self.post_message(self.Changed(self, self.value))

    def on_click(self, event: events.Click) -> None:
        event.stop()
        self.toggle()

    def on_key(self, event: events.Key) -> None:
        if activates(event):
            event.stop()
            self.toggle()


class Toggle(ToggleSwitch):
    """A labeled row with the switch at its end; the whole row is the control: `title` bold,
    `description` (`textDim`) under it."""

    title = kit_reactive("")
    description = kit_reactive("")

    def __init__(self, title: str, description: str = "", value: bool = False, *, width: int = 45,
                 cursor: bool = False, **kw):
        super().__init__(value, cursor=cursor, **kw)
        self.set_reactive(Toggle.title, title)
        self.set_reactive(Toggle.description, description)
        self.width = width

    def component(self) -> Draw:
        return C.Toggle(self.title, self.description, on=self.value, cursor=self.hot, width=self.width, key=self)
