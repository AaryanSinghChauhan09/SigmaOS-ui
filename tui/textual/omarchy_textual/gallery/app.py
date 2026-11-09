"""The Textual gallery: every Omarchy UI component in the live theme, section for section with the
other galleries (spec/gallery.md), at the cell sizes spec/terminal.md lists.

In the window: `[` and `]` (or Ctrl+←/→) change section, Tab and Shift+Tab walk the section's
controls, the focused one takes the keys, the mouse clicks and hovers, the wheel and Page Up/Down
scroll, `q` quits.
"""

from __future__ import annotations

from textual import events
from textual.app import ComposeResult
from textual.containers import VerticalScroll
from textual.widget import Widget

from ..app import OmarchyApp
from ..cells import layout as L
from ..cells.base import Draw
from ..cells.paint import Ink, hline, text, text_right
from ..widget import KitWidget
from ..widgets.layout import Canvas
from .page import COLS
from .sections import SECTIONS, section_class


class Header(KitWidget):
    """The gallery, the section and where it is in the list, over a rule."""

    def __init__(self, index: int, **kw):
        super().__init__(**kw)
        self.index = index

    def component(self) -> Draw:
        def draw(t, area, buf):
            text(t, buf, 1, area.y, "Omarchy UI · Textual", area.width, Ink(t.fg(), bold=True))
            title = SECTIONS[self.index][1]
            text_right(t, buf, max(0, area.width - 1), area.y, f"{title}  {self.index + 1}/{len(SECTIONS)}", t.dim())
            hline(t, buf, 0, area.y + 1, area.width, "─", t.track())

        return L.Custom(max(self.size.width, COLS), 2, draw)


class Footer(KitWidget):
    """The keys."""

    def component(self) -> Draw:
        from ..cells import KeyCombo

        def draw(t, area, buf):
            x = 1
            for combo, what in (("Tab", "focus"), ("[", ""), ("]", "section"), ("q", "quit")):
                kc = KeyCombo(combo)
                w = kc.size(t)[0]
                kc.draw(t, L.Rect(x, area.y, w, 1).intersection(area), buf)
                x += w + 1
                if what:
                    x += text(t, buf, x, area.y, what, max(0, area.width - x), t.dim()) + 2

        return L.Custom(max(self.size.width, COLS), 1, draw)


class GalleryApp(OmarchyApp):
    """The gallery. `snap=True` shows the section alone at the top left (for `--snap`)."""

    CSS = """
    Screen { overflow: hidden; }
    #body { height: 1fr; scrollbar-size-vertical: 1; }
    Header, Footer { width: 1fr; }
    """

    #: Nothing has the keys until Tab gives them (the snapshots show every control at rest).
    AUTO_FOCUS = None

    BINDINGS = [
        ("right_square_bracket", "section(1)", "Next section"),
        ("left_square_bracket", "section(-1)", "Previous section"),
        ("ctrl+right", "section(1)", "Next section"),
        ("ctrl+left", "section(-1)", "Previous section"),
        ("q", "quit", "Quit"),
    ]

    def __init__(self, section: str = "buttons", snap: bool = False, theme=None):
        super().__init__(theme=theme)
        names = [n for n, _ in SECTIONS]
        self.index = names.index(section) if section in names else 0
        self.snap = snap

    def make_section(self) -> Widget:
        return section_class(SECTIONS[self.index][0])(id="section")

    def compose(self) -> ComposeResult:
        if self.snap:
            yield self.make_section()
            return
        yield Header(self.index, id="header")
        with VerticalScroll(id="body"):
            yield self.make_section()
        yield Footer(id="footer")

    async def action_section(self, d: int) -> None:
        self.index = (self.index + d) % len(SECTIONS)
        body = self.query_one("#body")
        await body.remove_children()
        await body.mount(self.make_section())
        header = self.query_one(Header)
        header.index = self.index
        header.refresh()
        self.set_focus(None)

    def on_key(self, event: events.Key) -> None:
        if event.key == "escape":
            self.set_focus(None)
