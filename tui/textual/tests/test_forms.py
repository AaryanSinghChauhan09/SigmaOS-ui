"""Behavior tests for the forms and pickers (Ratatui's `forms_check_and_choose` and
`multiselect_keys`), through Textual's Pilot on the gallery."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets import Checkbox, Dialog, MultiSelect, PasswordField, RadioGroup, SwatchStrip, ToggleSwitch


def focus_order(app) -> list[str]:
    return [w.id for w in app.screen.focus_chain]


class Forms(unittest.IsolatedAsyncioTestCase):
    async def test_forms_check_and_choose(self):
        async with gallery("forms") as (app, pilot):
            self.assertEqual(focus_order(app), ["night", "check-0", "check-1", "check-4", "radio", "password"])
            await pilot.click("#check-0")
            self.assertTrue(app.query_one("#check-0", Checkbox).value)
            await pilot.click("#check-3")
            self.assertFalse(app.query_one("#check-3", Checkbox).value, "a disabled checkbox takes no clicks")
            radio = app.query_one("#radio", RadioGroup)
            await pilot.click("#radio", offset=(1, 2))
            self.assertEqual(radio.value, "hidden")
            radio.focus()
            await pilot.press("k")
            self.assertEqual(radio.value, "bottom")
            await pilot.press("up", "up")
            self.assertEqual(radio.value, "top")
            app.query_one("#check-1").focus()
            await pilot.press("space")
            self.assertFalse(app.query_one("#check-1", Checkbox).value)
            self.assertEqual(len(posted(app, Checkbox.Changed)), 2)

    async def test_setting_row_control_and_password(self):
        async with gallery("forms") as (app, pilot):
            night = app.query_one("#night", ToggleSwitch)
            await pilot.click("#night")
            self.assertFalse(night.value)
            pw = app.query_one("#password", PasswordField)
            pw.focus()
            await pilot.press("f2")
            self.assertTrue(pw.revealed)
            await pilot.click("#password", offset=(pw.region.width - 3, 0))
            self.assertFalse(pw.revealed, "the eye flips it")
            await pilot.press("backspace", "backspace", "4", "2")
            self.assertEqual(pw.value, "hunter42")


class Pickers(unittest.IsolatedAsyncioTestCase):
    async def test_multiselect_keys(self):
        async with gallery("pickers") as (app, pilot):
            self.assertEqual(focus_order(app), ["notify", "swatch", "discard"])
            notify = app.query_one("#notify", MultiSelect)
            notify.focus()
            await pilot.press("down", "enter")
            self.assertIn("calendar", notify.values)
            await pilot.press("j", "space")
            self.assertNotIn("mail", notify.values)
            await pilot.press("k", "k")
            self.assertIsNone(notify.row)
            await pilot.press("b", "a", "enter")
            self.assertIn("battery", notify.values)
            await pilot.press("escape")
            self.assertFalse(notify.expanded)
            self.assertEqual(notify.search, "")
            self.assertEqual(len(posted(app, MultiSelect.Changed)), 3)

    async def test_swatches_and_dialog_click(self):
        async with gallery("pickers") as (app, pilot):
            strip = app.query_one("#swatch", SwatchStrip)
            await pilot.click("#swatch", offset=(5 * 3, 0))
            self.assertEqual(strip.chosen, 5)
            strip.focus()
            await pilot.press("left", "enter")
            self.assertEqual(strip.chosen, 4)
            dialog = app.query_one("#discard", Dialog)
            r = dialog.rect_of((dialog, "cancel"))
            root = dialog.kit_root.content_region
            await pilot.click(offset=(root.x + r.x + 1, root.y + r.y))
            self.assertEqual(dialog.selected, 0)

    async def test_trigger_click_opens_and_rows_toggle(self):
        async with gallery("pickers") as (app, pilot):
            notify = app.query_one("#notify", MultiSelect)
            await pilot.click("#notify", offset=(2, 1))
            self.assertFalse(notify.expanded, "a click on the open trigger closes it")
            await pilot.click("#notify", offset=(2, 1))
            self.assertTrue(notify.expanded)
            # Rows start under the label, trigger, box edge, search and rule: Calendar at row 5.
            await pilot.click("#notify", offset=(3, 5))
            self.assertIn("calendar", notify.values)


if __name__ == "__main__":
    unittest.main()
