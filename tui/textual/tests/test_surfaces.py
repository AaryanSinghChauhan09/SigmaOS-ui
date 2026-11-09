"""Behavior tests for the Surfaces section (the keys and clicks of
`tui/ratatui/gallery/src/interact.rs`: the network toggle, the confirm dialog, the pane actions),
through Textual's Pilot."""

from __future__ import annotations

import unittest

from support import fixture, gallery, posted

from omarchy_textual import OmarchyApp
from omarchy_textual import cells as C
from omarchy_textual.widgets import Button, ConfirmDialog, Dialog, SectionHeader, Toggle, ToggleSwitch


def rows_of(t, d) -> list[str]:
    """The text of each row of a cells component drawn alone at its size."""
    w, h = d.size(t)
    buf = C.Buffer(C.Rect(0, 0, w, h))
    d.draw(t, buf.area, buf)
    return ["".join(c.sym for c in buf.row(y)) for y in range(h)]


def row_with(rows: list[str], s: str) -> int:
    return next((i for i, r in enumerate(rows) if s in r), -1)


class DialogBody(OmarchyApp):
    AUTO_FOCUS = None

    def compose(self):
        yield Dialog("Title", "Message", body=SectionHeader("Body"), width=30, scrim=False)


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


    async def test_dialog_body_sits_a_row_under_the_message(self):
        """Title, message and body a blank row apart (the body has no specimen in the gallery);
        with only a body it starts at the padding. The widget composes its body widget the same."""
        t = fixture("tokyo-night")
        rows = rows_of(t, C.Dialog("Title", "Message", body=C.SectionHeader("Body"), width=30, scrim=False))
        self.assertEqual([row_with(rows, s) for s in ("Title", "Message", "Body", "Confirm")], [2, 4, 6, 8], rows)
        self.assertEqual(len(rows), 11)
        rows = rows_of(t, C.Dialog("", body=C.SectionHeader("Body"), width=30, scrim=False))
        self.assertEqual([row_with(rows, s) for s in ("Body", "Confirm")] + [len(rows)], [2, 4, 7], rows)
        app = DialogBody(theme=t)
        app.kit.live = False
        async with app.run_test(size=(30, 11)) as pilot:
            await pilot.pause()
            strips = app.screen._compositor.render_strips()
            rows = ["".join(seg.text for seg in strip) for strip in strips]
            self.assertEqual([row_with(rows, s) for s in ("Title", "Message", "Body", "Confirm")], [2, 4, 6, 8], rows)


if __name__ == "__main__":
    unittest.main()
