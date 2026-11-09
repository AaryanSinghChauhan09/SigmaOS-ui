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



#: Every section's focusable controls in Tab order: the reference gallery's
#: (`tui/ratatui/gallery/src/interact.rs`, `focusables`), reading order as Qt's gallery walks them.
FOCUSABLES = {
    "buttons": ["plain", "bordered", "icon", "power", "align"],
    "toggles": ["switch-0", "switch-1", "wifi"],
    "inputs": ["search", "name", "gaps", "theme"],
    "slider": ["level", "stepped"],
    "surfaces": ["network", "confirm", "add-layer", "night-light"],
    "lists": ["rows"],
    "tiles": ["tile"],
    "messages": ["scan"],
    "menus": ["menu", "context"],
    "navigation": ["tabs", "rail"],
    "forms": ["night", "check-0", "check-1", "check-4", "radio", "password"],
    "pickers": ["notify", "swatch", "discard"],
    "data": ["table", "tree"],
    "layout": ["sidebar", "handle", "cal"],
    "timeline": ["timeline", "scrubber", "drag"],
    "color": ["picker"],
    "composed": ["networks", "searchlist"],
}


class FocusOrder(unittest.IsolatedAsyncioTestCase):
    async def test_tab_walks_every_section_and_wraps(self):
        """Tab walks each section's controls in the reference's order and wraps, Shift+Tab walks
        back: no control keeps Tab (a list's sections, a dialog's buttons)."""
        for section, want in FOCUSABLES.items():
            for key, order in (("tab", want), ("shift+tab", want[::-1])):
                async with gallery(section) as (app, pilot):
                    seen = []
                    for _ in range(2 * len(want)):
                        await pilot.press(key)
                        seen.append(app.focused.id if app.focused else None)
                    self.assertEqual(seen, order + order, f"{key} in {section}")


if __name__ == "__main__":
    unittest.main()
