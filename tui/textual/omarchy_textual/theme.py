"""The live Omarchy theme for a terminal app (spec/following.md, spec/terminal.md).

`Theme` holds the tokens in effect (the target of any fade), what is on screen (the same tokens
with colors part-way through a theme fade), the revision, the color depth, the animation clock
and the hit areas components record. Components draw from `Theme.shown`; an app's own drawing that
should fade with them reads colors there too.
"""

from __future__ import annotations

import time

from .cells.buffer import DIM, Buffer, Rect
from .color import Rgba
from .depth import Depth, nearest256, nearest_role, tables
from .tokens import Tokens, cubic_bezier, mix_tokens


class Theme:
    def __init__(self, tokens: Tokens, depth: Depth | None = None):
        self._tokens = tokens
        self._shown = tokens
        self._revision = 0
        self.depth = Depth.detect() if depth is None else depth
        self._fade: tuple[Tokens, float, float] | None = None
        self._started = time.monotonic()
        self._clock: int | None = None
        self.hits: list[tuple[object, Rect]] = []

    @staticmethod
    def load() -> "Theme":
        """The theme as it is now (or the `OMARCHY_UI_THEME_DIR` fixture); depth from the
        environment. `follow(app)` (or `OmarchyApp`) keeps it live."""
        from .core import load

        return Theme(load())

    # Changes ------------------------------------------------------------------------------

    def set_tokens(self, fresh: Tokens) -> bool:
        """Apply a change: the new tokens take effect at once, colors start fading (truecolor,
        no reduced motion), the revision counts it. Tokens equal to the current ones change
        nothing and return False."""
        if fresh == self._tokens:
            return False
        frm = self._shown
        self._tokens = fresh
        self._revision += 1
        if fresh.reduce_motion or fresh.motion.theme == 0 or self.depth != Depth.TRUE:
            self._shown = fresh
            self._fade = None
        else:
            self._shown = mix_tokens(frm, fresh, 0.0)
            self._fade = (frm, time.monotonic(), fresh.motion.theme / 1000.0)
        return True

    def tick(self) -> bool:
        """Advance the fade to now. True while it is still running (draw again soon)."""
        if self._fade is None:
            return False
        frm, start, duration = self._fade
        x = (time.monotonic() - start) / duration
        if x >= 1.0:
            self._shown = self._tokens
            self._fade = None
            return False
        p = cubic_bezier(self._tokens.motion.ease, x)
        self._shown = mix_tokens(frm, self._tokens, p)
        return True

    @property
    def fading(self) -> bool:
        return self._fade is not None

    @property
    def tokens(self) -> Tokens:
        """The tokens in effect: after a change, the new ones (a fade's target, never a frame of it)."""
        return self._tokens

    @property
    def shown(self) -> Tokens:
        """What is on screen: `tokens` with colors where the fade has them now."""
        return self._shown

    @property
    def revision(self) -> int:
        """0 for the tokens the app started with, plus one per change: a cache key."""
        return self._revision

    def string(self, key: str, fallback: str) -> str:
        return self._tokens.string(key, fallback)

    # Colors, from what is on screen ------------------------------------------------------------

    def fg(self) -> Rgba:
        return self._shown.palette.foreground

    def bg(self) -> Rgba:
        return self._shown.palette.background

    def accent(self) -> Rgba:
        return self._shown.palette.accent

    def urgent(self) -> Rgba:
        return self._shown.palette.urgent

    def success(self) -> Rgba:
        return self._shown.palette.success

    def warning(self) -> Rgba:
        return self._shown.palette.warning

    def muted(self) -> Rgba:
        return self._shown.palette.muted

    def dim(self) -> Rgba:
        """Secondary text (`roles.textDim`)."""
        return self._shown.roles.text_dim

    def faint(self) -> Rgba:
        """Disabled and placeholder text (`roles.textFaint`)."""
        return self._shown.roles.text_faint

    def track(self) -> Rgba:
        """Tracks, rules and separators (`roles.track`)."""
        return self._shown.roles.track

    def hot_accent(self) -> Rgba:
        """The accent with a quarter of the foreground mixed in."""
        return self.accent().mix(self.fg(), 0.25)

    def radius(self) -> int:
        return self._shown.radius

    def rounded(self) -> bool:
        return self._shown.radius > 0

    def reduce_motion(self) -> bool:
        return bool(self._shown.reduce_motion)

    # The animation clock -------------------------------------------------------------------------

    def clock(self) -> int:
        """Milliseconds since the theme was made, for spinners; pinned by `set_clock` in tests."""
        if self._clock is not None:
            return self._clock
        return int((time.monotonic() - self._started) * 1000)

    def set_clock(self, ms: int | None) -> None:
        self._clock = ms

    # Hit areas ---------------------------------------------------------------------------------

    def hit(self, key: object, area: Rect) -> None:
        """Record that `key` was drawn at `area` this frame."""
        if area.width > 0 and area.height > 0:
            self.hits.append((key, area))

    def hit_at(self, x: int, y: int) -> object | None:
        """The topmost key drawn at a cell this frame."""
        for k, r in reversed(self.hits):
            if r.contains(x, y):
                return k
        return None

    def find(self, key: object) -> Rect | None:
        for k, r in reversed(self.hits):
            if k == key:
                return r
        return None

    def clear_hits(self) -> None:
        self.hits = []

    # The last step of every frame ----------------------------------------------------------------

    def finish(self, buf: Buffer) -> None:
        """Cells whose background is the theme's background are written as the terminal's default
        (so terminal opacity shows), and below truecolor every color maps down (spec/terminal.md,
        Color)."""
        bg = self.bg().opaque()
        fg = self.fg().opaque()
        depth = self.depth
        ink_t = fill_t = None
        if depth == Depth.ANSI16:
            ink_t, fill_t = tables(self._shown)
        for cell in buf.cells:
            if cell.bg == bg:
                cell.bg = None
            if depth == Depth.TRUE:
                continue
            if depth == Depth.ANSI256:
                if cell.bg is not None:
                    c = cell.bg
                    cell.bg = nearest256((c.r, c.g, c.b))
                if cell.fg is not None:
                    c = cell.fg
                    cell.fg = None if c == fg else nearest256((c.r, c.g, c.b))
            else:
                if cell.bg is not None:
                    c = cell.bg
                    cell.bg = nearest_role((c.r, c.g, c.b), fill_t).index
                if cell.fg is not None:
                    c = cell.fg
                    role = nearest_role((c.r, c.g, c.b), ink_t)
                    cell.fg = role.index
                    if role.faint:
                        cell.mod |= DIM
                if cell.fg == cell.bg and cell.fg is not None:
                    cell.fg = None
