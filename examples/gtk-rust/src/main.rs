//! A tiny Omarchy panel in gtk4-rs, following the live theme.
//!
//!   cargo run --release                         a window; switch themes while it runs
//!   OMARCHY_GTK_SNAP=out.png cargo run          render it once and exit

use omarchy_gtk::gtk::{self, glib, prelude::*};
use omarchy_gtk::{Button, Heading, ListRow, Pane, Toggle};

fn build(app: &gtk::Application) {
    // The one call: Omarchy's look on GTK and libadwaita, following the desktop.
    let theme = omarchy_gtk::init();
    let pad = theme.spacing("panelPadding");

    let pane = Pane::new("");
    pane.set_property("bordered", true);
    let heading = Heading::new("Network");
    heading.set_property("subtitle", "Wi-Fi and saved networks");
    heading.set_property("level", "heading");
    pane.append(&heading);

    let wifi = Toggle::new("Wi-Fi", "On");
    wifi.set_checked(true);
    wifi.connect_toggled(|t| t.set_property("description", if t.is_checked() { "On" } else { "Off" }));
    pane.append(&wifi);

    let row: ListRow = glib::Object::builder()
        .property("title", "Basecamp")
        .property("glyph", "\u{F0928}")
        .property("value", "5 GHz")
        .property("current", true)
        .build();
    row.set_property("status", "Connected");
    row.set_property("status-tone", "success");
    pane.append(&row);

    let forget = Button::new("Forget network");
    forget.set_property("bordered", true);
    forget.set_halign(gtk::Align::Start);
    pane.append(&forget);

    pane.set_margin_start(pad);
    pane.set_margin_end(pad);
    pane.set_margin_top(pad);
    pane.set_valign(gtk::Align::Start);

    // The changed hook: once per theme switch, with the new tokens.
    theme.connect_changed(|theme, tokens| {
        eprintln!("theme changed (revision {}): background {}", theme.revision(), tokens.palette_background());
    });

    gtk::ApplicationWindow::builder()
        .application(app)
        .title("Network")
        .default_width(400)
        .default_height(340)
        .child(&pane)
        .build()
        .present();
}

fn main() -> glib::ExitCode {
    let app = gtk::Application::builder()
        .application_id("org.omarchy.examples.NetworkRust")
        .flags(gtk::gio::ApplicationFlags::NON_UNIQUE)
        .build();
    app.connect_activate(build);
    app.run()
}
