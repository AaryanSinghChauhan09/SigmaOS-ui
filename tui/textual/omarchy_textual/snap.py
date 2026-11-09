"""`--snap out.ans [--size COLSxROWS]` for any Textual app (spec/terminal.md, Snapshots): the app
runs headless at that size, and what Textual's compositor put on the screen is written as the
canonical ANSI snapshot (24-bit, the theme background as the terminal's default).

    from omarchy_textual.snap import from_args

    if __name__ == "__main__":
        app = MyApp()
        if not from_args(app):
            app.run()
"""

from __future__ import annotations

import asyncio
import sys
from typing import Callable

from textual.app import App

from .app import kit_of
from .depth import Depth
from .snapshot import grid_from_strips, write_canonical

#: Rows to lay out in when the snapshot takes the app's natural height.
TALL = 240


def parse_size(s: str | None, cols: int = 96) -> tuple[int, int]:
    """`COLSxROWS` (rows 0: the natural height); width defaults to 96 columns."""
    if not s:
        return (cols, 0)
    c, _, r = s.partition("x")
    return (int(c), int(r or 0))


def snap_app(app: App, cols: int, rows: int, natural: Callable[[App], int] | None = None) -> str:
    """Run `app` headless at `cols` × `rows` (rows 0: its natural height, `natural(app)`, else the
    height of what the screen shows) and return the canonical snapshot of its screen."""
    kit = kit_of(app)
    kit.live = False
    kit.theme.depth = Depth.TRUE
    kit.install()

    async def run() -> str:
        async with app.run_test(size=(cols, rows or TALL), notifications=False) as pilot:
            for _ in range(3):
                await pilot.pause()
            h = rows
            if h == 0:
                if natural is not None:
                    h = natural(app)
                else:
                    h = max((w.region.bottom for w in app.screen.children if w.display), default=1)
                h = max(1, h)
            strips = app.screen._compositor.render_strips()
            return write_canonical(grid_from_strips(strips, cols, h))

    return asyncio.run(run())


def from_args(app: App, argv: list[str] | None = None) -> bool:
    """`--snap out.ans [--size COLSxROWS]` on the command line: writes the snapshot and returns
    True (the app then exits instead of running)."""
    args = sys.argv[1:] if argv is None else argv
    if "--snap" not in args:
        return False
    i = args.index("--snap")
    out = args[i + 1]
    size = args[args.index("--size") + 1] if "--size" in args else None
    cols, rows = parse_size(size)
    with open(out, "w", encoding="utf-8") as f:
        f.write(snap_app(app, cols, rows))
    return True
