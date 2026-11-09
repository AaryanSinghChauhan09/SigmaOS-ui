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

#: The module (gallery/sections/<module>.py) holding each section's `Section` subclass, named
#: after the section (`Buttons`, `Toggles`, ...).
MODULE = {
    "buttons": "controls",
    "toggles": "controls",
    "inputs": "controls",
    "slider": "controls",
    "forms": "forms",
    "pickers": "forms",
    "surfaces": "surfaces",
    "menus": "navigation",
    "navigation": "navigation",
    "lists": "lists",
    "tiles": "lists",
    "progress": "feedback",
    "messages": "feedback",
    "data": "data",
    "layout": "data",
    "timeline": "editing",
    "color": "editing",
    "composed": "composed",
    "tokens": "tokens",
}


def section_class(name: str):
    """The `Section` subclass drawing `name` (a placeholder while it isn't written)."""
    try:
        mod = importlib.import_module(f".{MODULE[name]}", __name__)
        return getattr(mod, name.capitalize())
    except (ImportError, AttributeError):
        from ..page import Placeholder

        title = dict(SECTIONS)[name]
        return type(name.capitalize(), (Placeholder,), {"NAME": name, "TITLE": title})
