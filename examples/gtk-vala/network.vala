// A tiny Omarchy panel in Vala, following the live theme.
//
//   valac --pkg gtk4 --pkg omarchy-gtk-1 network.vala && ./network
//   OMARCHY_GTK_SNAP=out.png ./network      render it once and exit
//
// The omarchy-gtk-1 vapi comes from Omarchy-1.gir (vapigen), which needs g-ir-scanner.
int main (string[] args) {
    var app = new Gtk.Application ("org.omarchy.examples.NetworkVala", ApplicationFlags.NON_UNIQUE);
    app.activate.connect (() => {
        var theme = Omarchy.init (); // the one call: Omarchy's look on GTK and libadwaita, following the desktop
        theme.changed.connect ((tokens) => {
            stderr.printf ("theme changed, revision %llu\n", theme.revision);
        });
        int pad = theme.get_spacing ("panelPadding");

        var pane = new Omarchy.Pane (null);
        pane.bordered = true;
        var heading = new Omarchy.Heading ("Network");
        heading.subtitle = "Wi-Fi and saved networks";
        heading.level = "heading";
        pane.append (heading);
        var wifi = new Omarchy.Toggle ("Wi-Fi", "On");
        wifi.checked = true;
        wifi.toggled.connect (() => { wifi.description = wifi.checked ? "On" : "Off"; });
        pane.append (wifi);
        var row = new Omarchy.ListRow ("Basecamp");
        row.glyph = "\U000F0928";
        row.value = "5 GHz";
        row.current = true;
        row.set_status ("Connected", "success");
        pane.append (row);
        var forget = new Omarchy.Button ("Forget network");
        forget.bordered = true;
        forget.halign = Gtk.Align.START;
        pane.append (forget);
        pane.margin_start = pad;
        pane.margin_end = pad;
        pane.margin_top = pad;
        pane.valign = Gtk.Align.START;

        var window = new Gtk.ApplicationWindow (app);
        window.title = "Network";
        window.set_default_size (400, 340);
        window.child = pane;
        window.present ();
    });
    return app.run (args);
}
