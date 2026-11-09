"""Behavior tests for the search list and the composed section (Ratatui's
`search_list_filters_moves_and_activates`), through Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets.lists import ScrollArea
from omarchy_textual.widgets.search import SearchList


class Search(unittest.IsolatedAsyncioTestCase):
    async def test_filters_moves_and_activates(self):
        async with gallery("composed", size=(96, 40)) as (app, pilot):
            s = app.query_one("#searchlist", SearchList)
            s.focus()
            await pilot.press("down", "enter")
            await pilot.pause()
            self.assertEqual(posted(app, SearchList.Activated)[-1].index, 2)
            await pilot.press("space", "t")
            self.assertEqual(s.value, "term t")
            await pilot.press("backspace", "backspace", "escape")
            self.assertEqual(s.value, "", "Escape clears the query first")
            await pilot.press("c", "a")
            self.assertEqual(s.value, "ca")
            self.assertEqual([i.title for i in s.matches], ["Intermediate CA"])
            await pilot.press("enter")
            await pilot.pause()
            last = posted(app, SearchList.Activated)[-1]
            self.assertEqual((last.index, last.item.title), (0, "Intermediate CA"),
                             "a new query puts the cursor on the first match")
            await pilot.press("backspace", "backspace", "q")
            self.assertEqual(s.value, "q", "q types into the query (and doesn't quit)")
            self.assertTrue(app.is_running)
            await pilot.press("backspace")
            # A click on a row activates it.
            r = s.rect_of((s, ("row", 1)))
            sr = s.kit_root.content_region
            await pilot.click(offset=(sr.x + r.x + 5, sr.y + r.y))
            await pilot.pause()
            self.assertEqual(posted(app, SearchList.Activated)[-1].index, 1)
            await pilot.press("escape")
            await pilot.pause()
            self.assertEqual(len(posted(app, SearchList.Closed)), 1, "Escape on an empty query closes")

    async def test_tab_order_and_networks(self):
        async with gallery("composed", size=(96, 40)) as (app, pilot):
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "networks")
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "searchlist")
            nets = app.query_one("#networks", ScrollArea)
            nets.focus()
            self.assertEqual(nets.cursor_index, 1)
            await pilot.press("j", "j")
            self.assertEqual(nets.cursor_index, 3)
            await pilot.press("enter")
            await pilot.pause()
            self.assertEqual(posted(app, ScrollArea.Activated)[-1].index, 3)


if __name__ == "__main__":
    unittest.main()
