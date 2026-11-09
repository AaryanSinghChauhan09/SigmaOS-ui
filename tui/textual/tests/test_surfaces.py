"""Behavior tests for the Surfaces section (the keys and clicks of
`tui/ratatui/gallery/src/interact.rs`: the network toggle, the confirm dialog, the pane actions),
through Textual's Pilot."""

from __future__ import annotations

import unittest

from support import gallery, posted

from omarchy_textual.widgets import Button, ConfirmDialog, Dialog, Toggle, ToggleSwitch


class Surfaces(unittest.IsolatedAsyncioTestCase):
    async def test_tab_order(self):
        async with gallery("surfaces") as (app, pilot):
            seen = []
            for _ in range(4):
                await pilot.press("tab")
                seen.append(app.focused.id)
            self.assertEqual(seen, ["network", "confirm", "add-layer", "night-light"])

    async def test_toggles_and_actions(self):
        async with gallery("surfaces") as (app, pilot):
            network = app.query_one("#network", Toggle)
            network.focus()
            await pilot.press("space")
            self.assertFalse(network.value)
            await pilot.press("enter")
            self.assertTrue(network.value)
            night = app.query_one("#night-light", ToggleSwitch)
            await pilot.click("#night-light")
            self.assertFalse(night.value)
            await pilot.click("#add-layer")
            await pilot.pause()
            self.assertEqual([m.button.id for m in posted(app, Button.Pressed)], ["add-layer"])

    async def test_confirm_dialog_chooses(self):
        async with gallery("surfaces") as (app, pilot):
            confirm = app.query_one("#confirm", ConfirmDialog)
            confirm.focus()
            await pilot.press("left")
            self.assertEqual(confirm.selected, 0)
            await pilot.press("l")
            self.assertEqual(confirm.selected, 1)
            await pilot.press("enter")
            r = confirm.rect_of((confirm, "cancel"))
            root = confirm.kit_root.content_region
            await pilot.click(confirm, offset=(root.x + r.x + 2 - confirm.region.x, root.y + r.y - confirm.region.y))
            self.assertEqual(confirm.selected, 0)
            await pilot.pause()
            self.assertEqual([m.index for m in posted(app, Dialog.Chosen)], [1, 0])


if __name__ == "__main__":
    unittest.main()
