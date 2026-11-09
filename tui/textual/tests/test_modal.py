"""DialogScreen: a Dialog as a Textual modal screen, its scrim composited exactly (the core's
`mix`) over the cells of the screen under it; keys and clicks dismiss it with the choice."""

from __future__ import annotations

import unittest

from support import fixture

from omarchy_textual import OmarchyApp
from omarchy_textual.color import over
from omarchy_textual.widgets import Button, DialogScreen


class Under(OmarchyApp):
    AUTO_FOCUS = None

    def compose(self):
        yield Button("Selected", bordered=True, selected=True, id="b")


def cell(app, x, y):
    strip = app.screen._compositor.render_strips()[y]
    col = 0
    for seg in strip:
        for ch in seg.text:
            if col == x:
                return ch, seg.style
            col += 1
    raise IndexError(x)


class Modal(unittest.IsolatedAsyncioTestCase):
    async def test_scrim_over_the_screen_and_choices(self):
        app = Under(theme=fixture("tokyo-night"))
        app.kit.live = False
        choices = []
        async with app.run_test(size=(60, 16)) as pilot:
            await pilot.pause()
            ch0, st0 = cell(app, 3, 0)  # the selected button's label "S"
            app.push_screen(DialogScreen("Discard changes?", "Your edits will be lost.",
                                         labels=("Keep editing", "Discard"), destructive=True), choices.append)
            await pilot.pause()
            await pilot.pause()
            ch1, st1 = cell(app, 3, 0)
            t = app.kit.theme
            scrim = t.shown.roles.scrim
            self.assertEqual(ch1, ch0, "what is under the scrim still shows")
            fill = over(t.shown.controls.selected.fill, t.bg())
            want = over(scrim, fill)
            got = st1.bgcolor.triplet
            self.assertEqual((got.red, got.green, got.blue), (want.r, want.g, want.b))
            await pilot.press("h", "enter")
            await pilot.pause()
            self.assertEqual(choices, [0])
            self.assertEqual(cell(app, 3, 0)[1].bgcolor.triplet, st0.bgcolor.triplet, "the scrim is gone")
            app.push_screen(DialogScreen("Discard changes?", labels=("Keep editing", "Discard")), choices.append)
            await pilot.pause()
            await pilot.press("escape")
            await pilot.pause()
            self.assertEqual(choices, [0, 0])


if __name__ == "__main__":
    unittest.main()
