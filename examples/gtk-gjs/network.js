#!/usr/bin/env -S gjs -m
// A tiny Omarchy panel in GJS, following the live theme.
//
//   gjs -m network.js                         a window; switch themes while it runs
//   OMARCHY_GTK_SNAP=out.png gjs -m network.js     render it once and exit
//
// Needs the Omarchy-1 typelib (GI_TYPELIB_PATH when it isn't installed).
import Gtk from "gi://Gtk?version=4.0";
import Omarchy from "gi://Omarchy?version=1";

const app = new Gtk.Application({ application_id: "org.omarchy.examples.NetworkGjs" });
app.connect("activate", () => {
  const theme = Omarchy.init(); // the one call: Omarchy's look on GTK and libadwaita, following the desktop
  theme.connect("changed", (t, tokens) =>
    printerr(`theme changed: ${tokens.recursiveUnpack().palette.background}, revision ${t.revision}`));
  const pad = theme.get_spacing("panelPadding");

  const pane = new Omarchy.Pane({ bordered: true });
  pane.append(new Omarchy.Heading({ text: "Network", subtitle: "Wi-Fi and saved networks", level: "heading" }));
  const wifi = new Omarchy.Toggle({ label: "Wi-Fi", description: "On", checked: true });
  wifi.connect("toggled", (t) => { t.description = t.checked ? "On" : "Off"; });
  pane.append(wifi);
  const row = new Omarchy.ListRow({ title: "Basecamp", glyph: "\u{F0928}", value: "5 GHz", current: true });
  row.set_status("Connected", "success");
  pane.append(row);
  pane.append(new Omarchy.Button({ label: "Forget network", bordered: true, halign: Gtk.Align.START }));
  pane.set_margin_start(pad);
  pane.set_margin_end(pad);
  pane.set_margin_top(pad);
  pane.set_valign(Gtk.Align.START);

  const window = new Gtk.ApplicationWindow({ application: app, title: "Network", default_width: 400, default_height: 340 });
  window.set_child(pane);
  window.present();
});
app.run([]);
