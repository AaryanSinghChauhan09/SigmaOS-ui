"""Progress, Messages."""

from __future__ import annotations

from textual.app import ComposeResult

from ...cells.base import Tone, g
from ...widgets import Box, Col, Row
from ...widgets.feedback import Banner, EmptyState, Meter, ProgressBar, Spinner, Toast
from ..page import CONTENT, Section


class Progress(Section):
    NAME, TITLE = "progress", "Progress"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=2):
                yield Meter(0.2, markers=[0.5, 0.8], width=28)
                yield Meter(0.65, markers=[0.5, 0.8], width=28)
                yield Meter(0.92, markers=[0.5, 0.8], alarming=True, width=28)
            yield Meter(0.65, label="Battery", value_text="65%", width=45)
            with Row(gap=2):
                yield ProgressBar(0.4, width=43)
                yield ProgressBar(indeterminate=True, width=43)
            with Row(gap=2):
                yield Spinner()
                yield Spinner()
                yield Spinner(pulse=True)
                yield Spinner(pulse=True)


class Messages(Section):
    NAME, TITLE = "messages", "Messages"

    def compose(self) -> ComposeResult:
        with Col(gap=1):
            with Row(gap=2):
                yield Box(
                    EmptyState("No networks found", glyph=g(0xF05AA), hint="Turn on Wi-Fi to scan", action="Scan",
                               width=44, id="scan"),
                    width=44, height=9, center=True,
                )
                yield Box(
                    EmptyState("Clipboard is empty", glyph=g(0xF014C), hint="Copy something to see it here",
                               overlay=True, width=44),
                    width=44, height=9, center=True,
                )
            with Col(gap=1):
                yield Banner("Omarchy 3.2 is available", tone=Tone.INFO, action="Update", dismissible=True,
                             width=CONTENT, id="banner-update")
                yield Banner("Backup finished", tone=Tone.SUCCESS, dismissible=True, width=CONTENT, id="banner-backup")
                yield Banner("Battery is low (12%)", tone=Tone.WARNING, dismissible=True, width=CONTENT,
                             id="banner-battery")
                yield Banner("Could not reach the update server", tone=Tone.ERROR, dismissible=True, width=CONTENT,
                             id="banner-error")
            yield Toast("Export finished", "omarchy-demo.mp4 · 42 MB", glyph=g(0xF012C), action="Open", countdown=0.6,
                        width=45, id="toast")
