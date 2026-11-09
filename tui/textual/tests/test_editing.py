"""Behavior tests for the editing widgets (Ratatui's `editing_selects_and_seeks`), through
Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import fixture, gallery, posted

from omarchy_textual import cells as C
from omarchy_textual.cells.buffer import BOLD
from omarchy_textual.widgets.editing import ColorPicker, Scrubber, Timeline


class Editing(unittest.IsolatedAsyncioTestCase):
    async def test_timeline_selects_and_seeks(self):
        async with gallery("timeline") as (app, pilot):
            ids = []
            for _ in range(3):
                await pilot.press("tab")
                ids.append(app.focused.id)
            self.assertEqual(ids, ["timeline", "scrubber", "drag"])
            tl = app.query_one("#timeline", Timeline)
            # Voiceover: the second lane (rows 6–9), from x(2 s) = 6.
            await pilot.click("#timeline", offset=(8, 7))
            self.assertEqual(tl.selected, (1, 0))
            tl.focus()
            await pilot.press("right")
            self.assertEqual(tl.position, 13.0)
            await pilot.press("shift+right")
            self.assertEqual(tl.position, 18.0, "Shift steps by the labeled step (5 s at 90 columns for 30 s)")
            await pilot.press("end")
            self.assertEqual(tl.position, 30.0)
            await pilot.click("#timeline", offset=(45, 0))
            self.assertEqual(tl.position, 15.0, "a click on the ruler seeks")
            self.assertTrue(posted(app, Timeline.Seeked))

            sc = app.query_one("#scrubber", Scrubber)
            sc.focus()
            await pilot.press("right")
            self.assertEqual(sc.position, 47.0)
            await pilot.press("home")
            self.assertEqual(sc.position, 0.0)

            drag = app.query_one("#drag")
            drag.focus()
            await pilot.press("j", "j", "j")
            self.assertEqual(drag.cursor, 3)
            await pilot.click("#drag", offset=(5, 0))
            self.assertEqual(drag.cursor, 0)

    async def test_color_picker_steps_and_takes_swatches(self):
        async with gallery("color") as (app, pilot):
            picker = app.query_one("#picker", ColorPicker)
            picker.focus()
            s0 = picker.color.s
            await pilot.press("right")
            self.assertAlmostEqual(picker.color.s - s0, 0.01)
            # The palette's first swatch (red), on the row 3 under the square.
            await pilot.click("#picker", offset=(0, 14))
            red = app._omarchy_kit.theme.shown.palette.named["red"]
            self.assertEqual(picker.color.rgba().hex()[:7], red.hex()[:7])
            self.assertTrue(posted(app, ColorPicker.Changed))


    def test_scrubber_knob_is_never_bold(self):
        """The Scrubber's knob is the Slider's: foreground, never bold, under the cursor too."""
        t = fixture("tokyo-night")
        buf = C.Buffer(C.Rect(0, 0, 45, 1))
        C.Scrubber(42.0, 130.0, width=45, cursor=True).draw(t, buf.area, buf)
        knob = next(c for c in buf.row(0) if c.sym == "●")
        self.assertFalse(knob.mod & BOLD)


if __name__ == "__main__":
    unittest.main()
