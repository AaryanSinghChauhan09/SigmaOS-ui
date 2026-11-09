"""Composed: a Wi-Fi panel, an inspector, and two search lists, built from the kit's widgets."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells.base import Tone, g
from ...cells.search import SearchItem
from ...widgets import (
    Button,
    ColorWell,
    Dropdown,
    IconButton,
    NumberField,
    PasswordField,
    Slider,
    ToggleSwitch,
)
from ...widgets.forms import SettingRow
from ...widgets.layout import Col, Row
from ...widgets.lists import Hero, KeyValue, ListRow, ScrollArea
from ...widgets.navigation import Tabs
from ...widgets.search import SearchList
from ...widgets.surfaces import Card, Pane, SectionHeader, Separator
from ..page import Section


def search_items() -> list[SearchItem]:
    return [
        SearchItem("Terminal", glyph=g(0xF120), subtitle="App", detail="Open a new terminal window."),
        SearchItem("Terminal font", glyph=g(0xF031), subtitle="Style",
                   detail="Pick the monospace font every terminal uses."),
        SearchItem("Terminal theme", glyph=g(0xF1FC), subtitle="Style",
                   detail="Match the terminal colors to the desktop theme."),
        SearchItem("Intermediate CA", glyph=g(0xF0A3), subtitle="Certificate",
                   detail="Trust a certificate authority for this machine."),
    ]


class Composed(Section):
    NAME, TITLE = "composed", "Composed"

    def compose(self) -> ComposeResult:
        w, iw = 39, 40
        lock = (g(0xF033E), Tone.NEUTRAL)
        # Only the two lists take the keys here (as in the reference gallery); the rest are
        # specimens that still take clicks.
        still = {"focusable": False}
        with Col(gap=1):
            with Row(gap=2):
                with Card(width=45):
                    yield Hero(g(0xF0928), "Wi-Fi", status="Connected to Basecamp",
                               trailing=ToggleSwitch(True, **still), width=w)
                    yield SectionHeader("Networks")
                    with ScrollArea(width=w, max_rows=18, cursor=1, id="networks"):
                        yield ListRow("Basecamp", glyph=g(0xF0928), status=("Connected", Tone.SUCCESS), current=True,
                                      width=w, id="wifi-0")
                        yield ListRow("Basecamp Guest", glyph=g(0xF0925), subtitle="Open", action=(g(0xF0159), True),
                                      width=w, id="wifi-1")
                        yield ListRow(
                            "Coffee Shop", glyph=g(0xF0922), subtitle="WPA2", trailing_glyph=lock,
                            expansion=Row(PasswordField(placeholder="Password", width=23, **still),
                                          Button("Connect", bordered=True, **still), gap=1),
                            width=w, id="wifi-2",
                        )
                        yield ListRow("Library", glyph=g(0xF091F), subtitle="Saved", trailing_glyph=lock, width=w,
                                      id="wifi-3")
                    yield Separator(w)
                    yield KeyValue([("IP", "192.168.1.24"), ("Gateway", "192.168.1.1"), ("DNS", "1.1.1.1")])
                with Pane(
                    title="Recording 1", subtitle="1920 × 1080 · 0:42", divider="left", width=45,
                    actions=[IconButton(g(0xF0E2), **still), IconButton(g(0xF01E), **still),
                             Button("Export", bordered=True, **still)],
                ):
                    yield Tabs(["Shape", "Cursor", "Zoom"], 0, width=iw, **still)
                    with Col(gap=0):
                        yield SectionHeader("Frame")
                        yield SettingRow("Radius", control=Slider(0.4, width=14, **still), width=iw)
                        yield SettingRow("Border", control=Dropdown(["Solid"], 0, width=14, **still), width=iw)
                        yield SettingRow("Shadow", control=ToggleSwitch(True, **still), cursor=True, width=iw)
                    with Col(gap=0):
                        yield SectionHeader("Background")
                        yield SettingRow("Fill", control=ColorWell("accent", **still), width=iw)
                        yield SettingRow("Padding", control=NumberField(24, range=(0, 96), width=15, **still),
                                         width=iw)
            with Row(gap=2):
                yield SearchList(search_items(), query="term", cursor=1, detail=True, width=45, height=12,
                                 id="searchlist")
                yield SearchList(search_items(), query="zzz", width=38, height=9, focusable=False,
                                 id="search-empty")
