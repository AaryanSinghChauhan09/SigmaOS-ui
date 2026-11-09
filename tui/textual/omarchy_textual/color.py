"""Colors as the core resolves them: 8-bit sRGB with straight alpha, and the arithmetic a terminal
binding does on them (spec/terminal.md): compositing (`over`), the core's `mix`, `with_alpha` and
`times_alpha`, and Qt's `darker` (ported step for step from `crates/omarchy-theme/src/color.rs`,
single-precision floats included, so halves round exactly as the core rounds them).
"""

from __future__ import annotations

import math
import struct
from typing import NamedTuple

_F32 = struct.Struct("f")


def f32(x: float) -> float:
    """`x` rounded to an IEEE single, as Rust's `f32` holds it."""
    return _F32.unpack(_F32.pack(x))[0]


def rround(x: float) -> int:
    """Rust's `round()`: half away from zero."""
    if x < 0:
        return -rround(-x)
    r = math.floor(x)
    return int(r) + 1 if x - r >= 0.5 else int(r)


def _alpha_byte(alpha: float) -> int:
    if not math.isfinite(alpha):
        alpha = 0.0
    return rround(min(1.0, max(0.0, alpha)) * 255.0)


class Rgba(NamedTuple):
    """An 8-bit sRGB color with straight (not premultiplied) alpha."""

    r: int
    g: int
    b: int
    a: int = 255

    @staticmethod
    def parse(s: str) -> "Rgba":
        """`#rrggbb` or `#rrggbbaa` (the canonical token form)."""
        h = s.lstrip("#")
        if len(h) == 6:
            h += "ff"
        if len(h) != 8:
            raise ValueError(f"not a color: {s!r}")
        return Rgba(int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), int(h[6:8], 16))

    def hex(self) -> str:
        """`#rrggbbaa`, lowercase."""
        return f"#{self.r:02x}{self.g:02x}{self.b:02x}{self.a:02x}"

    def hex6(self) -> str:
        return f"#{self.r:02x}{self.g:02x}{self.b:02x}"

    def opaque(self) -> "Rgba":
        return Rgba(self.r, self.g, self.b, 255)

    def with_alpha(self, alpha: float) -> "Rgba":
        """The same color at `alpha` (0–1), replacing its own."""
        return Rgba(self.r, self.g, self.b, _alpha_byte(alpha))

    def times_alpha(self, alpha: float) -> "Rgba":
        """Its own alpha multiplied by `alpha` (0–1)."""
        return Rgba(self.r, self.g, self.b, _alpha_byte(self.a / 255.0 * alpha))

    def mix(self, other: "Rgba", t: float) -> "Rgba":
        """Toward `other` by `t` (0 keeps self, 1 is other), per channel in sRGB, alpha included."""
        t = min(1.0, max(0.0, t))

        def m(a: int, b: int) -> int:
            return rround(a + (b - a) * t)

        return Rgba(m(self.r, other.r), m(self.g, other.g), m(self.b, other.b), m(self.a, other.a))

    def darker(self, factor: float) -> "Rgba":
        """Qt's `QColor::darker(round(factor × 100))`, exactly as the core computes it."""
        factor_i = rround(factor * 100.0)
        if factor_i < 100:
            return self
        MAX = 65535.0

        def q_round(d: float) -> int:
            return int(f32(d + 0.5)) if d >= 0.0 else int(f32(d - 0.5))

        def fuzzy_eq(a: float, b: float) -> bool:
            return f32(abs(f32(a - b)) * 100000.0) <= min(abs(a), abs(b))

        r = f32(f32(self.r * 257.0) / MAX)
        g = f32(f32(self.g * 257.0) / MAX)
        b = f32(f32(self.b * 257.0) / MAX)
        mx = max(r, g, b)
        mn = min(r, g, b)
        delta = f32(mx - mn)
        value = q_round(f32(mx * MAX))
        if abs(delta) <= f32(0.00001):
            hue, sat = 65535, 0
        else:
            sat = q_round(f32(f32(delta / mx) * MAX))
            if fuzzy_eq(r, mx):
                h = f32(f32(g - b) / delta)
            elif fuzzy_eq(g, mx):
                h = f32(2.0 + f32(f32(b - r) / delta))
            else:
                h = f32(4.0 + f32(f32(r - g) / delta))
            h = f32(h * 60.0)
            if h < 0.0:
                h = f32(h + 360.0)
            hue = q_round(f32(h * 100.0))
        v16 = (value * 100) // factor_i
        hh = 0.0 if hue == 65535 else f32(hue / 6000.0)
        s = f32(sat / MAX)
        v = f32(v16 / MAX)
        if s == 0.0:
            rr, gg, bb = v, v, v
        else:
            i = int(hh)
            f = f32(hh - i)
            p = f32(v * f32(1.0 - s))
            if i & 1:
                q = f32(v * f32(1.0 - f32(s * f)))
                rr, gg, bb = {1: (q, v, p), 3: (p, q, v)}.get(i, (v, p, q))
            else:
                t = f32(v * f32(1.0 - f32(s * f32(1.0 - f))))
                rr, gg, bb = {0: (v, t, p), 2: (p, v, t)}.get(i, (t, p, v))

        def byte(c: float) -> int:
            c16 = min(65535, max(0, q_round(f32(c * MAX))))
            return rround(c16 / 65535.0 * 255.0)

        return Rgba(byte(rr), byte(gg), byte(bb), self.a)


TRANSPARENT = Rgba(0, 0, 0, 0)


def over(top: Rgba, under: Rgba) -> Rgba:
    """`top` composited over the opaque `under`, per 8-bit channel:
    `(t·a + u·(255 − a) + 127) / 255`, the core's `mix` by `a / 255` exactly."""
    a = top.a
    if a == 255:
        return top
    na = 255 - a
    return Rgba(
        (top.r * a + under.r * na + 127) // 255,
        (top.g * a + under.g * na + 127) // 255,
        (top.b * a + under.b * na + 127) // 255,
        255,
    )
