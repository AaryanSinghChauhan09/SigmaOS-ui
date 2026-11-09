#!/usr/bin/env python3
"""A tiny Omarchy panel in Python (PyGObject), following the live theme.

    python3 network.py                        a window; switch themes while it runs
    OMARCHY_GTK_SNAP=out.png python3 network.py    render it once and exit

Needs the Omarchy-1 typelib (GI_TYPELIB_PATH when it isn't installed)."""
import sys

import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Omarchy", "1")
from gi.repository import Gtk, Omarchy  # noqa: E402


def on_wifi(toggle):
    toggle.props.description = "On" if toggle.props.checked else "Off"


def on_changed(theme, tokens):
    # The changed hook: once per theme switch, with the new tokens (a dict-shaped GLib.Variant).
    print("theme changed:", tokens.unpack()["palette"]["background"], "revision", theme.props.revision, file=sys.stderr)


def activate(app):
    theme = Omarchy.init()  # the one call: Omarchy's look on GTK and libadwaita, following the desktop
    theme.connect("changed", on_changed)
    pad = theme.get_spacing("panelPadding")

    pane = Omarchy.Pane(bordered=True)
    pane.append(Omarchy.Heading(text="Network", subtitle="Wi-Fi and saved networks", level="heading"))
    wifi = Omarchy.Toggle(label="Wi-Fi", description="On", checked=True)
    wifi.connect("toggled", on_wifi)
    pane.append(wifi)
    row = Omarchy.ListRow(title="Basecamp", glyph="\U000F0928", value="5 GHz", current=True)
    row.set_status("Connected", "success")
    pane.append(row)
    pane.append(Omarchy.Button(label="Forget network", bordered=True, halign=Gtk.Align.START))
    pane.set_margin_start(pad)
    pane.set_margin_end(pad)
    pane.set_margin_top(pad)
    pane.set_valign(Gtk.Align.START)

    window = Gtk.ApplicationWindow(application=app, title="Network", default_width=400, default_height=340)
    window.set_child(pane)
    window.present()


app = Gtk.Application(application_id="org.omarchy.examples.NetworkPython")
app.connect("activate", activate)
sys.exit(app.run([]))
