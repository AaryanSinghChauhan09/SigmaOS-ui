"""The one-row controls as Textual widgets (spec/terminal.md, Basics and Forms): focusable, keyboard
first (focus looks like hover), clickable, each posting its own messages like Textual's stock
widgets do (`Button.Pressed`, `ToggleSwitch.Changed`, ...)."""

from __future__ import annotations

from typing import Callable, Iterable, Sequence, Union

from textual import events
from textual.message import Message

from .. import cells as C
from ..cells.base import Draw
from ..color import Rgba
from ..widget import KitWidget, kit_reactive

__all__ = [
    "Button",
    "ButtonGroup",
    "Checkbox",
    "ColorWell",
    "Dropdown",
    "DropdownList",
    "IconButton",
    "NumberField",
    "PasswordField",
    "Radio",
    "RadioGroup",
    "Slider",
    "SwatchStrip",
    "TextArea",
    "TextField",
    "Toggle",
    "ToggleSwitch",
]

#: A color given to a widget: an `Rgba`, the name of a theme color (`"accent"`, `"fg"`, `"dim"`,
#: any `Theme` method, or a `palette` key), or a function of the theme. Names and functions are read
#: at every draw, so they follow the theme and fade with it.
ColorSpec = Union[Rgba, str, Callable[[object], Rgba]]


def resolve_color(t, c: ColorSpec) -> Rgba:
    if isinstance(c, Rgba):
        return c
    if callable(c):
        return c(t)
    m = getattr(t, c, None)
    if callable(m):
        return m()
    return getattr(t.shown.palette, c)


def activates(event: events.Key) -> bool:
    return event.key in ("enter", "space")


def edited(text: str, event: events.Key) -> str | None:
    """`text` after a key that edits it (a printable character appends, Backspace removes the
    last), else None."""
    if event.key == "backspace":
        return text[:-1]
    if event.is_printable and event.character and not event.key.startswith(("ctrl+", "alt+")):
        return text + event.character
    return None


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


# TextField, PasswordField, TextArea -------------------------------------------------------------


class TextField(KitWidget, can_focus=True):
    """A single-line input: normal fill and edges at rest, the hover look with the cursor (focus,
    pointer), the caret (a reverse-video cell) at the end of the text while it has the keys. Typing
    and Backspace edit it (`Changed`), Enter submits it (`Submitted`)."""

    class Changed(Message):
        def __init__(self, field: "TextField", value: str) -> None:
            super().__init__()
            self.field = field
            self.value = value

        @property
        def control(self) -> "TextField":
            return self.field

    class Submitted(Changed):
        pass

    value = kit_reactive("")
    placeholder = kit_reactive("")

    def __init__(self, value: str = "", *, placeholder: str = "", width: int = 28, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(TextField.value, value)
        self.set_reactive(TextField.placeholder, placeholder)
        self.width = width
        self.pinned_cursor = cursor

    @property
    def caret(self) -> int | None:
        return len(self.value) if self.has_focus else None

    def component(self) -> Draw:
        return C.TextField(self.value, placeholder=self.placeholder, width=self.width, cursor=self.hot,
                           caret=self.caret, key=self)

    def on_key(self, event: events.Key) -> None:
        if event.key == "enter":
            event.stop()
            self.post_message(self.Submitted(self, self.value))
            return
        v = edited(self.value, event)
        if v is not None:
            event.stop()
            event.prevent_default()
            self.value = v
            self.post_message(self.Changed(self, v))


class PasswordField(TextField):
    """A TextField for secrets: a lock, the text as dots; `revealable` adds the eye (a click on it,
    or F2, shows the text); `error` draws the urgent edges and the message under it."""

    revealed = kit_reactive(False)
    error = kit_reactive("")

    def __init__(self, value: str = "", *, placeholder: str = "", width: int = 28, revealable: bool = False,
                 revealed: bool = False, error: str = "", cursor: bool = False, **kw):
        super().__init__(value, placeholder=placeholder, width=width, cursor=cursor, **kw)
        self.revealable = revealable
        self.set_reactive(PasswordField.revealed, revealed)
        self.set_reactive(PasswordField.error, error)

    def component(self) -> Draw:
        return C.PasswordField(self.value, placeholder=self.placeholder, width=self.width, cursor=self.hot,
                               revealable=self.revealable, revealed=self.revealed, error=self.error,
                               caret=self.caret, key=self)

    def on_key(self, event: events.Key) -> None:
        if event.key == "f2" and self.revealable:
            event.stop()
            self.revealed = not self.revealed
            return
        super().on_key(event)

    def on_click(self, event: events.Click) -> None:
        if self.part_at(event) == "eye":
            event.stop()
            self.revealed = not self.revealed


class TextArea(KitWidget, can_focus=True):
    """Wrapped multi-line text in TextField's frame (edges on every row), `rows` (min, max) tall.
    Typing, Enter (a new line) and Backspace edit it (`Changed`)."""

    class Changed(Message):
        def __init__(self, area: "TextArea", value: str) -> None:
            super().__init__()
            self.area = area
            self.value = value

        @property
        def control(self) -> "TextArea":
            return self.area

    value = kit_reactive("")

    def __init__(self, value: str = "", *, placeholder: str = "", width: int = 60, rows: tuple[int, int] = (3, 6),
                 cursor: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(TextArea.value, value)
        self.placeholder = placeholder
        self.width = width
        self.rows = rows
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.TextArea(self.value, placeholder=self.placeholder, width=self.width, rows=self.rows,
                          cursor=self.hot, caret=len(self.value) if self.has_focus else None, key=self)

    def on_key(self, event: events.Key) -> None:
        v = self.value + "\n" if event.key == "enter" else edited(self.value, event)
        if v is not None:
            event.stop()
            event.prevent_default()
            self.value = v
            self.post_message(self.Changed(self, v))


# NumberField, Dropdown ---------------------------------------------------------------------------


class NumberField(KitWidget, can_focus=True):
    """An integer stepper: an optional `label` row, then `−  value  +` in a field. Up/k/+ and
    Down/j/− step within `range`, a click on − or + steps (`Changed`)."""

    class Changed(Message):
        def __init__(self, field: "NumberField", value: int) -> None:
            super().__init__()
            self.field = field
            self.value = value

        @property
        def control(self) -> "NumberField":
            return self.field

    value = kit_reactive(0)

    def __init__(self, value: int = 0, *, label: str = "", range: tuple[int, int] = (0, 100), width: int = 15,
                 cursor: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(NumberField.value, value)
        self.label = label
        self.range = range
        self.width = width
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.NumberField(self.value, label=self.label, range=self.range, width=self.width, cursor=self.hot, key=self)

    def set_value(self, v: int) -> None:
        v = min(self.range[1], max(self.range[0], v))
        if v != self.value:
            self.value = v
            self.post_message(self.Changed(self, v))

    def on_key(self, event: events.Key) -> None:
        v = C.NumberField.handle_key(self.value, self.range[0], self.range[1], event.key)
        if v is not None:
            event.stop()
            self.set_value(v)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if p == "dec":
            event.stop()
            self.set_value(self.value - 1)
        elif p == "inc":
            event.stop()
            self.set_value(self.value + 1)


class DropdownList(KitWidget):
    """A Dropdown's open list: a box on the popups surface, one row per option, the `highlighted`
    row in the hover fill, the `chosen` one bold with an accent check. Drawn over whatever is under
    it (an overlay of its own, not part of the page's layout); a click on a row picks it."""

    own_root = True

    DEFAULT_CSS = """
    DropdownList {
        overlay: screen;
        position: absolute;
    }
    """

    class Picked(Message):
        def __init__(self, dropdown_list: "DropdownList", index: int) -> None:
            super().__init__()
            self.dropdown_list = dropdown_list
            self.index = index

        @property
        def control(self) -> "DropdownList":
            return self.dropdown_list

    highlighted = kit_reactive(None)
    chosen = kit_reactive(None)

    def __init__(self, options: Sequence[str], chosen: int | None = None, highlighted: int | None = None,
                 width: int = 30, **kw):
        super().__init__(**kw)
        self.options = list(options)
        self.set_reactive(DropdownList.chosen, chosen)
        self.set_reactive(DropdownList.highlighted, highlighted)
        self.width = width

    def component(self) -> Draw:
        return C.DropdownList(self.options, self.chosen, self.highlighted, width=self.width, key=self)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.post_message(self.Picked(self, p))

    def on_mouse_move(self, event: events.MouseMove) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            self.highlighted = p


class Dropdown(KitWidget, can_focus=True):
    """A single choice: an optional `label` row (bold `textDim`), then the trigger (the chosen
    option, the chevron). Enter, Space, Down or a click open the list under the trigger; j/k and
    the arrows walk it, Enter, Space or a click pick (`Changed`), Escape closes it."""

    class Changed(Message):
        def __init__(self, dropdown: "Dropdown", selected: int) -> None:
            super().__init__()
            self.dropdown = dropdown
            self.selected = selected

        @property
        def value(self) -> str:
            return self.dropdown.options[self.selected]

        @property
        def control(self) -> "Dropdown":
            return self.dropdown

    selected = kit_reactive(0)
    expanded = kit_reactive(False)
    row = kit_reactive(0)

    def __init__(self, options: Sequence[str], selected: int = 0, *, label: str = "", width: int = 30,
                 cursor: bool = False, **kw):
        super().__init__(**kw)
        self.options = list(options)
        self.set_reactive(Dropdown.selected, selected)
        self.label = label
        self.width = width
        self.pinned_cursor = cursor
        self._list: DropdownList | None = None

    @property
    def value(self) -> str:
        return self.options[self.selected]

    def component(self) -> Draw:
        return C.Dropdown(self.value, label=self.label, width=self.width, cursor=self.hot, key=self)

    def open(self) -> None:
        if not self.expanded:
            self.row = self.selected
            self.expanded = True

    def close(self) -> None:
        self.expanded = False

    def pick(self, i: int) -> None:
        self.expanded = False
        if i != self.selected:
            self.selected = i
            self.post_message(self.Changed(self, i))

    def watch_expanded(self, on: bool) -> None:
        if on and self._list is None:
            lst = DropdownList(self.options, self.selected, self.row, width=self.width)
            w, h = lst.component().size(self.theme_)
            lst.styles.offset = (0, 2 if self.label else 1)
            lst.styles.width = w
            lst.styles.height = h
            self._list = lst
            self.mount(lst)
        elif not on and self._list is not None:
            self._list.remove()
            self._list = None

    def watch_row(self, row: int) -> None:
        if self._list is not None:
            self._list.highlighted = row

    def watch_selected(self, i: int) -> None:
        if self._list is not None:
            self._list.chosen = i

    def on_dropdown_list_picked(self, event: DropdownList.Picked) -> None:
        event.stop()
        self.pick(event.index)

    def on_blur(self, event: events.Blur) -> None:
        self.close()

    def on_key(self, event: events.Key) -> None:
        k = event.key
        if not self.expanded:
            if k in ("enter", "space", "down"):
                event.stop()
                self.open()
            return
        if k in ("down", "j"):
            self.row = min(len(self.options) - 1, self.row + 1)
        elif k in ("up", "k"):
            self.row = max(0, self.row - 1)
        elif k in ("enter", "space"):
            self.pick(self.row)
        elif k == "escape":
            self.close()
        else:
            return
        event.stop()

    def on_click(self, event: events.Click) -> None:
        if self.part_at(event) is True:
            event.stop()
            if self.expanded:
                self.close()
            else:
                self.open()


# Slider ------------------------------------------------------------------------------------------


class Slider(KitWidget, can_focus=True):
    """A thin track with a knob: `value` 0–1, `notches` evenly spaced steps (ends included).
    Left/h and Right/l step (a notch, or 5%), Home/End jump, a click sets it (`Changed`)."""

    class Changed(Message):
        def __init__(self, slider: "Slider", value: float) -> None:
            super().__init__()
            self.slider = slider
            self.value = value

        @property
        def control(self) -> "Slider":
            return self.slider

    value = kit_reactive(0.0)

    def __init__(self, value: float = 0.0, *, notches: int = 0, width: int = 45, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.set_reactive(Slider.value, C.Slider(value).value)
        self.notches = notches
        self.width = width
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.Slider(self.value, notches=self.notches, width=self.width, cursor=self.hot, key=self)

    def set_value(self, v: float) -> None:
        if v != self.value:
            self.value = v
            self.post_message(self.Changed(self, v))

    def on_key(self, event: events.Key) -> None:
        v = C.Slider.handle_key(self.value, self.notches, event.key)
        if v is not None:
            event.stop()
            self.set_value(v)

    def on_click(self, event: events.Click) -> None:
        r = self.rect_of(self)
        x = self.local_x(event)
        if r is None or x is None:
            return
        event.stop()
        v = C.Slider.value_at(r, x)
        if self.notches > 1:
            steps = self.notches - 1
            from ..color import f32, rround

            v = f32(rround(f32(v * steps)) / steps)
        self.set_value(v)


# Checkbox, Radio, RadioGroup ---------------------------------------------------------------------


class Checkbox(KitWidget, can_focus=True):
    """`[✓] Label`, an optional `description` under it. Enter, Space or a click flip it
    (`Changed`); `disabled` (Textual's) draws it faint and takes nothing."""

    class Changed(Message):
        def __init__(self, checkbox: "Checkbox", value: bool) -> None:
            super().__init__()
            self.checkbox = checkbox
            self.value = value

        @property
        def control(self) -> "Checkbox":
            return self.checkbox

    value = kit_reactive(False)
    exclusive = False

    def __init__(self, label: str, value: bool = False, *, description: str = "", cursor: bool = False, **kw):
        super().__init__(**kw)
        self.label = label
        self.description = description
        self.set_reactive(Checkbox.value, value)
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.Checkbox(self.label, self.description, checked=self.value, exclusive=self.exclusive,
                          cursor=self.hot, disabled=self.disabled, key=self)

    def watch_disabled(self, value: bool) -> None:
        super().watch_disabled(value)
        self.kit_refresh()

    def toggle(self) -> None:
        if self.disabled:
            return
        self.value = not self.value
        self.post_message(self.Changed(self, self.value))

    def on_click(self, event: events.Click) -> None:
        event.stop()
        self.toggle()

    def on_key(self, event: events.Key) -> None:
        if activates(event):
            event.stop()
            self.toggle()


class Radio(Checkbox):
    """`(●) Label`: a Checkbox that a click or key only turns on."""

    exclusive = True

    def toggle(self) -> None:
        if not self.disabled and not self.value:
            self.value = True
            self.post_message(self.Changed(self, True))


class RadioGroup(KitWidget, can_focus=True):
    """Radios stacked, one chosen: `options` are (value, label). The arrows and j/k/h/l move the
    choice, stopping at the ends; a click picks (`Changed`)."""

    class Changed(Message):
        def __init__(self, group: "RadioGroup", value: str) -> None:
            super().__init__()
            self.group = group
            self.value = value

        @property
        def control(self) -> "RadioGroup":
            return self.group

    value = kit_reactive("")

    def __init__(self, options: Iterable[tuple[str, str]], value: str = "", **kw):
        super().__init__(**kw)
        self.options = list(options)
        self.set_reactive(RadioGroup.value, value or (self.options[0][0] if self.options else ""))

    def component(self) -> Draw:
        values = [v for v, _ in self.options]
        cursor = (values.index(self.value) if self.value in values else 0) if self.has_focus else None
        return C.RadioGroup(self.options, self.value, cursor=cursor, key=self)

    def choose(self, v: str) -> None:
        if v != self.value:
            self.value = v
            self.post_message(self.Changed(self, v))

    def on_key(self, event: events.Key) -> None:
        v = C.RadioGroup.handle_key(self.value, [v for v, _ in self.options], event.key)
        if v is not None:
            event.stop()
            self.choose(v)

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, str):
            event.stop()
            self.choose(p)


# ColorWell, SwatchStrip --------------------------------------------------------------------------


class ColorWell(KitWidget, can_focus=True):
    """A color in a bordered well, `▏███▕`, with the ring outside it under the cursor: 7 columns.
    Enter, Space or a click press it (`Pressed`: open a picker)."""

    class Pressed(Message):
        def __init__(self, well: "ColorWell") -> None:
            super().__init__()
            self.well = well

        @property
        def control(self) -> "ColorWell":
            return self.well

    def __init__(self, color: ColorSpec, *, cursor: bool = False, **kw):
        super().__init__(**kw)
        self.color = color
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.ColorWell(resolve_color(self.theme_, self.color), cursor=self.hot, key=self)

    def on_click(self, event: events.Click) -> None:
        event.stop()
        self.post_message(self.Pressed(self))

    def on_key(self, event: events.Key) -> None:
        if activates(event):
            event.stop()
            self.post_message(self.Pressed(self))


class SwatchStrip(KitWidget, can_focus=True):
    """Color chips `██` one column apart, the `chosen` one marked `▔▔` in accent, the cursor's in
    the hover border. Left/h and Right/l move the cursor, Enter or Space choose, a click chooses
    (`Changed`)."""

    class Changed(Message):
        def __init__(self, strip: "SwatchStrip", chosen: int) -> None:
            super().__init__()
            self.strip = strip
            self.chosen = chosen

        @property
        def control(self) -> "SwatchStrip":
            return self.strip

    chosen = kit_reactive(None)
    cursor = kit_reactive(None)

    def __init__(self, colors: Sequence[ColorSpec], chosen: int | None = None, **kw):
        super().__init__(**kw)
        self.swatches = list(colors)
        self.set_reactive(SwatchStrip.chosen, chosen)

    def component(self) -> Draw:
        t = self.theme_
        return C.SwatchStrip([resolve_color(t, c) for c in self.swatches], self.chosen,
                             cursor=self.cursor if self.has_focus else None, key=self)

    def choose(self, i: int) -> None:
        self.cursor = i
        if i != self.chosen:
            self.chosen = i
            self.post_message(self.Changed(self, i))

    def on_key(self, event: events.Key) -> None:
        i = self.cursor if self.cursor is not None else (self.chosen or 0)
        k = event.key
        if k in ("left", "h"):
            self.cursor = max(0, i - 1)
        elif k in ("right", "l"):
            self.cursor = min(len(self.swatches) - 1, i + 1)
        elif activates(event):
            self.choose(i)
        else:
            return
        event.stop()

    def on_click(self, event: events.Click) -> None:
        p = self.part_at(event)
        if isinstance(p, int):
            event.stop()
            self.choose(p)
