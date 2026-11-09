"""The live-switch test (spec/following.md), run under `tools/live/run`: an `OmarchyApp` runs
headless and follows the theme, the theme is switched the way `omarchy theme set` does it, and the
app must see catppuccin-latte through the public API, `OmarchyThemeChanged` once with the new
tokens (after they are in effect, on the app's loop), revision 1, a fade that ran and landed
exactly (the composed screen's painted cell too); under `OMARCHY_UI_REDUCE_MOTION=1`, no fade.

    tools/live/run build/textual/venv/bin/python tui/textual/tests/live_switch.py
"""

from __future__ import annotations

import asyncio
import os
import subprocess
import sys
import threading
import time

from omarchy_textual import Depth, OmarchyApp, OmarchyThemeChanged, Theme
from omarchy_textual.cells.paint import put
from omarchy_textual.widgets import Canvas

LATTE = "#eff1f5ff"


def fail(msg: str) -> None:
    print(f"live switch: FAIL: {msg}", file=sys.stderr)
    sys.exit(1)


class Live(OmarchyApp):
    def __init__(self) -> None:
        theme = Theme.load()
        theme.depth = Depth.TRUE
        super().__init__(theme=theme)
        self.hooks: list[str] = []
        self.revisions: list[int] = []
        self.on_loop: list[bool] = []
        self.faded = False
        self.between = False
        self.start_bg = theme.tokens.palette.background
        self.loop_thread = None

    def compose(self):
        # One cell in the accent, drawn by the kit (so it fades with it).
        yield Canvas(lambda t, area, buf: put(t, buf, area.x, area.y, "█", t.accent()), 1, 1, id="cell")

    def on_mount(self) -> None:
        self.loop_thread = threading.get_ident()
        self.set_interval(1 / 120, self.sample)

    def sample(self) -> None:
        t = self.kit.theme
        shown = t.shown.palette.background
        self.faded |= t.fading
        self.between |= (
            t.tokens.palette.background.hex() == LATTE and shown != self.start_bg and shown != t.tokens.palette.background
        )

    def on_omarchy_theme_changed(self, event: OmarchyThemeChanged) -> None:
        # After the new tokens are in effect: the public API already reads them.
        if self.kit.theme.tokens.palette.background != event.tokens.palette.background:
            fail("the hook ran before the new tokens were in effect")
        self.on_loop.append(threading.get_ident() == self.loop_thread)
        self.hooks.append(event.tokens.palette.background.hex())
        self.revisions.append(event.revision)


async def main() -> None:
    home = os.environ.get("OMARCHY_UI_LIVE_HOME") or fail("run me under tools/live/run")
    switch = os.environ.get("OMARCHY_UI_LIVE_SWITCH") or fail("no OMARCHY_UI_LIVE_SWITCH")
    reduced = os.environ.get("OMARCHY_UI_REDUCE_MOTION") == "1"
    app = Live()
    async with app.run_test(size=(8, 2), notifications=False) as pilot:
        await asyncio.sleep(0.3)
        if subprocess.run([switch, home, "catppuccin-latte"]).returncode != 0:
            fail(f"{switch} failed")
        switched = time.monotonic()
        landed = None
        while True:
            await asyncio.sleep(0.01)
            t = app.kit.theme
            if landed is None and t.tokens.palette.background.hex() == LATTE and not t.fading:
                landed = time.monotonic()
            if landed is not None and time.monotonic() - landed > 0.6:
                break
            if time.monotonic() - switched > 5:
                fail(f"background is {t.tokens.palette.background.hex()} after 5 s, not {LATTE}")
        await pilot.pause()
        # The last frame Textual composed: the accent cell holds the new accent exactly.
        seg = next(s for s in app.screen._compositor.render_strips()[0] if s.text.strip())
        c = seg.style.color.triplet
        accent = app.kit.theme.tokens.palette.accent
        if (c.red, c.green, c.blue) != (accent.r, accent.g, accent.b):
            fail(f"the frame shows {c.hex}, not the new accent {accent.hex()}")
    t = app.kit.theme
    if t.shown.palette.background != t.tokens.palette.background:
        fail("the fade didn't land exactly")
    if app.hooks != [LATTE]:
        fail(f"changed hook fired {app.hooks}, want once with {LATTE}")
    if app.revisions != [1] or t.revision != 1:
        fail(f"revision {t.revision} (hook saw {app.revisions}), want 1")
    if app.on_loop != [True]:
        fail("the hook didn't run on the app's loop")
    if reduced and (app.faded or app.between):
        fail("a fade ran under reduced motion")
    if not reduced and not (app.faded and app.between):
        fail(f"no fade seen (fading {app.faded}, a color between {app.between})")
    ms = int((landed - switched) * 1000)
    kind = "no fade (reduced motion)" if reduced else "faded and landed exactly"
    print(f"live switch: {LATTE} in {ms} ms, hook once, revision 1, {kind}")


if __name__ == "__main__":
    asyncio.run(main())
