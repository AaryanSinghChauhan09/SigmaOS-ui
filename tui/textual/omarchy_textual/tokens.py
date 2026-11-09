"""The core's tokens (spec/tokens.md) as Python reads them: the `omarchy-theme --state` line,
wrapped for attribute access (`tokens.palette.accent`, `tokens.controls.hover.fill`), colors as
`Rgba`. Nothing is resolved here: every value is the core's. The fade's arithmetic
(`mix_tokens`, `cubic_bezier`) is what spec/following.md asks every binding to do.
"""

from __future__ import annotations

import re
from typing import Any

from .color import Rgba

_HEX = re.compile(r"^#[0-9a-fA-F]{8}$")


def _camel(name: str) -> str:
    head, *rest = name.split("_")
    return head + "".join(p[:1].upper() + p[1:] for p in rest)


def _value(v: Any) -> Any:
    if isinstance(v, dict):
        return Node(v)
    if isinstance(v, str) and _HEX.match(v):
        return Rgba.parse(v)
    if isinstance(v, list):
        return [_value(x) for x in v]
    return v


class Node:
    """A JSON object of the tokens, read by attribute: snake_case names map to the camelCase keys
    (`text_dim` is `textDim`); `#rrggbbaa` strings come back as `Rgba`."""

    __slots__ = ("_raw", "_cache")

    def __init__(self, raw: dict):
        self._raw = raw
        self._cache: dict[str, Any] = {}

    def __getattr__(self, name: str) -> Any:
        cache = self._cache
        if name in cache:
            return cache[name]
        raw = self._raw
        key = name if name in raw else _camel(name)
        if key not in raw:
            raise AttributeError(name)
        v = cache[name] = _value(raw[key])
        return v

    def __getitem__(self, key: str) -> Any:
        return _value(self._raw[key])

    def get(self, key: str, default: Any = None) -> Any:
        return _value(self._raw[key]) if key in self._raw else default

    def is_none(self) -> bool:
        """A border spec with no width (`BorderSpec::is_none`)."""
        widths = self._raw.get("widths") or [0]
        return max(widths) <= 0

    @property
    def raw(self) -> dict:
        return self._raw

    def __repr__(self) -> str:
        return f"Node({self._raw!r})"


class Tokens:
    """The tokens in one state line: `palette`, `roles`, `controls`, `surfaces`, `font`,
    `spacing`, `motion`, `radius`, `reduce_motion`, `mode`; `palette.named` holds the theme's
    named colors (`red`, `orange`, ...); `string(key, fallback)` the kit's strings in the display
    language."""

    def __init__(self, state: dict):
        self.state = state
        raw = state["tokens"]
        self._root = Node(raw)
        self.named: dict[str, Rgba] = {k: Rgba.parse(v) for k, v in (state.get("colors") or {}).items()}

    def __getattr__(self, name: str) -> Any:
        if name.startswith("_"):
            raise AttributeError(name)
        if name == "palette":
            p = self._root.palette
            p._cache["named"] = self.named
            return p
        return getattr(self._root, name)

    @property
    def raw(self) -> dict:
        """The canonical token JSON (the core's `--compact` object)."""
        return self.state["tokens"]

    @property
    def language(self) -> str:
        return self.state.get("language") or "en"

    @property
    def theme_name(self) -> str:
        return self.state.get("theme") or ""

    def string(self, key: str, fallback: str) -> str:
        return (self.state.get("strings") or {}).get(key, fallback)

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Tokens):
            return NotImplemented
        a, b = self.state, other.state
        return all(a.get(k) == b.get(k) for k in ("tokens", "colors", "strings", "language", "iconTheme", "theme"))

    def __hash__(self) -> int:  # pragma: no cover - identity is enough
        return id(self)


def _mix_value(a: Any, b: Any, p: float) -> Any:
    if isinstance(b, str) and _HEX.match(b):
        if isinstance(a, str) and _HEX.match(a):
            return Rgba.parse(a).mix(Rgba.parse(b), p).hex()
        return b
    if isinstance(b, dict):
        if not isinstance(a, dict):
            return b
        return {k: (_mix_value(a[k], v, p) if k in a else v) for k, v in b.items()}
    if isinstance(b, list):
        if isinstance(a, list) and len(a) == len(b):
            return [_mix_value(x, y, p) for x, y in zip(a, b)]
        return b
    return b


def mix_tokens(a: Tokens, b: Tokens, p: float) -> Tokens:
    """`b` with every color `p` of the way from `a`'s (0 is `a`'s colors, 1 is exactly `b`):
    the palette, its named colors, roles, control states and surfaces, per channel in sRGB, alpha
    included. Everything that isn't a color is `b`'s."""
    if p >= 1.0:
        return b
    state = dict(b.state)
    state["tokens"] = _mix_value(a.state["tokens"], b.state["tokens"], p)
    state["colors"] = _mix_value(a.state.get("colors") or {}, b.state.get("colors") or {}, p)
    return Tokens(state)


def cubic_bezier(c: list[float] | tuple[float, ...], x: float) -> float:
    """`cubic-bezier(x1, y1, x2, y2)` at time `x`: Newton steps with a bisection fallback, as
    browsers (and every binding's fade) solve it."""
    x = min(1.0, max(0.0, x))
    if x == 0.0 or x == 1.0:
        return x
    x1, y1, x2, y2 = c

    def coord(t: float, a: float, b: float) -> float:
        u = 1.0 - t
        return 3.0 * u * u * t * a + 3.0 * u * t * t * b + t * t * t

    def slope(t: float, a: float, b: float) -> float:
        u = 1.0 - t
        return 3.0 * u * u * a + 6.0 * u * t * (b - a) + 3.0 * t * t * (1.0 - b)

    t = x
    for _ in range(8):
        err = coord(t, x1, x2) - x
        if abs(err) < 1e-7:
            return coord(t, y1, y2)
        d = slope(t, x1, x2)
        if abs(d) < 1e-6:
            break
        t -= err / d
    lo, hi = 0.0, 1.0
    t = x
    for _ in range(40):
        v = coord(t, x1, x2)
        if abs(v - x) < 1e-7:
            break
        if v < x:
            lo = t
        else:
            hi = t
        t = (lo + hi) / 2.0
    return coord(t, y1, y2)
