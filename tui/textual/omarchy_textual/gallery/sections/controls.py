"""Buttons, Toggles, Inputs, Slider."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells.base import g
from ...widgets import Button, ButtonGroup, Col, Dropdown, IconButton, NumberField, Row, TextField, Toggle, ToggleSwitch
from ...widgets import Slider as SliderWidget
from ..page import Section


class Buttons(Section):
    NAME, TITLE = "buttons", "Buttons"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=1):
                yield Button("Plain", id="plain")
                yield Button("Bordered", bordered=True, id="bordered")
                yield Button("Hover", bordered=True, cursor=True, focusable=False)
                yield Button("Selected", bordered=True, selected=True, focusable=False)
                yield Button("Icon", icon=g(0xF013), bordered=True, id="icon")
                yield IconButton(g(0xF011), bordered=True, id="power")
                yield IconButton(g(0xF021), bordered=True, cursor=True, focusable=False)
            yield ButtonGroup(["Left", "Center", "Right"], 1, id="align")


class Toggles(Section):
    NAME, TITLE = "toggles", "Toggles"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=2):
                for i, on in enumerate([False, True, False, True]):
                    # The last two show the cursor's look (specimens, not stops for Tab).
                    yield ToggleSwitch(on, cursor=i >= 2, focusable=i < 2, id=f"switch-{i}")
            yield Toggle("Wi-Fi", "Connected to Basecamp", True, width=45, id="wifi")
            yield Toggle("Bluetooth", "Off", False, width=45, cursor=True, focusable=False, id="bluetooth")


#: The Dropdown's options (as every gallery's).
THEMES = ["Tokyo Night", "Catppuccin", "Gruvbox", "Nord", "Rose Pine"]


class Inputs(Section):
    NAME, TITLE = "inputs", "Inputs"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=1):
                yield TextField("", placeholder="Search…", width=28, id="search")
                yield TextField("Omarchy", width=28, id="name")
                yield TextField("Hovered", width=28, cursor=True, focusable=False)
            with Row(gap=2):
                yield NumberField(10, label="Gaps", range=(0, 40), width=15, id="gaps")
                yield Dropdown(THEMES, 0, label="Theme", width=30, id="theme")


class Slider(Section):
    NAME, TITLE = "slider", "Slider"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            yield SliderWidget(0.35, width=45, id="level")
            yield SliderWidget(0.75, notches=5, width=45, id="stepped")
