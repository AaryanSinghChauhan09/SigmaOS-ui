"""Behavior tests for the controls (the keys and clicks the other galleries' tests drive: Ratatui's
`gallery/src/tests.rs`, `qt/tests/tst_*.qml`), through Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets import (
    Button,
    ButtonGroup,
    Dropdown,
    DropdownList,
    NumberField,
    Slider,
    TextField,
    Toggle,
    ToggleSwitch,
)


def focus_order(app) -> list[str]:
    """The ids Tab walks, in order."""
    return [w.id for w in app.screen.focus_chain]


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


class Inputs(unittest.IsolatedAsyncioTestCase):
    async def test_fields_type_step_and_choose(self):
        async with gallery("inputs") as (app, pilot):
            self.assertEqual(focus_order(app), ["search", "name", "gaps", "theme"])
            search = app.query_one("#search", TextField)
            search.focus()
            await pilot.press("w", "i", "f", "i", "backspace")
            self.assertEqual(search.value, "wif")
            # Keys a field takes aren't the app's: q types, it doesn't quit.
            await pilot.press("q")
            self.assertEqual(search.value, "wifq")
            self.assertTrue(app.is_running)
            self.assertEqual([m.value for m in posted(app, TextField.Changed)][-1], "wifq")

            gaps = app.query_one("#gaps", NumberField)
            await pilot.click("#gaps", offset=(gaps.region.width - 3, 1))
            self.assertEqual(gaps.value, 11)
            gaps.focus()
            await pilot.press("down", "down")
            self.assertEqual(gaps.value, 9)
            await pilot.click("#gaps", offset=(2, 1))
            self.assertEqual(gaps.value, 8)

    async def test_dropdown_opens_walks_and_picks(self):
        async with gallery("inputs") as (app, pilot):
            theme = app.query_one("#theme", Dropdown)
            theme.focus()
            await pilot.press("enter", "j", "j", "k", "enter")
            self.assertEqual(theme.value, "Catppuccin")
            self.assertFalse(theme.expanded)
            # A click on the trigger opens it, a click on a row picks.
            await pilot.click("#theme", offset=(3, 1))
            await pilot.pause()
            self.assertTrue(theme.expanded)
            lst = app.query_one(DropdownList)
            self.assertEqual(lst.region.y, theme.region.y + 2, "the list sits under the trigger")
            await pilot.click(DropdownList, offset=(3, 1 + 3))
            await pilot.pause()
            self.assertEqual(theme.value, "Nord")
            self.assertFalse(theme.expanded)
            self.assertEqual(len(app.query(DropdownList)), 0)
            self.assertEqual([m.value for m in posted(app, Dropdown.Changed)], ["Catppuccin", "Nord"])
            # Escape closes without choosing.
            theme.focus()
            await pilot.press("down", "j", "escape")
            self.assertFalse(theme.expanded)
            self.assertEqual(theme.value, "Nord")


class Sliders(unittest.IsolatedAsyncioTestCase):
    async def test_sliders_step_and_follow_clicks(self):
        async with gallery("slider") as (app, pilot):
            stepped = app.query_one("#stepped", Slider)
            stepped.focus()
            await pilot.press("right")
            self.assertEqual(stepped.value, 1.0)
            await pilot.press("left", "left")
            self.assertEqual(stepped.value, 0.5)
            level = app.query_one("#level", Slider)
            await pilot.click("#level", offset=(1, 0))
            self.assertEqual(level.value, 0.0)
            await pilot.click("#level", offset=(level.region.width - 2, 0))
            self.assertEqual(level.value, 1.0)
            # A click on a notched slider lands on a notch.
            await pilot.click("#stepped", offset=(12, 0))
            self.assertEqual(stepped.value, 0.25)
            await pilot.press("end")
            self.assertEqual(stepped.value, 1.0)
            self.assertTrue(posted(app, Slider.Changed))


if __name__ == "__main__":
    unittest.main()
