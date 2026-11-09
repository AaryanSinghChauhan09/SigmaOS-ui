"""A Wi-Fi panel in the terminal: a Card with a Hero and a list of networks under a keyboard
cursor, following the Omarchy theme live. j/k or the arrows move, Return connects, the mouse
hovers and clicks, q quits. `--snap out.ans [--size COLSxROWS]` writes it as an ANSI snapshot
instead.
"""

from __future__ import annotations

from textual.app import ComposeResult

from omarchy_textual import OmarchyApp, OmarchyThemeChanged, Tone
from omarchy_textual.snap import from_args
from omarchy_textual.widgets import Card, Col, Hero, KeyValue, ListRow, ScrollArea, SectionHeader, Separator, ToggleSwitch

WIFI = "\U000f0928"
LOCK = "\U000f033e"
NETWORKS = [("Basecamp", "WPA2", True), ("Basecamp Guest", "Open", False), ("Coffee Shop", "WPA2", True), ("Library", "Saved", True)]


class Wifi(OmarchyApp):
    BINDINGS = [("q", "quit", "Quit")]

    def __init__(self) -> None:
        super().__init__()
        self.connected = 0
        #: Theme changes seen: the changed hook counts them.
        self.changes = 0

    def compose(self) -> ComposeResult:
        with Card(width=48):
            yield Hero(WIFI, "Wi-Fi", status=self.status(), trailing=ToggleSwitch(True), width=42, id="hero")
            yield SectionHeader("Networks")
            with ScrollArea(width=42, max_rows=8, cursor=1, id="networks"):
                for i, (name, security, locked) in enumerate(NETWORKS):
                    yield ListRow(name, glyph=WIFI, width=42, id=f"net-{i}", **self.row_state(i, security, locked))
            with Col():
                yield Separator(42)
                yield KeyValue(self.stats(), id="stats")

    def status(self) -> str:
        return f"Connected to {NETWORKS[self.connected][0]}"

    def row_state(self, i: int, security: str, locked: bool) -> dict:
        state = {"current": i == self.connected, "trailing_glyph": (LOCK, Tone.NEUTRAL) if locked else None}
        if i == self.connected:
            state["status"] = ("Connected", Tone.SUCCESS)
        else:
            state["subtitle"] = security
        return state

    def stats(self) -> list[tuple[str, str]]:
        return [("Theme changes", str(self.changes)), ("Revision", str(self.kit.revision))]

    def on_mount(self) -> None:
        self.query_one("#networks").focus()

    def on_scroll_area_activated(self, event: ScrollArea.Activated) -> None:
        self.connected = event.index
        for i, (_, security, locked) in enumerate(NETWORKS):
            row = self.query_one(f"#net-{i}", ListRow)
            state = self.row_state(i, security, locked)
            row.current = state["current"]
            row.status = state.get("status")
            row.subtitle = state.get("subtitle", "")
            row.refresh()
        hero = self.query_one("#hero", Hero)
        hero.status = self.status()

    def on_list_row_selected(self, event: ListRow.Selected) -> None:
        index = int(event.row.id.removeprefix("net-"))
        self.on_scroll_area_activated(ScrollArea.Activated(self.query_one("#networks", ScrollArea), index, event.row))

    def on_omarchy_theme_changed(self, event: OmarchyThemeChanged) -> None:
        # The changed hook: once per change, the new tokens already in effect.
        self.changes += 1
        stats = self.query_one("#stats", KeyValue)
        stats.pairs = self.stats()
        stats.refresh(layout=True)


def main() -> None:
    app = Wifi()
    if not from_args(app):
        app.run(mouse=True)


if __name__ == "__main__":
    main()
