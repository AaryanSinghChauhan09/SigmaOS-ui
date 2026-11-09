"""Levels, work in progress, emptiness and outcomes as Textual widgets (spec/terminal.md,
Feedback): `Meter`, `ProgressBar`, `Spinner`, `EmptyState`, `Banner`, `Toast`. Spinners and the
indeterminate bar animate on the app's clock (none under reduced motion: their still frames)."""

from __future__ import annotations

from typing import Iterable

from textual import events
from textual.message import Message

from .. import cells as C
from ..cells.base import Draw, Tone
from ..color import Rgba
from ..widget import KitWidget, kit_reactive

__all__ = ["Banner", "EmptyState", "Meter", "ProgressBar", "Spinner", "Toast"]

#: Frames a second while a spinner turns or a bar sweeps.
ANIMATION_FPS = 30


class _Animated(KitWidget):
    """Redraws on the app's clock while it animates and motion isn't reduced."""

    def animating(self) -> bool:
        return False

    def on_mount(self) -> None:
        self.set_interval(1 / ANIMATION_FPS, self._tick, name="omarchy-animation")

    def _tick(self) -> None:
        if self.animating() and not self.theme_.reduce_motion():
            self.refresh()


class Meter(KitWidget):
    """A determinate level (0–1): an optional header (`label`, `value_text` at the end), then the
    bar in the tone (`urgent` when `alarming`), with `markers` (fractions) on it."""

    value = kit_reactive(0.0)
    label = kit_reactive("")
    value_text = kit_reactive("")
    alarming = kit_reactive(False)

    def __init__(self, value: float = 0.0, *, tone: Tone | str | None = None, alarming: bool = False,
                 markers: Iterable[float] = (), label: str = "", value_text: str = "", width: int = 28, **kw):
        super().__init__(**kw)
        self.set_reactive(Meter.value, value)
        self.set_reactive(Meter.label, label)
        self.set_reactive(Meter.value_text, value_text)
        self.set_reactive(Meter.alarming, alarming)
        self.tone = tone
        self.markers = list(markers)
        self.width = width

    def component(self) -> Draw:
        return C.Meter(self.value, tone=self.tone, alarming=self.alarming, markers=self.markers,
                       label=self.label, value_text=self.value_text, width=self.width)


class ProgressBar(_Animated):
    """A Meter in accent; `indeterminate` sweeps a quarter-width segment across every 1.4 s."""

    value = kit_reactive(0.0)
    indeterminate = kit_reactive(False)
    label = kit_reactive("")
    value_text = kit_reactive("")

    def __init__(self, value: float = 0.0, *, indeterminate: bool = False, tone: Tone | str | None = None,
                 label: str = "", value_text: str = "", width: int = 28, **kw):
        super().__init__(**kw)
        self.set_reactive(ProgressBar.value, value)
        self.set_reactive(ProgressBar.indeterminate, indeterminate)
        self.set_reactive(ProgressBar.label, label)
        self.set_reactive(ProgressBar.value_text, value_text)
        self.tone = tone
        self.width = width

    def animating(self) -> bool:
        return self.indeterminate

    def component(self) -> Draw:
        return C.ProgressBar(self.value, indeterminate=self.indeterminate, tone=self.tone, label=self.label,
                             value_text=self.value_text, width=self.width)


class Spinner(_Animated):
    """Busy: the braille frames turning (a turn in 900 ms), or with `pulse` the glyph breathing
    between half and full opacity. Still under reduced motion (and in snapshots)."""

    pulse = kit_reactive(False)

    def __init__(self, *, pulse: bool = False, glyph: str = C.glyphs.LOADING, color: Rgba | None = None, **kw):
        super().__init__(**kw)
        self.set_reactive(Spinner.pulse, pulse)
        self.glyph = glyph
        self.color = color

    def animating(self) -> bool:
        return True

    def component(self) -> Draw:
        return C.Spinner(pulse=self.pulse, glyph=self.glyph, color=self.color)


class EmptyState(KitWidget):
    """What a list or panel shows when it has nothing: glyph, title, hint and an optional bordered
    action Button, centered in `width`. `overlay` for over content (a search with no matches).
    With an action it takes focus; Enter, Space or a click on the button post `ActionPressed`."""

    class ActionPressed(Message):
        def __init__(self, empty: "EmptyState") -> None:
            super().__init__()
            self.empty = empty

        @property
        def control(self) -> "EmptyState":
            return self.empty

    title = kit_reactive("")
    hint = kit_reactive("")

    def __init__(self, title: str, *, glyph: str = "", hint: str = "", action: str = "", overlay: bool = False,
                 width: int = 44, cursor: bool = False, **kw):
        kw.setdefault("focusable", bool(action))
        super().__init__(**kw)
        self.set_reactive(EmptyState.title, title)
        self.set_reactive(EmptyState.hint, hint)
        self.glyph, self.action, self.overlay, self.width = glyph, action, overlay, width
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.EmptyState(self.title, glyph=self.glyph, hint=self.hint, action=self.action, overlay=self.overlay,
                            cursor=self.hot and bool(self.action), width=self.width, key=self)

    @property
    def hot(self) -> bool:
        # Only focus (or the pinned look) lights the action; the pointer over the panel doesn't.
        return self.pinned_cursor or self.has_focus

    def press(self) -> None:
        if self.action and not self.disabled:
            self.post_message(self.ActionPressed(self))

    def on_click(self, event: events.Click) -> None:
        if self.part_at(event) == "action":
            event.stop()
            self.press()

    def on_key(self, event: events.Key) -> None:
        if event.key in ("enter", "space"):
            event.stop()
            self.press()


class Banner(KitWidget):
    """An inline message, one row: the tone's edges and glyph (`info`, `success`, `warning`,
    `error`), the text, an optional bordered `action` Button and a dismiss mark. Clicks post
    `ActionPressed` and `Dismissed` (the app removes it, or not)."""

    class ActionPressed(Message):
        def __init__(self, banner: "Banner") -> None:
            super().__init__()
            self.banner = banner

        @property
        def control(self) -> "Banner":
            return self.banner

    class Dismissed(Message):
        def __init__(self, banner: "Banner") -> None:
            super().__init__()
            self.banner = banner

        @property
        def control(self) -> "Banner":
            return self.banner

    text = kit_reactive("")
    tone = kit_reactive(Tone.INFO)

    def __init__(self, text: str, *, tone: Tone | str | None = None, glyph: str = "", action: str = "",
                 dismissible: bool = False, width: int = 92, cursor: bool = False, **kw):
        kw.setdefault("focusable", False)
        super().__init__(**kw)
        self.set_reactive(Banner.text, text)
        self.set_reactive(Banner.tone, Tone.INFO if tone is None else Tone.of(tone))
        self.glyph, self.action, self.dismissible, self.width = glyph, action, dismissible, width
        self.pinned_cursor = cursor

    @property
    def hot(self) -> bool:
        return self.pinned_cursor or self.has_focus

    def component(self) -> Draw:
        return C.Banner(self.text, tone=self.tone, glyph=self.glyph, action=self.action,
                        dismissible=self.dismissible, cursor=self.hot and bool(self.action), width=self.width, key=self)

    def on_click(self, event: events.Click) -> None:
        part = self.part_at(event)
        if part == "action":
            event.stop()
            self.post_message(self.ActionPressed(self))
        elif part == "dismiss":
            event.stop()
            self.post_message(self.Dismissed(self))

    def on_key(self, event: events.Key) -> None:
        if event.key in ("enter", "space") and self.action:
            event.stop()
            self.post_message(self.ActionPressed(self))
        elif event.key in ("escape", "delete") and self.dismissible:
            event.stop()
            self.post_message(self.Dismissed(self))


class Toast(KitWidget):
    """A transient message on the notifications surface: glyph, bold title over the text, an
    optional `action` Button (a click posts `ActionPressed`), and the `countdown` (0–1, the share
    of its time left) as the bottom edge. The pointer over it shows the hover fill."""

    class ActionPressed(Message):
        def __init__(self, toast: "Toast") -> None:
            super().__init__()
            self.toast = toast

        @property
        def control(self) -> "Toast":
            return self.toast

    title = kit_reactive("")
    text = kit_reactive("")
    countdown = kit_reactive(None)

    def __init__(self, title: str, text: str = "", *, glyph: str = "", action: str = "",
                 countdown: float | None = None, width: int = 45, cursor: bool = False, **kw):
        kw.setdefault("focusable", False)
        super().__init__(**kw)
        self.set_reactive(Toast.title, title)
        self.set_reactive(Toast.text, text)
        self.set_reactive(Toast.countdown, countdown)
        self.glyph, self.action, self.width = glyph, action, width
        self.pinned_cursor = cursor

    def component(self) -> Draw:
        return C.Toast(self.title, text=self.text, glyph=self.glyph, action=self.action, countdown=self.countdown,
                       cursor=self.hot, width=self.width, key=self)

    def on_click(self, event: events.Click) -> None:
        if self.part_at(event) == "action":
            event.stop()
            self.post_message(self.ActionPressed(self))

    def on_key(self, event: events.Key) -> None:
        if event.key in ("enter", "space") and self.action:
            event.stop()
            self.post_message(self.ActionPressed(self))
