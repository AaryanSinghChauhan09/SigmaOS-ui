"""Surfaces."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells.base import g
from ...widgets import (
    Box,
    Card,
    Col,
    ConfirmDialog,
    Heading,
    HeadingLevel,
    IconButton,
    MenuRow,
    Pane,
    Row,
    Separator,
    SettingRow,
    Side,
    Slider,
    Toggle,
    ToggleSwitch,
    Tooltip,
)
from ..page import Section


class Surfaces(Section):
    NAME, TITLE = "surfaces", "Surfaces"

    def compose(self) -> ComposeResult:
        def row(label: str, width: int, **kw) -> MenuRow:
            return MenuRow(label, width=width, focusable=False, **kw)

        card = Card(
            Toggle("Wi-Fi", "Basecamp", True, width=32, id="network"),
            Separator(32),
            Col(row("Basecamp", 32, checked=True), row("Basecamp Guest", 32, highlighted=True), row("Coffee Shop", 32)),
            title="Network",
            width=38,
        )
        confirm = ConfirmDialog("Remove the Coffee Shop network?", width=39, selected=1, id="confirm")
        right = Col(Tooltip("Restart Wi-Fi"), Box(confirm, width=43, height=11, fill=True), gap=1)
        layers = Pane(
            Col(row("Background", 25), row("Title", 25, checked=True), row("Logo", 25)),
            title="Layers",
            subtitle="3 items",
            divider=Side.LEFT,
            actions=[IconButton(g(0xF067), id="add-layer")],
            width=30,
        )
        night = Pane(
            SettingRow("Strength", control=Slider(0.6, width=13, focusable=False), width=24),
            title="Night light",
            subtitle="Sunset to sunrise",
            bordered=True,
            actions=[ToggleSwitch(True, id="night-light")],
            width=30,
        )
        headings = Row(
            Col(
                Heading("Settings", "Omarchy", level=HeadingLevel.DISPLAY, width=30),
                Heading("Appearance", "Theme, font and rounding", level=HeadingLevel.HEADING, width=30),
                gap=1,
            ),
            Col(
                Heading("Frame", trailing=[IconButton(g(0xF141), focusable=False)], width=30),
                Heading("notes.md", "~/Documents", centered=True, width=30),
                gap=1,
            ),
            gap=2,
        )
        with Col(gap=1):
            yield Row(card, right, gap=2)
            yield Row(layers, night, gap=2)
            yield headings
