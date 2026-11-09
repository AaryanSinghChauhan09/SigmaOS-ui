"""Every gallery section, in spec/gallery.md order: (name, title, Section class)."""

from __future__ import annotations

import importlib

#: (name, title) in order.
SECTIONS: list[tuple[str, str]] = [
    ("buttons", "Buttons"),
    ("toggles", "Toggles"),
    ("inputs", "Inputs"),
    ("slider", "Slider"),
    ("surfaces", "Surfaces"),
    ("lists", "Lists"),
    ("tiles", "Tiles"),
    ("progress", "Progress"),
    ("messages", "Messages"),
    ("menus", "Menus"),
    ("navigation", "Navigation"),
    ("forms", "Forms"),
    ("pickers", "Pickers"),
    ("data", "Data"),
    ("layout", "Layout"),
    ("timeline", "Timeline"),
    ("color", "Color"),
    ("composed", "Composed"),
    ("tokens", "Tokens"),
]

#: The section whose module isn't written yet draws as a placeholder.
_MODULE = {
    "buttons": "controls",
    "toggles": "controls",
    "inputs": "controls",
    "slider": "controls",
}


def section_class(name: str):
    """The `Section` subclass drawing `name`."""
    mod = importlib.import_module(f".{_MODULE.get(name, name)}", __name__)
    return getattr(mod, name.capitalize())
