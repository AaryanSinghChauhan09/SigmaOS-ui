"""Behavior tests for lists and tiles (Ratatui's `lists_cursor_follows_keys_and_pointer`, the tile
grid's cursor), through Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.cells.lists import AvatarImage, CursorModel
from omarchy_textual.core import REPO
from omarchy_textual.widgets.lists import ListRow, ScrollArea, TileGrid


def current(area: ScrollArea) -> int:
    return [r.current for r in area.rows].index(True)


class Lists(unittest.IsolatedAsyncioTestCase):
    async def test_cursor_follows_keys_and_pointer(self):
        async with gallery("lists") as (app, pilot):
            rows = app.query_one("#rows", ScrollArea)
            rows.focus()
            self.assertEqual(rows.cursor_index, 1)
            await pilot.press("j")
            self.assertEqual(rows.cursor_index, 2)
            self.assertTrue(rows.rows[2].cursor and not rows.rows[1].cursor)
            await pilot.press("k")
            self.assertEqual(rows.cursor_index, 1)
            await pilot.press("end")
            self.assertEqual(rows.cursor_index, 8)
            self.assertEqual(rows.top_row, 2, "the last row is revealed")
            await pilot.press("home")
            self.assertEqual(rows.top_row, 0)
            await pilot.press("j", "enter")
            await pilot.pause()
            self.assertEqual(current(rows), 1)
            # The pointer: the first sample primes, the next in another cell moves.
            await pilot.hover("#row-4", offset=(3, 0))
            self.assertEqual(rows.cursor_index, 1)
            await pilot.hover("#row-4", offset=(4, 0))
            self.assertEqual(rows.cursor_index, 4)
            await pilot.click("#row-5", offset=(10, 0))
            await pilot.pause()
            self.assertEqual(current(rows), 5)
            self.assertEqual([m.index for m in posted(app, ScrollArea.Activated)], [1, 5])

    async def test_disabled_row_takes_no_clicks_and_actions_post(self):
        async with gallery("lists") as (app, pilot):
            rows = app.query_one("#rows", ScrollArea)
            await pilot.click("#row-3", offset=(10, 0))
            await pilot.pause()
            self.assertEqual(current(rows), 2, "Neighbor 5G is disabled")
            # Office shows its action (action cursor): a click on it is the action, not the row.
            office = app.query_one("#row-6", ListRow)
            await pilot.click("#row-6", offset=(office.region.width - 3, 0))
            await pilot.pause()
            self.assertEqual([m.row.id for m in posted(app, ListRow.ActionPressed)], ["row-6"])
            self.assertEqual(current(rows), 2)

    async def test_rows_scrolled_away_hide(self):
        async with gallery("lists") as (app, pilot):
            rows = app.query_one("#rows", ScrollArea)
            rows.top_row = 2
            await pilot.pause()
            self.assertEqual(rows.top_row, 2)
            self.assertEqual(app.query_one("#row-0").styles.visibility, "hidden", "scrolled away")


class Tiles(unittest.IsolatedAsyncioTestCase):
    async def test_tile_cursor_keys_clicks_and_pointer(self):
        async with gallery("tiles") as (app, pilot):
            grid = app.query_one("#tile", TileGrid)
            self.assertEqual(grid.cursor, 2)
            grid.focus()
            await pilot.press("l", "l")
            self.assertEqual(grid.cursor, 3, "stops at the row's end")
            await pilot.press("j")
            self.assertEqual(grid.cursor, 7)
            await pilot.press("h", "k")
            self.assertEqual(grid.cursor, 2)
            await pilot.click("#tile", offset=(5, 5))
            self.assertEqual(grid.cursor, 4)
            await pilot.hover("#tile", offset=(16, 1))
            self.assertEqual(grid.cursor, 1)
            await pilot.pause()
            self.assertEqual([m.index for m in posted(app, TileGrid.Selected)], [4])


class Model(unittest.TestCase):
    def test_cursor_model_grid_and_sections(self):
        m = CursorModel([("a", 5), ("b", 3)], columns=2)
        self.assertTrue(m.move_by(0, 1), "the first key only shows the cursor")
        self.assertEqual((m.section(), m.index(), m.active), ("a", 0, True))
        m.set("a", 3)
        m.move_by(0, 1)
        self.assertEqual((m.section(), m.index()), ("b", 1), "down past the end goes on in the next section")
        m.move_by(0, -1)
        self.assertEqual((m.section(), m.index()), ("a", 4), "up lands on the last line, same column (or the last)")
        self.assertTrue(m.end())
        self.assertEqual((m.section(), m.index()), ("b", 2))

    def test_the_sample_picture_decodes(self):
        img = AvatarImage.load(REPO / "qt/gallery/assets/avatar.png")
        self.assertIsNotNone(img)
        self.assertEqual((img.width, img.height, len(img.rgba)), (96, 96, 96 * 96 * 4))
        self.assertEqual(img.sample(4, 4, 0, 0).a, 255)


if __name__ == "__main__":
    unittest.main()
