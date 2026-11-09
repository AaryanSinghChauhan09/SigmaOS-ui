"""Lists, Tiles."""

from __future__ import annotations

from functools import lru_cache

from textual.app import ComposeResult

from ...cells import Avatar as AvatarCells
from ...cells.base import Tone, g
from ...cells.lists import AvatarImage
from ...core import REPO
from ...widgets import ToggleSwitch
from ...widgets.layout import Col, Row
from ...widgets.lists import Avatar, Badge, Hero, KeyValue, ListRow, ScrollArea, StatusDot, Tile, TileGrid
from ..page import Section

W = 53
LOCK = g(0xF033E)
FORGET = g(0xF0159)


@lru_cache(maxsize=1)
def picture() -> AvatarImage | None:
    """The sample picture (the same 96×96 file as the Qt and Rune galleries')."""
    return AvatarImage.load(REPO / "qt/gallery/assets/avatar.png")


class Lists(Section):
    NAME, TITLE = "lists", "Lists"

    def compose(self) -> ComposeResult:
        neutral = Tone.NEUTRAL
        with Col(gap=1):
            with ScrollArea(width=W, max_rows=7, cursor=1, id="rows"):
                yield ListRow("Coffee Shop", glyph=g(0xF0928), subtitle="WPA2", value="5 GHz",
                              trailing_glyph=(LOCK, neutral), width=W, id="row-0")
                yield ListRow("Basecamp Guest", glyph=g(0xF0925), subtitle="Open", width=W, id="row-1")
                yield ListRow("Basecamp", glyph=g(0xF0928), status=("Connected", Tone.SUCCESS), current=True,
                              width=W, id="row-2")
                yield ListRow("Neighbor 5G", glyph=g(0xF091F), subtitle="Out of range", disabled=True, width=W,
                              id="row-3")
                yield ListRow("Messages", leading=Avatar("Messages", size="small"), subtitle="From Basecamp",
                              trailing=Badge("3", Tone.ACCENT, filled=True), width=W, id="row-4")
                yield ListRow("Headphones", glyph=g(0xF00AF), subtitle="Battery 80%",
                              trailing=ToggleSwitch(True, bare=True, focusable=False), width=W, id="row-5")
                yield ListRow("Office", glyph=g(0xF0922), subtitle="Saved", trailing_glyph=(LOCK, neutral),
                              action=(FORGET, True), action_cursor=True, width=W, id="row-6")
                yield ListRow("Library", glyph=g(0xF0922), subtitle="WPA2", trailing_glyph=(LOCK, neutral),
                              action=(FORGET, True), width=W, id="row-7")
                yield ListRow("Airport Free WiFi", glyph=g(0xF091F), subtitle="Open", width=W, id="row-8")
            chevron = g(0xF0142)
            with Col(gap=0):
                yield ListRow("Appearance", glyph=g(0xF03D8), subtitle="Theme, font, rounding",
                              trailing_glyph=(chevron, neutral), width=W)
                yield ListRow("Display", glyph=g(0xF0379), subtitle="2560 × 1440", trailing_glyph=(chevron, neutral),
                              width=W)
                yield ListRow("Headphones", glyph=g(0xF02CB), subtitle="Output", current=True,
                              trailing_glyph=(g(0xF012C), Tone.ACCENT), width=W)
            yield Hero(g(0xF0928), "Wi-Fi", status="Connected to Basecamp",
                       trailing=ToggleSwitch(True, focusable=False), width=W)
            yield Hero(g(0xF00B2), "Bluetooth", status="No adapter found", error=True, glyph_opacity=0.5, width=W)
            yield KeyValue(
                [
                    ("Ping", "18 ms"),
                    ("Packet loss", "4%", Tone.URGENT),
                    ("Receiving", "1.2 MB/s"),
                    ("Sending", "240 KB/s"),
                    ("IP address", "192.168.1.24"),
                    ("Gateway", "192.168.1.1"),
                ],
                columns=2,
            )

    def on_scroll_area_activated(self, event: ScrollArea.Activated) -> None:
        # Choosing a row makes it the current one (not the disabled one).
        if event.area.id == "rows" and not event.row.disabled:
            for r in event.area.rows:
                r.current = r is event.row


class Tiles(Section):
    NAME, TITLE = "tiles", "Tiles"

    def compose(self) -> ComposeResult:
        pic = picture()
        with Col(gap=1):
            with Row(gap=1):
                yield Badge("New")
                yield Badge("3", Tone.ACCENT, filled=True)
                yield Badge("12", Tone.URGENT, filled=True)
                yield Badge("Online", Tone.SUCCESS)
                yield Badge("Low", Tone.WARNING)
                yield Badge("Offline", Tone.URGENT)
            with Row(gap=2):
                with Row(gap=1):
                    for tone in (Tone.SUCCESS, Tone.WARNING, Tone.URGENT, Tone.MUTED, Tone.ACCENT):
                        yield StatusDot(tone)
                yield Avatar("Ryan", round=True, status=Tone.SUCCESS)
                yield Avatar("Picture", image=pic, status=Tone.WARNING)
                yield Avatar("", glyph=g(0xF048B), status=Tone.URGENT)
            with Row(gap=2):
                yield Avatar("ada", size="small")
                yield Avatar("Basecamp")
                yield Avatar("Omarchy", size="large", round=True)
                yield Avatar("", glyph=g(0xF06A9))
                yield Avatar("Picture", image=pic, size="small")
                yield Avatar("Picture", image=pic, round=True)
                yield Avatar("Picture", image=pic, size="large")
                yield Avatar("Missing", image=None)
            with Row(gap=2):
                yield Avatar("Files", icon="org.gnome.Nautilus")
                yield Avatar("Terminal", icon="utilities-terminal")
                yield Avatar("Firefox", icon="firefox")
                yield Avatar("Chromium", icon="chromium", size="small")
                yield Avatar("Steam", icon="steam", size="large")
                yield Avatar("Btop", icon="btop", size="small")
                yield Avatar("Nothing", icon="no-such-app", glyph=g(0xF120))
            yield TileGrid(
                [
                    Tile("Terminal", glyph=g(0xF120)),
                    Tile("Files", glyph=g(0xF07B)),
                    Tile("Browser", glyph=g(0xF0AC)),
                    Tile("Music", glyph=g(0xF001)),
                    Tile("Settings", glyph=g(0xF013)),
                    Tile("Calendar", glyph=g(0xF073)),
                    Tile("Screen Recorder", glyph=g(0xF03D)),
                    Tile("Photos", avatar=AvatarCells("Photos", image=pic)),
                ],
                columns=4,
                cursor=2,
                id="tile",
            )
