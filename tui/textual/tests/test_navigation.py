"""Behavior tests for Menus and Navigation (as `tui/ratatui/gallery/src/tests.rs` drives them:
menu_walks_with_vim_keys_and_arrows, tabs_and_rail_choose), through Textual's Pilot."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets import IconRail, Menu, Tabs


async def click_part(pilot, widget, part) -> None:
    """Click the middle of `widget`'s part keyed `(widget, part)`."""
    r = widget.rect_of((widget, part))
    assert r is not None, f"no part {part!r} drawn"
    root = widget.kit_root.content_region
    x = root.x + r.x + r.width // 2 - widget.region.x
    y = root.y + r.y + r.height // 2 - widget.region.y
    await pilot.click(widget, offset=(x, y))


class Menus(unittest.IsolatedAsyncioTestCase):
    async def test_menu_walks_with_vim_keys_and_arrows(self):
        async with gallery("menus") as (app, pilot):
            menu = app.query_one("#menu", Menu)
            label = lambda: posted(app, Menu.Selected)[-1].item.label  # noqa: E731
            await click_part(pilot, menu, 0)
            self.assertEqual(label(), "New")
            self.assertIs(app.focused, menu, "a click gives the menu the keys")
            sidebar, wrap = menu.items[4], menu.items[5]
            for keys, ran in [
                (["j", "enter"], "Open…"),
                (["down", "enter"], "Save"),
                (["k", "enter"], "Open…"),
                (["j", "j", "enter"], "Sidebar"),
                (["end", "down", "enter"], "New"),
                (["home", "up", "up", "enter"], "Word wrap"),
            ]:
                await pilot.press(*keys)
                await pilot.pause()
                self.assertEqual(label(), ran, f"after {keys}")
            self.assertFalse(sidebar.check, "j skips the disabled row to Sidebar and Return flips it")
            self.assertTrue(wrap.check, "Up from the first row wraps to the last")

            context = app.query_one("#context", Menu)
            by_name, by_date = context.items[3], context.items[4]
            await click_part(pilot, context, 4)
            self.assertFalse(by_name.radio)
            self.assertTrue(by_date.radio)
            await pilot.press("k", "enter")
            await pilot.pause()
            self.assertTrue(by_name.radio)
            self.assertFalse(by_date.radio)

    async def test_tab_walks_the_menus(self):
        async with gallery("menus") as (app, pilot):
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "menu")
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "context")


class Navigation(unittest.IsolatedAsyncioTestCase):
    async def test_tabs_and_rail_choose(self):
        async with gallery("navigation") as (app, pilot):
            tabs = app.query_one("#tabs", Tabs)
            rail = app.query_one("#rail", IconRail)
            await click_part(pilot, tabs, 3)
            self.assertEqual(tabs.current, 3)
            await click_part(pilot, rail, 4)
            self.assertEqual(rail.current, 4)
            tabs.focus()
            for keys, want in [(["left"], 2), (["h"], 1), (["home"], 0), (["left"], 0), (["end"], 3), (["l"], 3)]:
                await pilot.press(*keys)
                self.assertEqual(tabs.current, want, f"tabs after {keys}")
            rail.focus()
            for keys, want in [(["home"], 0), (["j"], 1), (["down"], 2), (["k", "up"], 0), (["end"], 4), (["down"], 4)]:
                await pilot.press(*keys)
                self.assertEqual(rail.current, want, f"rail after {keys}")
            await pilot.pause()
            self.assertTrue(posted(app, Tabs.Changed) and posted(app, IconRail.Changed))

    async def test_rail_changes_the_panel(self):
        async with gallery("navigation") as (app, pilot):
            app.query_one("#rail", IconRail).focus()
            await pilot.press("end")
            await pilot.pause()
            from support import snap_text

            self.assertIn("Settings", snap_text(app))

    async def test_tab_walks_tabs_and_rail(self):
        async with gallery("navigation") as (app, pilot):
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "tabs")
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "rail")


if __name__ == "__main__":
    unittest.main()
