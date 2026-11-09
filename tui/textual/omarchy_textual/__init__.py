"""Omarchy UI for Textual: the live Omarchy theme on a Textual app, followed without a restart,
and the kit's components as Textual widgets drawn on the cell grid the way spec/terminal.md maps
them.

    from omarchy_textual import OmarchyApp, OmarchyThemeChanged
    from omarchy_textual.widgets import Card, Toggle

    class Settings(OmarchyApp):
        def compose(self):
            with Card("Network"):
                yield Toggle("Wi-Fi", "Connected to Basecamp", True)

        def on_omarchy_theme_changed(self, event: OmarchyThemeChanged) -> None:
            ...  # the changed hook: event.tokens, event.revision
"""

from .app import Kit, OmarchyApp, OmarchyThemeChanged, follow, kit_of
from .cells.base import Tone
from .color import Rgba
from .depth import Depth
from .theme import Theme
from .tokens import Tokens

__all__ = ["Depth", "Kit", "OmarchyApp", "OmarchyThemeChanged", "Rgba", "Theme", "Tokens", "Tone", "follow", "kit_of"]
