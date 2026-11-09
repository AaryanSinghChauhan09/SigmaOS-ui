"""Menus, Navigation."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells import paint
from ...cells.base import g
from ...widgets import (
    Button,
    Canvas,
    Col,
    ContextMenu,
    Heading,
    IconButton,
    IconRail,
    KeyCombo,
    Keycap,
    Menu,
    MenuItem,
    MenuSection,
    MenuSeparator,
    Row,
    ShortcutSheet,
    Tabs,
    Toolbar,
    ToolbarSeparator,
    Tooltip,
)
from ..page import CONTENT, Section

KEYS = ["Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K"]
RAIL = [(0xF07B, "Files"), (0xF002, "Search"), (0xF126, "Source control"), (0xF188, "Debug"), (0xF013, "Settings")]


class Menus(Section):
    NAME, TITLE = "menus", "Menus"

    def compose(self) -> ComposeResult:
        menu = Menu(
            MenuSection("File"),
            MenuItem("New", icon=g(0xF15B), shortcut="Ctrl+N"),
            MenuItem("Open…", icon=g(0xF07B), shortcut="Ctrl+O"),
            MenuItem("Save", icon=g(0xF0C7), shortcut="Ctrl+S"),
            MenuItem("Duplicate", icon=g(0xF0C5), disabled=True),
            MenuSeparator(),
            MenuSection("View"),
            MenuItem("Sidebar", check=True),
            MenuItem("Word wrap", check=False),
            MenuSeparator(),
            MenuItem("Export", icon=g(0xF064), submenu=True),
            cursor=1,
            id="menu",
        )
        context = ContextMenu(
            MenuItem("Cut", shortcut="Ctrl+X"),
            MenuItem("Copy", shortcut="Ctrl+C"),
            MenuItem("Paste", shortcut="Ctrl+V"),
            MenuSeparator(),
            MenuItem("By name", radio=True),
            MenuItem("By date", radio=False),
            MenuSeparator(),
            MenuItem("Delete", shortcut="Del", destructive=True),
            cursor=0,
            cursor_on_focus=True,
            id="context",
        )
        sheet = ShortcutSheet(
            [
                ("File", [("Ctrl+N", "New file"), ("Ctrl+O", "Open"), ("Ctrl+S", "Save")]),
                ("View", [("Ctrl+B", "Sidebar"), ("Ctrl+Shift+P", "Commands"), ("F11", "Full screen")]),
            ],
            columns=2,
        )
        with Col(gap=1):
            yield Row(menu, context, Tooltip("Save", "Ctrl+S"), gap=2)
            yield Row(*(Keycap(k) for k in KEYS), gap=1)
            yield KeyCombo("Ctrl+Shift+P")
            yield sheet


class Navigation(Section):
    NAME, TITLE = "navigation", "Navigation"

    def compose(self) -> ComposeResult:
        def ib(cp: int, cursor: bool = False) -> IconButton:
            return IconButton(g(cp), cursor=cursor, focusable=False)

        bar = Toolbar(
            CONTENT,
            leading=[
                ib(0xF032), ib(0xF033, True), ib(0xF0CD), ToolbarSeparator(),
                ib(0xF0CA), ib(0xF0CB), ToolbarSeparator(), ib(0xF03E), ib(0xF0C1),
            ],
            center=[Heading("notes.md", centered=True)],
            trailing=[Button("Share", bordered=True, focusable=False)],
            overflow=True,
            id="toolbar",
        )
        self.rail = IconRail([(g(cp), label) for cp, label in RAIL], 1, id="rail")

        def panel(t, area, buf) -> None:
            name = RAIL[self.rail.current][1]
            paint.text_center(t, buf, area.x, area.y + (area.height - 1) // 2, area.width, name, t.dim())

        self.panel = Canvas(panel, 40, 5, id="panel")
        with Col(gap=1):
            yield bar
            yield Tabs(["General", "Display", "Audio", "Network"], 1, cursor=2, width=CONTENT, id="tabs")
            yield Row(self.rail, self.panel, gap=2)

    def on_icon_rail_changed(self, event: IconRail.Changed) -> None:
        self.panel.refresh()
