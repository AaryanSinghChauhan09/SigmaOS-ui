"""What the tests share: fixture themes (read from the core, as the app does), and the gallery
run headless on one section (Textual's Pilot drives keys and clicks)."""

from __future__ import annotations

import os
import subprocess
from contextlib import asynccontextmanager
from functools import lru_cache
from pathlib import Path

from omarchy_textual import Depth, Theme, Tokens
from omarchy_textual.core import find_theme_bin, read_state

ROOT = Path(__file__).resolve().parents[3]
FIXTURES = ROOT / "fixtures/themes"


@lru_cache(maxsize=64)
def fixture_state(name: str, radius: int | None = None) -> dict:
    """A fixture theme's state, resolved by the core (`OMARCHY_UI_THEME_DIR`), reduced motion."""
    env = dict(os.environ)
    env["OMARCHY_UI_THEME_DIR"] = str(FIXTURES / name)
    env["OMARCHY_UI_REDUCE_MOTION"] = "1"
    if radius is not None:
        env["OMARCHY_UI_ROUNDING"] = str(radius)
    else:
        env.pop("OMARCHY_UI_ROUNDING", None)
    import json

    out = subprocess.run([find_theme_bin(), "--state"], env=env, check=True, capture_output=True, text=True).stdout
    return json.loads(out)


def fixture(name: str = "tokyo-night", radius: int | None = None) -> Theme:
    """A truecolor `Theme` pinned to a fixture."""
    return Theme(Tokens(fixture_state(name, radius)), depth=Depth.TRUE)


def fixtures() -> list[str]:
    return sorted(p.name for p in FIXTURES.iterdir() if p.is_dir())


@asynccontextmanager
async def gallery(section: str, theme: str = "tokyo-night", size: tuple[int, int] = (96, 60)):
    """The gallery on `section` (alone, as `--snap` shows it), headless: yields (app, pilot);
    `app.seen` lists every message delivered."""
    from omarchy_textual.gallery.app import GalleryApp
    from omarchy_textual import kit_of

    app = GalleryApp(section=section, snap=True, theme=fixture(theme))
    kit_of(app).live = False
    #: Every message delivered, in order (to check what a control posted).
    app.seen = []
    async with app.run_test(size=size, notifications=False, message_hook=app.seen.append) as pilot:
        await pilot.pause()
        await pilot.pause()
        yield app, pilot


def snap_text(app, cols: int = 96, rows: int | None = None) -> str:
    """The app's screen now, as a canonical snapshot (rows: the section's natural height)."""
    from omarchy_textual.snapshot import grid_from_strips, write_canonical

    if rows is None:
        rows = app.query_one("#section").get_content_height(None, None, cols)
    return write_canonical(grid_from_strips(app.screen._compositor.render_strips(), cols, rows))


def posted(app, kind) -> list:
    """The messages of type `kind` delivered so far (once each: a bubbling message is seen at every
    stop, counted at its first)."""
    out = []
    for m in app.seen:
        if isinstance(m, kind) and not any(m is o for o in out):
            out.append(m)
    return out
