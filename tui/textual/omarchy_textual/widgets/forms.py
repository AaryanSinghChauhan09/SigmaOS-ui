"""Setting rows and the multi-choice dropdown as Textual widgets (spec/terminal.md, Forms)."""

from __future__ import annotations

from typing import Iterable, Sequence

from textual import events
from textual.message import Message
from textual.widget import Widget

from .. import cells as C
from ..cells.base import Draw
from ..cells.forms import MultiSelectState
from ..widget import KitWidget, kit_reactive

__all__ = ["MultiSelect", "MultiSelectState", "SettingRow"]


class SettingRow(KitWidget):
    """A row of a settings list: `label` (and a `textDim` `description` under it) at the start, the
    `control` widget (a ToggleSwitch, Dropdown, Slider, ...) at the end of the first row, a column in
    from the edge. The hover fill across under the pointer (or `cursor=True`). A click on the row
    outside the control gives the control the keys."""

    def __init__(self, label: str, description: str = "", control: Widget | None = None, *, width: int = 60,
                 cursor: bool = False, **kw):
        super().__init__(*([control] if control is not None else []), **kw)
        self.label = label
        self.description = description
        self.control = control
        self.width = width
        self.pinned_cursor = cursor

    @property
    def hot(self) -> bool:
        return self.pinned_cursor or (self.mouse_hover and not self.disabled)

    def component(self) -> Draw:
        ctl = self.part(self.control) if self.control is not None and self.control.display else None
        return C.SettingRow(self.label, self.description, ctl, cursor=self.hot, disabled=self.disabled,
                            width=self.width, key=self)

    def on_click(self, event: events.Click) -> None:
        if self.control is not None and self.control.can_focus:
            self.control.focus()


class MultiSelect(KitWidget, can_focus=True):
    """The searchable multi-choice dropdown: `options` are (value, label), `values` the chosen. The
    trigger shows the chosen labels ("N selected" past three); open (Enter, Space, Down, j or a
    click), the popup under it has a search row and a checkbox per option: typing filters, Enter
    toggles the first match, Down goes into the list, j/k walk, Enter or Space toggle, Up off the
    first row goes back to the search, Escape closes (`Changed` on every toggle)."""

    class Changed(Message):
        def __init__(self, select: "MultiSelect", values: list[str]) -> None:
            super().__init__()
            self.select = select
            self.values = values

        @property
        def control(self) -> "MultiSelect":
            return self.select

    values = kit_reactive((), always_update=True)
    expanded = kit_reactive(False)
    search = kit_reactive("")
    row = kit_reactive(None)

    def __init__(self, options: Iterable[tuple[str, str]], values: Iterable[str] = (), *, label: str = "",
                 width: int = 33, open: bool = False, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.options = list(options)
        self.set_reactive(MultiSelect.values, tuple(values))
        self.set_reactive(MultiSelect.expanded, open)
        self.label = label
        self.width = width
        self.pinned_cursor = cursor

    @property
    def state(self) -> MultiSelectState:
        return MultiSelectState(self.expanded, self.search, self.row)

    def component(self) -> Draw:
        return C.MultiSelect(self, self.options, self.values, label=self.label, open=self.expanded,
                             cursor=self.hot, highlighted=self.row, search=self.search, width=self.width)

    def toggle_value(self, v: str) -> None:
        vals = list(self.values)
        if v in vals:
            vals.remove(v)
        else:
            vals.append(v)
        self.values = tuple(vals)
        self.post_message(self.Changed(self, list(vals)))

    def on_key(self, event: events.Key) -> None:
        st = self.state
        char = event.character if event.is_printable else None
        handled, toggled = C.MultiSelect.handle_key(st, self.options, event.key, char)
        if not handled:
            return
        event.stop()
        event.prevent_default()
        self.expanded, self.search, self.row = st.open, st.search, st.row
        if toggled is not None:
            self.toggle_value(toggled)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if p is True:
            event.stop()
            self.expanded = not self.expanded
        elif isinstance(p, str):
            event.stop()
            self.toggle_value(p)
