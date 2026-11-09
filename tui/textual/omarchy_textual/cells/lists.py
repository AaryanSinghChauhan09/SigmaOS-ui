"""Rows, the keyboard cursor that walks them, the scroll view they sit in, and the small pieces rows
and grids are made of (spec/terminal.md, Lists). A port of `omarchy-ratatui`'s `ui/lists.rs`, cell
for cell."""

from __future__ import annotations

import enum
import struct
import zlib
from pathlib import Path
from typing import Iterable, NamedTuple

from ..color import TRANSPARENT, Rgba, over
from . import paint
from .base import Draw, Tone, border_of, look
from .buffer import Buffer, Rect
from .controls import wrap
from .paint import Ink, edges, fill, put, text, text_center, text_fit, text_right, width

__all__ = [
    "Avatar",
    "AvatarImage",
    "AvatarSize",
    "Badge",
    "CursorKey",
    "CursorModel",
    "CursorSection",
    "Hero",
    "KeyValue",
    "ListRow",
    "ScrollArea",
    "StatusDot",
    "Tile",
    "TileGrid",
    "printable",
]


# CursorModel -----------------------------------------------------------------------------------


class CursorSection(NamedTuple):
    name: str
    count: int


class CursorKey(enum.Enum):
    """What a key did to a `CursorModel` (`TEXT` carries the text in `CursorModel.typed`)."""

    MOVED = "moved"
    ACTIVATE = "activate"
    CLOSE = "close"
    DELETE = "delete"
    TEXT = "text"
    IGNORED = "ignored"


def _sign(n: int) -> int:
    return (n > 0) - (n < 0)


class CursorModel:
    """The keyboard cursor every panel re-implements: a section and a row within it, and whether it
    shows. The same keys as the GUI bindings' (spec/components/lists.md); the pointer moves it only
    when it reaches another cell."""

    def __init__(self, sections: Iterable[tuple[str, int]], columns: int = 1):
        self.sections = [CursorSection(n, c) for n, c in sections]
        self._section = 0
        self._index = 0
        self.active = False
        self.columns = max(1, columns)
        self.page = 5
        self._gate: tuple[int, int] | None = None
        self._initial_sample = False
        #: The text the last `apply_key` typed (CursorKey.TEXT).
        self.typed = ""
        self._settle()

    @staticmethod
    def list(count: int) -> "CursorModel":
        """One unnamed section of `count` rows."""
        return CursorModel([("", count)])

    def section(self) -> str:
        return self.sections[self._section].name if self._section < len(self.sections) else ""

    def index(self) -> int:
        return self._index

    def is_at(self, section: str, index: int) -> bool:
        return self.active and self.section() == section and self._index == index

    def cursor_in(self, section: str) -> int | None:
        return self._index if self.active and self.section() == section else None

    def set_count(self, section: str, count: int) -> None:
        for i, s in enumerate(self.sections):
            if s.name == section:
                self.sections[i] = CursorSection(s.name, count)
                break
        self._settle()

    def set(self, section: str, index: int) -> None:
        for i, s in enumerate(self.sections):
            if s.name == section:
                self._section = i
                self._index = index
                self.active = True
                self._settle()
                return

    def reset(self) -> None:
        self.active = False
        self._gate = None
        self._initial_sample = False

    def move_by(self, dx: int, dy: int) -> bool:
        if not self._wake():
            return True
        before = (self._section, self._index)
        if dx != 0 and self.columns > 1:
            col = self._index % self.columns
            to = col + dx
            if 0 <= to < self.columns:
                nxt = self._index + dx
                if 0 <= nxt < self._count():
                    self._index = nxt
        if dy != 0:
            step = self.columns
            for _ in range(abs(dy)):
                nxt = self._index + step * _sign(dy)
                if 0 <= nxt < self._count():
                    self._index = nxt
                elif dy > 0:
                    s = self._neighbour(1)
                    if s is not None:
                        self._section = s
                        self._index = min(self._index % self.columns, max(0, self._count() - 1))
                    elif self.columns > 1 and self._index // self.columns < max(0, self._count() - 1) // self.columns:
                        self._index = self._count() - 1
                else:
                    s = self._neighbour(-1)
                    if s is not None:
                        self._section = s
                        count = self._count()
                        last_line = (max(0, count - 1) // self.columns) * self.columns
                        self._index = min(last_line + self._index % self.columns, max(0, count - 1))
        return (self._section, self._index) != before

    def move_section(self, delta: int) -> bool:
        if not self._wake():
            return True
        s = self._neighbour(_sign(delta))
        if s is None:
            return False
        self._section = s
        self._index = 0
        return True

    def page_by(self, delta: int) -> bool:
        if not self._wake():
            return True
        before = self._index
        last = max(0, self._count() - 1)
        step = max(1, self.page) * self.columns
        self._index = min(last, max(0, self._index + step * _sign(delta)))
        return self._index != before

    def home(self) -> bool:
        self.active = True
        before = (self._section, self._index)
        for i, s in enumerate(self.sections):
            if s.count > 0:
                self._section, self._index = i, 0
                break
        return (self._section, self._index) != before

    def end(self) -> bool:
        self.active = True
        before = (self._section, self._index)
        for i in range(len(self.sections) - 1, -1, -1):
            if self.sections[i].count > 0:
                self._section, self._index = i, self.sections[i].count - 1
                break
        return (self._section, self._index) != before

    def apply_key(self, key: str, character: str | None = None) -> CursorKey:
        """A Textual key name (`down`, `j`, `pagedown`, `enter`, ...): Down/j, Up/k, Right/l,
        Left/h, Page Up/Down, Home/End, Tab/Shift+Tab (sections), Return and Space (activate),
        Escape (close), Delete and x (delete), any other printable key (text)."""
        moves = {
            "down": lambda: self.move_by(0, 1),
            "j": lambda: self.move_by(0, 1),
            "up": lambda: self.move_by(0, -1),
            "k": lambda: self.move_by(0, -1),
            "right": lambda: self.move_by(1, 0),
            "l": lambda: self.move_by(1, 0),
            "left": lambda: self.move_by(-1, 0),
            "h": lambda: self.move_by(-1, 0),
            "pagedown": lambda: self.page_by(1),
            "pageup": lambda: self.page_by(-1),
            "home": self.home,
            "end": self.end,
            "tab": lambda: self.move_section(1),
            "shift+tab": lambda: self.move_section(-1),
        }
        if key in moves:
            moves[key]()
            return CursorKey.MOVED
        if key in ("enter", "space"):
            return CursorKey.ACTIVATE
        if key == "escape":
            return CursorKey.CLOSE
        if key in ("delete", "x", "X", "shift+x"):
            return CursorKey.DELETE
        p = printable(key, character)
        if p is not None:
            self.typed = p
            return CursorKey.TEXT
        return CursorKey.IGNORED

    def pointer_moved(self, col: int, row: int, section: str, index: int) -> bool:
        """A pointer sample over row `index` of `section` at a cell: moves the cursor only when the
        pointer reached another cell since the last accepted sample (a keyboard move resets the
        gate; the next sample only primes it)."""
        if self._gate is not None:
            moved = self._gate != (col, row)
        else:
            moved = self._initial_sample
        if self._gate is None or moved:
            self._gate = (col, row)
        self._initial_sample = False
        if moved:
            self.set(section, index)
        return moved

    def reset_pointer(self) -> None:
        self._gate = None
        self._initial_sample = False

    def allow_initial_sample(self) -> None:
        self._gate = None
        self._initial_sample = True

    def _count(self) -> int:
        return self.sections[self._section].count if self._section < len(self.sections) else 0

    def _wake(self) -> bool:
        self.reset_pointer()
        if self.active:
            return True
        self.active = True
        self._settle()
        return False

    def _neighbour(self, d: int) -> int | None:
        i = self._section + d
        while 0 <= i < len(self.sections):
            if self.sections[i].count > 0:
                return i
            i += d
        return None

    def _settle(self) -> None:
        if self._count() == 0:
            s = self._neighbour(1)
            if s is None:
                s = self._neighbour(-1)
            if s is not None:
                self._section, self._index = s, 0
        self._index = min(self._index, max(0, self._count() - 1))


def printable(key: str, character: str | None = None) -> str | None:
    """The text a key types (type-ahead filters), or None for a non-printable key or one with
    Ctrl, Alt or Super."""
    if any(key.startswith(m) for m in ("ctrl+", "alt+", "super+", "meta+")):
        return None
    ch = character
    if ch is None and len(key) == 1:
        ch = key
    if ch is None or len(ch) != 1:
        return None
    if ch.isspace() or ord(ch) < 32 or 0x7F <= ord(ch) < 0xA0:
        return None
    return ch


# ListRow ---------------------------------------------------------------------------------------


class ListRow(Draw):
    """One row of a keyboard-and-mouse list, one line:
    `[edge][pad][glyph ]Title  subtitle ··· value [trailing] [end][pad][edge]`. The cursor row
    takes the hover fill and edges, the current row the selected fill. The row keyed `key`, its
    action `(key, "action")`."""

    def __init__(self, title: str, glyph: str = "", glyph_color: Rgba | None = None, leading: Draw | None = None,
                 subtitle: str = "", status: tuple[str, Tone] | None = None, value: str = "",
                 trailing: Draw | None = None, trailing_glyph: tuple[str, Tone] | None = None,
                 action: tuple[str, bool] | None = None, action_cursor: bool = False,
                 expansion: Draw | None = None, cursor: bool = False, current: bool = False,
                 disabled: bool = False, width: int = 53, key: object = None):
        self.title, self.glyph, self.glyph_color, self.leading = title, glyph, glyph_color, leading
        self.subtitle, self.status, self.value, self.trailing = subtitle, status, value, trailing
        self.trailing_glyph, self.action, self.action_cursor = trailing_glyph, action, action_cursor
        self.expansion, self.cursor, self.current, self.disabled = expansion, cursor, current, disabled
        self.width, self.key = width, key

    def size(self, t) -> tuple[int, int]:
        return (self.width, 1 + (self.expansion.size(t)[1] if self.expansion is not None else 0))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        hot = self.cursor and not self.disabled
        if hot:
            fill_c, border = c.hover.fill, border_of(c.hover.border)
        elif self.current and not self.disabled:
            fill_c, border = c.selected.fill, border_of(c.selected.border)
        else:
            fill_c, border = TRANSPARENT, None
        # The whole row first, so the action and anything keyed in the row lie over it.
        if self.key is not None and not self.disabled:
            t.hit(self.key, area.replace(height=1))
        fill(t, buf, area, fill_c)
        for dy in range(area.height):
            edges(t, buf, area.x, area.y + dy, area.width, border)
        y = area.y
        faint = t.faint() if self.disabled else None
        x = area.x + 2
        if self.leading is not None:
            lw, _ = self.leading.size(t)
            self.leading.draw(t, Rect(x, y, lw, 1), buf)
            x += lw + 1
        elif self.glyph:
            ink = faint if faint is not None else (self.glyph_color if self.glyph_color is not None else t.fg())
            put(t, buf, x, y, self.glyph, ink)
            x += 3
        right = max(0, area.right - 2)
        show_action = self.action is not None and not self.disabled and (self.cursor or self.action_cursor)
        if self.trailing_glyph is not None or self.action is not None:
            right = max(0, right - 1)
            ex = right
            if show_action:
                g, urgent = self.action
                ink = t.urgent() if urgent else t.dim()
                if self.action_cursor:
                    fill(t, buf, Rect(ex, y, 1, 1), ink.with_alpha(float(_f32(c.hover.fill.a / 255.0))))
                put(t, buf, ex, y, g, ink)
                if self.key is not None:
                    t.hit((self.key, "action"), Rect(ex, y, 1, 1))
            elif self.trailing_glyph is not None:
                g, tone = self.trailing_glyph
                put(t, buf, ex, y, g, faint if faint is not None else Tone.of(tone).color(t, t.dim()))
            right = max(0, right - 1)
        if self.trailing is not None:
            tw, _ = self.trailing.size(t)
            right = max(0, right - tw)
            self.trailing.draw(t, Rect(right, y, tw, 1), buf)
            right = max(0, right - 1)
        if self.value:
            vw = width(self.value)
            text_right(t, buf, right, y, self.value, faint if faint is not None else t.dim())
            right = max(0, right - (vw + 1))
        avail = max(0, right - x)
        tw = text_fit(t, buf, x, y, self.title, avail, Ink(faint if faint is not None else t.fg(), bold=True))
        second = None
        if self.status is not None:
            s, tone = self.status
            second = (s, faint if faint is not None else Tone.of(tone).color(t, t.dim()))
        elif self.subtitle:
            second = (self.subtitle, faint if faint is not None else t.dim())
        if second is not None and avail > tw + 2:
            text_fit(t, buf, x + tw + 2, y, second[0], avail - tw - 2, second[1])
        if self.expansion is not None:
            _, eh = self.expansion.size(t)
            self.expansion.draw(t, Rect(area.x + 2, y + 1, max(0, area.width - 4), eh), buf)


def _f32(x: float) -> float:
    from ..color import f32

    return f32(x)


# ScrollArea ------------------------------------------------------------------------------------

_U16_MAX = 65535


class ScrollArea(Draw):
    """A vertical scroll view capped at `max_rows`, the bar over its last column while the content
    overflows: a `│` groove in `track`, a `┃` thumb in the normal border color."""

    def __init__(self, width: int, *children: Draw, max_rows: int = _U16_MAX, offset: int = 0, gap: int = 0):
        self.width, self.children = width, list(children)
        self.max_rows, self.offset, self.gap = max_rows, offset, gap

    def child(self, d: Draw) -> "ScrollArea":
        self.children.append(d)
        return self

    def content_h(self, t) -> int:
        n = len(self.children)
        return sum(c.size(t)[1] for c in self.children) + max(0, n - 1) * self.gap

    @staticmethod
    def ensure_visible(offset: int, top: int, h: int, max_rows: int) -> int:
        """The offset showing content rows `[top, top + h)` in a `max_rows` viewport, moving as
        little as possible from `offset`."""
        if top < offset:
            return top
        if top + h > offset + max_rows:
            return max(0, top + h - max_rows)
        return offset

    def size(self, t) -> tuple[int, int]:
        return (self.width, min(self.content_h(t), self.max_rows))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        total = self.content_h(t)
        view = area.height
        offset = min(self.offset, max(0, total - view))
        scratch_area = Rect(area.x, 0, area.width, max(total, 1))
        scratch = Buffer(scratch_area)
        # What's under the area shows through the scratch (fills composite over it).
        for y in range(min(total, view + offset)):
            if y >= offset:
                for x in range(area.x, area.right):
                    src, dst = buf.cell(x, area.y + y - offset), scratch.cell(x, y)
                    if src is not None and dst is not None:
                        dst.sym, dst.fg, dst.bg, dst.mod = src.sym, src.fg, src.bg, src.mod
        y = 0
        hits = len(t.hits)
        for c in self.children:
            cw, ch = c.size(t)
            c.draw(t, Rect(area.x, y, min(cw, area.width), ch).intersection(scratch_area), scratch)
            y += ch + self.gap
        # Keys drawn in the scratch buffer land where they show, or nowhere when scrolled away.
        shown = Rect(area.x, offset, area.width, view)
        tail = []
        for k, r in t.hits[hits:]:
            r = r.intersection(shown)
            if r.height > 0:
                tail.append((k, r.replace(y=r.y - offset + area.y)))
        t.hits[hits:] = tail
        for dy in range(min(view, total)):
            for x in range(area.x, area.right):
                src, dst = scratch.cell(x, offset + dy), buf.cell(x, area.y + dy)
                if src is not None and dst is not None:
                    dst.sym, dst.fg, dst.bg, dst.mod = src.sym, src.fg, src.bg, src.mod
        if total > view and view > 0:
            bx = area.right - 1
            thumb = max(1, (view * view + total // 2) // total)
            room = view - thumb
            max_off = total - view
            ty = (offset * room + max_off // 2) // max_off
            for dy in range(view):
                on = ty <= dy < ty + thumb
                sym, ink = ("┃", t.shown.controls.normal.border.color) if on else ("│", t.track())
                put(t, buf, bx, area.y + dy, sym, ink)


# KeyValue, Hero --------------------------------------------------------------------------------


class KeyValue(Draw):
    """Dim labels beside their values, `columns` pairs to a line: label column, one space, value
    column, three columns between pairs. `pairs` are (label, value) or (label, value, tone)."""

    def __init__(self, pairs: Iterable[tuple] = (), columns: int = 1):
        self.pairs = [(p[0], p[1], Tone.of(p[2]) if len(p) > 2 else Tone.NEUTRAL) for p in pairs]
        self.columns = max(1, columns)

    def pair(self, label: str, value: str, tone: Tone = Tone.NEUTRAL) -> "KeyValue":
        self.pairs.append((label, value, Tone.of(tone)))
        return self

    def widths(self) -> list[tuple[int, int]]:
        out = []
        for c in range(self.columns):
            col = self.pairs[c :: self.columns]
            out.append((max((width(a) for a, _, _ in col), default=0), max((width(b) for _, b, _ in col), default=0)))
        return out

    def size(self, t) -> tuple[int, int]:
        w = sum(l + 1 + v for l, v in self.widths()) + 3 * (self.columns - 1)
        return (w, -(-len(self.pairs) // self.columns))

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        ws = self.widths()
        for i, (l, v, tone) in enumerate(self.pairs):
            line, col = i // self.columns, i % self.columns
            x = area.x + sum(a + 1 + b + 3 for a, b in ws[:col])
            y = area.y + line
            text(t, buf, x, y, l, ws[col][0], t.dim())
            text(t, buf, x + ws[col][0] + 1, y, v, ws[col][1], tone.color(t, t.fg()))


class Hero(Draw):
    """A panel's head: the glyph, then the bold title over an uppercased bold status line
    (`textDim`, urgent on error), the trailing control at the end of the first row."""

    def __init__(self, glyph: str, title: str, status: str = "", error: bool = False, glyph_opacity: float = 1.0,
                 trailing: Draw | None = None, width: int = 53):
        self.glyph, self.title, self.status, self.error = glyph, title, status, error
        self.glyph_opacity, self.trailing, self.width = glyph_opacity, trailing, width

    def size(self, t) -> tuple[int, int]:
        return (self.width, 2)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        put(t, buf, area.x, area.y, self.glyph, t.fg().with_alpha(self.glyph_opacity))
        tw = self.trailing.size(t)[0] + 2 if self.trailing is not None else 0
        words = max(0, area.width - (3 + tw))
        text_fit(t, buf, area.x + 3, area.y, self.title, words, Ink(t.fg(), bold=True))
        ink = t.urgent() if self.error else t.dim()
        text_fit(t, buf, area.x + 3, area.y + 1, self.status.upper(), words, Ink(ink, bold=True))
        if self.trailing is not None:
            w, h = self.trailing.size(t)
            self.trailing.draw(t, Rect(max(0, area.right - w), area.y, w, h), buf)


# Badge, StatusDot ------------------------------------------------------------------------------


class Badge(Draw):
    """A count or short label: ` 3 ` bold. Neutral: the normal fill; toned: the tone at the
    selected fill's alpha, text in the tone; filled: the tone solid, text in the background."""

    def __init__(self, label: str, tone: Tone | str = Tone.NEUTRAL, filled: bool = False):
        self.label, self.tone, self.filled = label, Tone.of(tone), filled

    def size(self, t) -> tuple[int, int]:
        return (width(self.label) + 2, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        c = t.shown.controls
        tone = self.tone.color(t, t.fg())
        if self.filled:
            bg, ink = tone, t.bg()
        elif self.tone is Tone.NEUTRAL:
            bg, ink = c.normal.fill, t.fg()
        else:
            bg, ink = tone.with_alpha(float(_f32(c.selected.fill.a / 255.0))), tone
        fill(t, buf, area, bg)
        text(t, buf, area.x + 1, area.y, self.label, max(0, area.width - 2), Ink(ink, bold=True))


class StatusDot(Draw):
    """A dot `●` in its tone."""

    def __init__(self, tone: Tone | str = Tone.NEUTRAL):
        self.tone = Tone.of(tone)

    def size(self, t) -> tuple[int, int]:
        return (1, 1)

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        put(t, buf, area.x, area.y, "●", self.tone.color(t, t.fg()))


# Avatar ----------------------------------------------------------------------------------------


class AvatarSize(enum.Enum):
    """An avatar's size in cells: small 2×1 (a ListRow's leading slot), medium 4×2, large 6×3."""

    SMALL = "small"
    MEDIUM = "medium"
    LARGE = "large"

    def cells(self) -> tuple[int, int]:
        return {AvatarSize.SMALL: (2, 1), AvatarSize.MEDIUM: (4, 2), AvatarSize.LARGE: (6, 3)}[self]

    @staticmethod
    def of(v: "AvatarSize | str") -> "AvatarSize":
        return v if isinstance(v, AvatarSize) else AvatarSize(v)


def _paeth(a: int, b: int, c: int) -> int:
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    return b if pb <= pc else c


class AvatarImage:
    """A decoded picture (8-bit straight RGBA), drawn in half-block pixels."""

    def __init__(self, width: int, height: int, rgba: bytes):
        self.width, self.height, self.rgba = width, height, rgba

    @staticmethod
    def load(path: str | Path) -> "AvatarImage | None":
        try:
            return AvatarImage.decode(Path(path).read_bytes())
        except OSError:
            return None

    @staticmethod
    def decode(data: bytes) -> "AvatarImage | None":
        """Decode PNG bytes (8- or 16-bit gray, gray-alpha, RGB, RGBA, or 8-bit palette; not
        interlaced) with zlib alone; None when it can't be read."""
        try:
            return AvatarImage._decode(data)
        except (ValueError, IndexError, struct.error, zlib.error):
            return None

    @staticmethod
    def _decode(data: bytes) -> "AvatarImage | None":
        if data[:8] != b"\x89PNG\r\n\x1a\n":
            return None
        pos = 8
        idat = []
        palette = trns = None
        w = h = depth = ctype = interlace = 0
        while pos < len(data):
            n, kind = struct.unpack(">I4s", data[pos : pos + 8])
            body = data[pos + 8 : pos + 8 + n]
            pos += 12 + n
            if kind == b"IHDR":
                w, h, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
            elif kind == b"PLTE":
                palette = body
            elif kind == b"tRNS":
                trns = body
            elif kind == b"IDAT":
                idat.append(body)
            elif kind == b"IEND":
                break
        if interlace or depth not in (8, 16) or ctype not in (0, 2, 3, 4, 6) or (ctype == 3 and depth != 8):
            return None
        channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ctype]
        bpp = channels * depth // 8
        stride = w * bpp
        raw = zlib.decompress(b"".join(idat))
        out = bytearray(stride * h)
        prev = bytearray(stride)
        p = 0
        for y in range(h):
            f = raw[p]
            line = bytearray(raw[p + 1 : p + 1 + stride])
            p += 1 + stride
            for i in range(stride):
                a = line[i - bpp] if i >= bpp else 0
                b = prev[i]
                c = prev[i - bpp] if i >= bpp else 0
                if f == 1:
                    line[i] = (line[i] + a) & 0xFF
                elif f == 2:
                    line[i] = (line[i] + b) & 0xFF
                elif f == 3:
                    line[i] = (line[i] + ((a + b) >> 1)) & 0xFF
                elif f == 4:
                    line[i] = (line[i] + _paeth(a, b, c)) & 0xFF
            out[y * stride : (y + 1) * stride] = line
            prev = line
        if depth == 16:
            out = out[0::2]  # the high bytes (STRIP_16)
        rgba = bytearray(w * h * 4)
        for i in range(w * h):
            if ctype == 6:
                rgba[i * 4 : i * 4 + 4] = out[i * 4 : i * 4 + 4]
            elif ctype == 2:
                rgba[i * 4 : i * 4 + 3] = out[i * 3 : i * 3 + 3]
                rgba[i * 4 + 3] = 255
            elif ctype == 4:
                g = out[i * 2]
                rgba[i * 4 : i * 4 + 4] = bytes((g, g, g, out[i * 2 + 1]))
            elif ctype == 0:
                g = out[i]
                rgba[i * 4 : i * 4 + 4] = bytes((g, g, g, 255))
            else:
                k = out[i]
                rgba[i * 4 : i * 4 + 3] = palette[k * 3 : k * 3 + 3]
                rgba[i * 4 + 3] = trns[k] if trns is not None and k < len(trns) else 255
        return AvatarImage(w, h, bytes(rgba))

    def sample(self, w: int, h: int, i: int, j: int) -> Rgba:
        """The `w` × `h` box-filtered pixel (i, j): the source box `[floor(i·W/w),
        floor((i+1)·W/w))` (and the same down), alpha-weighted color `round(Σc·a / Σa)`, alpha
        `round(Σa / n)`."""
        x0 = i * self.width // w
        x1 = max((i + 1) * self.width // w, x0 + 1)
        y0 = j * self.height // h
        y1 = max((j + 1) * self.height // h, y0 + 1)
        r = g = b = a = n = 0
        px = self.rgba
        for y in range(y0, min(y1, self.height)):
            for x in range(x0, min(x1, self.width)):
                p = (y * self.width + x) * 4
                pa = px[p + 3]
                r += px[p] * pa
                g += px[p + 1] * pa
                b += px[p + 2] * pa
                a += pa
                n += 1
        if a == 0 or n == 0:
            return TRANSPARENT

        def ch(s: int) -> int:
            return (s * 2 + a) // (2 * a)

        return Rgba(ch(r), ch(g), ch(b), (a * 2 + n) // (2 * n))


class Avatar(Draw):
    """A person's or an app's picture with fallbacks: the image (in half-block pixels, two per
    cell), a glyph, the initial. Icons by freedesktop name don't exist in a terminal: an Avatar with
    only `icon` draws its glyph or its initial (spec/terminal.md, Icons)."""

    def __init__(self, name: str, image: AvatarImage | None = None, icon: str | None = None, glyph: str = "",
                 size: AvatarSize | str = AvatarSize.MEDIUM, round: bool = False, status: Tone | str | None = None):
        self.name, self.image, self.icon, self.glyph = name, image, icon, glyph
        self.avatar_size, self.round = AvatarSize.of(size), round
        self.status = Tone.of(status) if status is not None else None

    @staticmethod
    def initial(name: str) -> str:
        """The first letter (or digit) of `name`, upper-cased."""
        for ch in name:
            if ch.isalnum():
                return ch.upper()
        return ""

    def size(self, t) -> tuple[int, int]:
        return self.avatar_size.cells()

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        w, h = self.avatar_size.cells()
        area = area.replace(width=min(w, area.width), height=min(h, area.height))
        img = self.image
        if img is not None:
            pw, ph = w, h * 2
            for j in range(h):
                for i in range(w):

                    def px(pj: int) -> Rgba:
                        if self.round:
                            cx, cy = i + 0.5 - pw / 2.0, pj + 0.5 - ph / 2.0
                            r = min(pw, ph) / 2.0
                            if cx * cx + cy * cy > r * r:
                                return TRANSPARENT
                        return img.sample(pw, ph, i, pj)

                    top, bottom = px(j * 2), px(j * 2 + 1)
                    x, y = area.x + i, area.y + j
                    fill(t, buf, Rect(x, y, 1, 1), bottom)
                    put(t, buf, x, y, "▀", over(top, t.bg()))
        else:
            fill(t, buf, area, t.shown.controls.selected.fill)
            if self.glyph:
                mark, bold = self.glyph, False
            else:
                mark, bold = Avatar.initial(self.name), True
            cx, cy = area.x + (w - 1) // 2, area.y + (h - 1) // 2
            put(t, buf, cx, cy, mark, Ink(t.fg(), bold=bold))
        if self.status is not None:
            put(t, buf, area.right - 1, area.bottom - 1, "●", self.status.color(t, t.fg()))


# TileGrid --------------------------------------------------------------------------------------


class Tile:
    """One tile: a glyph or an Avatar over a label of up to two lines."""

    def __init__(self, label: str, glyph: str = "", avatar: Avatar | None = None):
        self.label, self.glyph = label, glyph
        if avatar is not None:
            avatar.avatar_size = AvatarSize.MEDIUM
        self.avatar = avatar


class TileGrid(Draw):
    """Tiles 11 columns × 4 rows, no gap: an icon box of two rows (the glyph on its second row, or a
    medium Avatar filling it), then two label rows, centered, wrapped and elided. The cursor tile
    takes the hover fill and edges, the current one the selected fill. Tiles keyed `(key, i)`."""

    TILE = (11, 4)

    def __init__(self, tiles: Iterable[Tile], columns: int = 4, cursor: int | None = None,
                 current: int | None = None, key: object = None):
        self.tiles, self.columns = list(tiles), max(1, columns)
        self.cursor, self.current, self.key = cursor, current, key

    def size(self, t) -> tuple[int, int]:
        cols = min(self.columns, max(1, len(self.tiles)))
        return (cols * self.TILE[0], -(-len(self.tiles) // self.columns) * self.TILE[1])

    def draw(self, t, area: Rect, buf: Buffer) -> None:
        tw, th = self.TILE
        for i, tile in enumerate(self.tiles):
            c, r = i % self.columns, i // self.columns
            cell = Rect(area.x + c * tw, area.y + r * th, tw, th).intersection(area)
            if cell.height < th:
                continue
            l = look(t, self.cursor == i, self.current == i, False, False)
            fill(t, buf, cell, l.fill)
            for dy in range(th):
                edges(t, buf, cell.x, cell.y + dy, tw, l.border)
            if tile.avatar is not None:
                aw, ah = tile.avatar.size(t)
                tile.avatar.draw(t, Rect(cell.x + (tw - aw) // 2, cell.y, aw, ah), buf)
            else:
                text_center(t, buf, cell.x + 1, cell.y + 1, tw - 2, tile.glyph, t.fg())
            lines = wrap(tile.label, tw - 2)
            for n, line in enumerate(lines[:2]):
                if n == 1 and len(lines) > 2:
                    line = paint.fit(f"{line} {' '.join(lines[2:])}", tw - 2)
                text_center(t, buf, cell.x + 1, cell.y + 2 + n, tw - 2, line, Ink(t.fg(), bold=l.bold))
            if self.key is not None:
                t.hit((self.key, i), cell)
