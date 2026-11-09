"""Behavior tests for the data widgets (Ratatui's `data_selects_and_folds` and
`layout_picks_and_steps`), through Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets.data import Calendar, Date, Sidebar, SplitView, Table, Tree


async def tab_order(app, pilot, n: int) -> list[str]:
    ids = []
    for _ in range(n):
        await pilot.press("tab")
        ids.append(app.focused.id)
    return ids


class Data(unittest.IsolatedAsyncioTestCase):
    async def test_data_selects_and_folds(self):
        async with gallery("data") as (app, pilot):
            self.assertEqual(await tab_order(app, pilot, 2), ["table", "tree"])
            table = app.query_one("#table", Table)
            await pilot.click("#table", offset=(4, 2 + 4))
            self.assertEqual((table.selected, table.cursor), (4, 4))
            table.focus()
            await pilot.press("k", "k", "enter")
            self.assertEqual(table.selected, 2)
            self.assertEqual([m.row for m in posted(app, Table.Selected)], [4, 2])

            tree = app.query_one("#tree", Tree)
            # rune is the fifth visible row (root, qt, Button.qml, Table.qml, rune), its chevron at 1 + 2.
            await pilot.click("#tree", offset=(3, 4))
            self.assertIn("rune", tree.expanded)
            tree.focus()
            tree.cursor = "qt"
            await pilot.press("left")
            self.assertNotIn("qt", tree.expanded, "Left folds an open branch")
            await pilot.press("right")
            self.assertIn("qt", tree.expanded)
            await pilot.press("j", "enter")
            self.assertEqual(tree.selected, "qt/button")
            self.assertTrue(posted(app, Tree.Toggled))


class Layout(unittest.IsolatedAsyncioTestCase):
    async def test_layout_picks_and_steps(self):
        async with gallery("layout") as (app, pilot):
            self.assertEqual(await tab_order(app, pilot, 3), ["sidebar", "handle", "cal"])
            side = app.query_one("#sidebar", Sidebar)
            await pilot.click("#sidebar", offset=(6, 4))
            self.assertEqual(side.selected, "trash")
            await pilot.press("k")
            self.assertEqual(side.selected, "starred")

            cal = app.query_one("#cal", Calendar)
            n = (Date(2026, 11, 20).ordinal() - Calendar_first(2026, 11).ordinal())
            await pilot.click("#cal", offset=(n % 7 * 4 + 1, 2 + n // 7))
            self.assertEqual(cal.selected, Date(2026, 11, 20))
            cal.focus()
            await pilot.press("j", "j", "right", "enter")
            self.assertEqual(cal.selected, Date(2026, 12, 5), "the month follows the cursor")
            self.assertEqual((cal.year, cal.month), (2026, 12))
            await pilot.click("#cal", offset=(1, 0))
            self.assertEqual((cal.year, cal.month), (2026, 11))

            split = app.query_one("#split", SplitView)
            app.query_one("#handle").focus()
            await pilot.press("l", "l")
            self.assertEqual(split.sizes[0], 27)
            await pilot.pause()
            self.assertEqual(side.region.width, 27, "the pane follows its handle")
            for _ in range(20):
                await pilot.press("h")
            self.assertEqual(split.sizes[0], 15, "not under the minimum")


def Calendar_first(y: int, m: int) -> Date:
    from omarchy_textual.cells.data import Calendar as Cells

    return Cells.first_cell(y, m)


if __name__ == "__main__":
    unittest.main()
