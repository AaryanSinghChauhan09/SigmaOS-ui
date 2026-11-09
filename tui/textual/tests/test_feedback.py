"""Behavior tests for the feedback components: spinners and the indeterminate bar on the clock
(Ratatui's `spinners_turn_and_pulse_on_the_clock`), and the messages section's action, dismiss and
focus, through Textual's Pilot on the gallery."""

from __future__ import annotations

import copy
import unittest

from support import fixture_state, gallery, posted

from omarchy_textual import Depth, Theme, Tokens, kit_of
from omarchy_textual.cells import Spinner as SpinnerCells
from omarchy_textual.cells.base import glyphs
from omarchy_textual.widgets.feedback import Banner, EmptyState, Spinner, Toast


def part_offset(widget, part):
    """Where `(widget, part)` was drawn, as an offset into `widget` (for `pilot.click`)."""
    r = widget.rect_of((widget, part))
    root = widget.kit_root
    return (r.x - (widget.region.x - root.region.x), r.y - (widget.region.y - root.region.y))


class SpinnersOnTheClock(unittest.TestCase):
    def test_spinners_turn_and_pulse_on_the_clock(self):
        self.assertEqual(SpinnerCells.frame(0), 0)
        self.assertEqual(SpinnerCells.frame(95), 1)
        self.assertEqual(SpinnerCells.frame(899), 9)
        self.assertEqual(SpinnerCells.frame(900), 0)
        self.assertEqual(SpinnerCells.opacity(0), 0.5)
        self.assertAlmostEqual(SpinnerCells.opacity(420), 1.0, places=9)


class SpinnersMove(unittest.IsolatedAsyncioTestCase):
    async def test_spinner_draws_the_clock_without_reduced_motion(self):
        from omarchy_textual.gallery.app import GalleryApp

        state = copy.deepcopy(fixture_state("tokyo-night"))
        state["tokens"]["reduceMotion"] = False
        theme = Theme(Tokens(state), depth=Depth.TRUE)
        theme.set_clock(95)
        app = GalleryApp(section="progress", snap=True, theme=theme)
        kit_of(app).live = False
        async with app.run_test(size=(96, 30), notifications=False) as pilot:
            await pilot.pause()
            spinner = app.query(Spinner).first()
            frame = spinner.kit_frame()
            r = frame.places[spinner]
            self.assertEqual(frame.buf.cell(r.x, r.y).sym, glyphs.SPINNER[1], "frame 1 at 95 ms")
            theme.set_clock(190)
            spinner.refresh()
            await pilot.pause()
            frame = spinner.kit_frame()
            self.assertEqual(frame.buf.cell(r.x, r.y).sym, glyphs.SPINNER[2], "the clock moved it")

    async def test_still_under_reduced_motion(self):
        async with gallery("progress") as (app, pilot):
            kit_of(app).theme.set_clock(95)
            spinner = app.query(Spinner).first()
            spinner.refresh()
            await pilot.pause()
            frame = spinner.kit_frame()
            r = frame.places[spinner]
            self.assertEqual(frame.buf.cell(r.x, r.y).sym, glyphs.SPINNER[0])


class Messages(unittest.IsolatedAsyncioTestCase):
    async def test_scan_takes_focus_and_keys(self):
        async with gallery("messages") as (app, pilot):
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "scan", "the empty state's action is the section's one stop")
            await pilot.press("enter", "space")
            await pilot.pause()
            self.assertEqual(len(posted(app, EmptyState.ActionPressed)), 2)

    async def test_clicks_reach_the_parts(self):
        async with gallery("messages") as (app, pilot):
            scan = app.query_one("#scan", EmptyState)
            await pilot.click("#scan", offset=part_offset(scan, "action"))
            update = app.query_one("#banner-update", Banner)
            await pilot.click("#banner-update", offset=part_offset(update, "action"))
            backup = app.query_one("#banner-backup", Banner)
            await pilot.click("#banner-backup", offset=part_offset(backup, "dismiss"))
            toast = app.query_one("#toast", Toast)
            await pilot.click("#toast", offset=part_offset(toast, "action"))
            # A click on the banner's text does nothing.
            await pilot.click("#banner-error", offset=(6, 0))
            await pilot.pause()
            self.assertEqual(len(posted(app, EmptyState.ActionPressed)), 1)
            self.assertEqual([m.banner.id for m in posted(app, Banner.ActionPressed)], ["banner-update"])
            self.assertEqual([m.banner.id for m in posted(app, Banner.Dismissed)], ["banner-backup"])
            self.assertEqual(len(posted(app, Toast.ActionPressed)), 1)


if __name__ == "__main__":
    unittest.main()
