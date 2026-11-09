"""The core, from Python: find the `omarchy-theme` CLI, read its state once, and follow
`omarchy-theme --watch --state` (one JSON line per change, the first the initial state) on the
asyncio loop. Never resolves a token itself.
"""

from __future__ import annotations

import asyncio
import json
import os
import shutil
import subprocess
from pathlib import Path
from typing import Awaitable, Callable

from .tokens import Tokens

#: This checkout's root (tui/textual/omarchy_textual/../../..), for the repository build of the CLI.
REPO = Path(__file__).resolve().parents[3]


def find_theme_bin(env: dict[str, str] | None = None) -> str:
    """The `omarchy-theme` CLI: `$OMARCHY_THEME_BIN`, else on `PATH`, else this checkout's
    `target/release/omarchy-theme`."""
    env = os.environ if env is None else env
    if env.get("OMARCHY_THEME_BIN"):
        return env["OMARCHY_THEME_BIN"]
    found = shutil.which("omarchy-theme", path=env.get("PATH"))
    if found:
        return found
    for p in (REPO / "target/release/omarchy-theme", REPO / "target/debug/omarchy-theme"):
        if p.exists():
            return str(p)
    raise FileNotFoundError(
        "omarchy-theme not found: install the omarchy-ui package, put it on PATH, or set OMARCHY_THEME_BIN"
    )


def read_state(bin: str | None = None) -> dict:
    """The state now, once: `{schema, revision, theme, iconTheme, language, tokens, colors, strings}`."""
    out = subprocess.run([bin or find_theme_bin(), "--state"], check=True, capture_output=True, text=True).stdout
    return json.loads(out)


def load() -> Tokens:
    """The tokens now (or the `OMARCHY_UI_THEME_DIR` fixture, under the `OMARCHY_UI_*` overrides)."""
    return Tokens(read_state())


async def watch(on_tokens: Callable[[Tokens, bool], Awaitable[None] | None], bin: str | None = None) -> None:
    """Follow `omarchy-theme --watch --state` until cancelled: `on_tokens(tokens, is_change)` for
    the first line (not a change) and every later one. The core coalesces (150 ms) and drops
    lines that change nothing."""
    proc = await asyncio.create_subprocess_exec(
        bin or find_theme_bin(),
        "--watch",
        "--state",
        stdin=asyncio.subprocess.DEVNULL,
        stdout=asyncio.subprocess.PIPE,
        limit=16 << 20,
    )
    first = True
    try:
        assert proc.stdout is not None
        while True:
            line = await proc.stdout.readline()
            if not line:
                break
            if not line.strip():
                continue
            try:
                state = json.loads(line)
            except ValueError:
                continue
            r = on_tokens(Tokens(state), not first)
            if asyncio.iscoroutine(r):
                await r
            first = False
    finally:
        if proc.returncode is None:
            proc.kill()
            try:
                await proc.wait()
            except Exception:  # pragma: no cover - the loop is going away
                pass
