"""Behavior tests for the controls (the keys and clicks the other galleries' tests drive: Ratatui's
`gallery/src/tests.rs`, `qt/tests/tst_*.qml`), through Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets import Button, ButtonGroup, Toggle, ToggleSwitch


class Toggles(unittest.IsolatedAsyncioTestCase):
    async def test_toggles_toggle(self):
        async with gallery("toggles") as (app, pilot):
            await pilot.click("#switch-0")
            self.assertTrue(app.query_one("#switch-0", ToggleSwitch).value)
            await pilot.click("#bluetooth")
            self.assertTrue(app.query_one("#bluetooth", Toggle).value)
            wifi = app.query_one("#wifi", Toggle)
            wifi.focus()
            await pilot.press("space")
            self.assertFalse(wifi.value)
            await pilot.press("enter")
            self.assertTrue(wifi.value)
            await pilot.pause()
            self.assertEqual(len(posted(app, ToggleSwitch.Changed)), 4, "a Changed message per flip")


class Buttons(unittest.IsolatedAsyncioTestCase):
    async def test_button_group_takes_keys_and_clicks(self):
        async with gallery("buttons") as (app, pilot):
            group = app.query_one("#align", ButtonGroup)
            group.focus()
            await pilot.press("right", "l")
            self.assertEqual(group.selected, 2, "stops at the end")
            await pilot.press("h", "home")
            self.assertEqual(group.selected, 0)
            r = group.region
            await pilot.click("#align", offset=(r.width - 3, 0))
            self.assertEqual(group.selected, 2)

    async def test_buttons_press_on_click_and_keys(self):
        async with gallery("buttons") as (app, pilot):
            await pilot.click("#bordered")
            app.query_one("#plain").focus()
            await pilot.press("enter", "space")
            await pilot.pause()
            self.assertEqual([m.button.id for m in posted(app, Button.Pressed)], ["bordered", "plain", "plain"])

    async def test_tab_walks_the_controls(self):
        async with gallery("buttons") as (app, pilot):
            await pilot.press("tab")
            self.assertEqual(app.focused.id, "plain")
            await pilot.press("shift+tab", "shift+tab")
            self.assertEqual(app.focused.id, "power")


if __name__ == "__main__":
    unittest.main()
