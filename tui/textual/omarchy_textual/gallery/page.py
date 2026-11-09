"""A gallery section (spec/terminal.md, Gallery): 96 columns, a blank row, from column 2 the title
(a SectionHeader), a blank row, the body, a blank row. Each section is a `Section` whose one child
is its body (usually a `Col`)."""

from __future__ import annotations

from ..cells import layout as L
from ..cells.base import Draw
from ..cells.paint import Ink
from ..widget import KitWidget

#: Every section is this wide; the body gets 4 fewer.
COLS = 96
CONTENT = COLS - 4


class Section(KitWidget):
    """The page frame around a section's body."""

    #: (name, title), set by each section.
    NAME = ""
    TITLE = ""

    def component(self) -> Draw:
        header = L.label(self.TITLE, lambda t: Ink(t.dim(), bold=True))
        inner = L.col(1, header, *self.parts())
        return L.col(0, L.Blank(COLS, 1), L.row(0, L.Blank(2, 1), inner), L.Blank(COLS, 1))


class Placeholder(Section):
    """A section not written yet."""

    def component(self) -> Draw:
        header = L.label(self.TITLE, lambda t: Ink(t.dim(), bold=True))
        note = L.label("(not ported yet)", lambda t: t.faint())
        return L.col(0, L.Blank(COLS, 1), L.row(0, L.Blank(2, 1), L.col(1, header, note)), L.Blank(COLS, 1))
