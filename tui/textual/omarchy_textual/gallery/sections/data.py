"""Data (Table, Tree) and Layout (a framed SplitView with a Sidebar, the Calendar)."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells import layout as L
from ...cells import paint
from ...cells.base import Draw, g
from ...widget import KitWidget
from ...widgets import Canvas, Col, Row
from ...widgets.data import (Calendar, Column, Date, Sidebar, SidebarItem, SidebarSection, SplitView, Table, Tree,
                             TreeNode)
from ..page import Section

TABLE_ROWS = [
    ["assets", "Folder", "—", "2026-11-02"],
    ["Cargo.toml", "TOML", "1.2 KB", "2026-11-07"],
    ["README.md", "Markdown", "4.8 KB", "2026-11-05"],
    ["screenshot-of-the-new-settings-panel.png", "PNG image", "1.4 MB", "2026-11-08"],
    ["src", "Folder", "—", "2026-11-08"],
    ["theme.toml", "TOML", "812 B", "2026-10-30"],
]


def tree_nodes() -> list[TreeNode]:
    file = g(0xF0219)
    return [
        TreeNode("root", "omarchy-ui", g(0xF0770), [
            TreeNode("qt", "qt", g(0xF0770), [TreeNode("qt/button", "Button.qml", file), TreeNode("qt/table", "Table.qml", file)]),
            TreeNode("rune", "rune", g(0xF024B), [TreeNode("rune/lib", "lib.rs", file)]),
            TreeNode("spec", "spec", g(0xF0770), [TreeNode("spec/tokens", "tokens.md", file)]),
            TreeNode("readme", "README.md", file),
        ])
    ]


def sidebar_sections() -> list[SidebarSection]:
    return [
        SidebarSection("Library", [
            SidebarItem("all", "All files", g(0xF024B)),
            SidebarItem("recent", "Recent", g(0xF02DA)),
            SidebarItem("starred", "Starred", g(0xF04CE)),
            SidebarItem("trash", "Trash", g(0xF0A7A)),
        ]),
        SidebarSection("Places", [SidebarItem("documents", "Documents", g(0xF0219)), SidebarItem("downloads", "Downloads", g(0xF01DA))]),
    ]


class Data(Section):
    NAME, TITLE = "data", "Data"

    def compose(self) -> ComposeResult:
        with Col(gap=0):
            with Row(gap=2):
                yield Table(
                    [Column("Name"), Column("Kind", 11), Column("Size", 9, right=True), Column("Modified", 13)],
                    TABLE_ROWS, sort=(0, True), selected=1, cursor=3, width=60, id="table",
                )
                yield Tree(tree_nodes(), expanded=("root", "qt", "spec"), selected="qt/table", cursor="readme",
                           width=28, id="tree")


class Framed(KitWidget):
    """A `width` × `height` box in `track` with the radius' corners, its one child filling the
    inside."""

    def __init__(self, child, *, width: int, height: int, **kw):
        super().__init__(child, **kw)
        self.box = (width, height)

    def component(self) -> Draw:
        inner = self.parts()

        def draw(t, area, buf):
            r = paint.frame(t, buf, area, t.track())
            for d in inner:
                d.draw(t, r, buf)

        return L.Custom(*self.box, draw)


def _no_file(t, area, buf) -> None:
    paint.text_center(t, buf, area.x, area.y + max(0, area.height - 1) // 2, area.width, "No file selected", t.faint())


class Layout(Section):
    NAME, TITLE = "layout", "Layout"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            split = SplitView(
                Sidebar(sidebar_sections(), selected="recent", width=25, id="sidebar"),
                Canvas(_no_file, 10, 1, id="empty"),
                width=88, height=12, sizes=(25, None), mins=(15, 0), maxes=(40, None), handle_ids=("handle",), id="split",
            )
            yield Framed(split, width=90, height=14)
            yield Calendar(2026, 11, today=Date(2026, 11, 8), selected=Date(2026, 11, 14), id="cal")
