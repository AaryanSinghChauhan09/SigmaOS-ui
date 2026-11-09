"""Buttons, Toggles, Inputs, Slider."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells.base import g
from ...widgets import Button, ButtonGroup, Col, IconButton, Row, Toggle, ToggleSwitch
from ..page import Section


class Buttons(Section):
    NAME, TITLE = "buttons", "Buttons"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=1):
                yield Button("Plain", id="plain")
                yield Button("Bordered", bordered=True, id="bordered")
                yield Button("Hover", bordered=True, cursor=True)
                yield Button("Selected", bordered=True, selected=True)
                yield Button("Icon", icon=g(0xF013), bordered=True, id="icon")
                yield IconButton(g(0xF011), bordered=True, id="power")
                yield IconButton(g(0xF021), bordered=True, cursor=True)
            yield ButtonGroup(["Left", "Center", "Right"], 1, id="align")


class Toggles(Section):
    NAME, TITLE = "toggles", "Toggles"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=2):
                for i, on in enumerate([False, True, False, True]):
                    yield ToggleSwitch(on, cursor=i >= 2, id=f"switch-{i}")
            yield Toggle("Wi-Fi", "Connected to Basecamp", True, width=45, id="wifi")
            yield Toggle("Bluetooth", "Off", False, width=45, cursor=True, id="bluetooth")
