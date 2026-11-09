//! A tiny Omarchy panel in Tauri, following the live theme (the Tauri twin of examples/web-electron
//! and examples/rune).
//!
//!   cargo run --release                          a window; switch themes while it runs
//!   cargo run --release --features snap -- --snap out.png
//!                                                render once, offscreen, to a PNG, and exit
//!
//! The whole adoption: `.plugin(tauri_plugin_omarchy::init())` here, and `import "@omarchy/ui"` in
//! the page (dist/app.js).

fn main() {
    #[cfg(feature = "snap")]
    let snap = tauri_plugin_omarchy::snap::from_args(); // first: it starts a private display

    #[allow(unused_mut)]
    let mut app = tauri::Builder::default().plugin(tauri_plugin_omarchy::init());

    #[cfg(feature = "snap")]
    if let Some(snap) = snap {
        app = app.plugin(snap.plugin());
    }

    app.run(tauri::generate_context!()).expect("tauri");
}
