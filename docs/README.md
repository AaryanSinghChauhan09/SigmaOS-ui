# Using Omarchy UI in an app

Omarchy UI is meant to be used from your own app, not copied into it. Pick your toolkit:

| You're building with | Read | Ways in |
|---|---|---|
| Qt Quick: C++, the `qml` tool, PySide6, Quickshell, Kirigami | [using-qt.md](using-qt.md) | the installed QML modules (Arch package or `make install`), or vendoring `qt/omarchy-ui.pri` |
| GTK 4 / libadwaita: C, Python, GJS, Vala, Rust | [using-gtk.md](using-gtk.md) | `libomarchy-gtk-1` (`pkg-config omarchy-gtk-1`, GI namespace `Omarchy`), one `omarchy_gtk_init()` / `Omarchy.init()`; Rust through `gtk/rust/omarchy-gtk` |
| Qt Widgets, KDE Frameworks, Kirigami | [using-kde.md](using-kde.md) | `OmarchyWidgets` (CMake `find_package` or `pkg-config omarchy-widgets`), one `Omarchy::Widgets::apply(app)` |
| Rune | [using-rust.md](using-rust.md) | `omarchy-rune` as a Cargo path or git dependency, one `omarchy_rune::run(spec)` |
| GPUI, with or without gpui-component | [using-gpui.md](using-gpui.md) | `omarchy-gpui` as a Cargo path dependency, one `omarchy_gpui::init(cx)` |
| Electron, or a web UI in any Chromium shell | [using-web.md](using-web.md) | `@omarchy/ui` (by path; not on npm), `followOmarchy()` in main plus the preload; `<omarchy-*>` elements for React, Vue, Svelte or plain HTML |
| Tauri v2 (WebKitGTK on Linux) | [using-tauri.md](using-tauri.md) | `tauri-plugin-omarchy` as a Cargo path or git dependency, one `.plugin(tauri_plugin_omarchy::init())`; `import "@omarchy/ui"` in the page |
| Ratatui, or any terminal UI | [using-ratatui.md](using-ratatui.md), [using-terminal.md](using-terminal.md) | `omarchy-ratatui` as a Cargo path dependency, one `omarchy_ratatui::run(app)`; the cell-grid contract every terminal binding follows |
| Any other Rust UI (iced, egui) | [using-rust.md](using-rust.md#any-other-rust-framework-omarchy-theme) | `omarchy-theme`: tokens, live watch, icons, strings |
| C, C++, anything with a C FFI | [using-c.md](using-c.md) | `libomarchy-theme` (`pkg-config omarchy-theme`): tokens, a main-loop watch, icons, strings |
| Node, scripts, anything that spawns a process | [using-c.md](using-c.md#the-cli-stream) | `omarchy-theme --watch --state`: one JSON line per change |

Each route has a working example under [`examples/`](../examples), which `bin/test` builds and renders on every run:

- [`examples/qt-installed`](../examples/qt-installed): one QML file run with the `qml` tool against the installed modules.
- [`examples/qt-vendored`](../examples/qt-vendored): a qmake app that includes `qt/omarchy-ui.pri`.
- [`examples/rune`](../examples/rune): a Rune app outside this workspace with a path dependency on `omarchy-rune`.
- [`examples/gtk-c`](../examples/gtk-c), [`gtk-python`](../examples/gtk-python), [`gtk-gjs`](../examples/gtk-gjs), [`gtk-vala`](../examples/gtk-vala), [`gtk-rust`](../examples/gtk-rust): the same GTK panel from each language (Python, GJS and Vala need `gobject-introspection` and `vala` installed; they skip otherwise).
- [`examples/kde-widgets`](../examples/kde-widgets), [`kde-kirigami`](../examples/kde-kirigami): a Qt Widgets app through CMake, and a Kirigami app following the theme.
- [`examples/gpui`](../examples/gpui): a GPUI app with a path dependency on `omarchy-gpui`.
- [`examples/ratatui`](../examples/ratatui): a Ratatui app with a path dependency on `omarchy-ratatui`, snapshotted as ANSI text.
- [`examples/web-electron`](../examples/web-electron), [`web-plain`](../examples/web-plain): an Electron app and a plain HTML page on `@omarchy/ui`.
- [`examples/tauri`](../examples/tauri): a Tauri v2 app on `tauri-plugin-omarchy` and `@omarchy/ui`.

What the kit contains, and the numbers behind it:

- [`spec/components.md`](../spec/components.md) and [`spec/components/`](../spec/components): every component, its name in each binding, its states and metrics.
- [`spec/terminal.md`](../spec/terminal.md): the same tokens and components on a terminal's cell grid (colors at 24-bit, 256 and 16, metrics in cells, every section's size, the snapshot format).
- [`spec/tokens.md`](../spec/tokens.md): the token contract (colors, states, borders, rounding, spacing, type, motion) and the `OMARCHY_UI_*` overrides.
- [`spec/following.md`](../spec/following.md): how every binding follows the theme while an app runs: the changed hook, the revision, the 280 ms color fade on a theme switch.
- [`AGENTS.md`](../AGENTS.md#a-new-framework): what a new binding (iced, egui, wxWidgets, Flutter…) needs, and its registry entry.
- [`spec/gallery.md`](../spec/gallery.md): what each gallery section shows. The galleries (`bin/build`, then each binding's `gallery.window` in `tools/bindings.d/`, e.g. `build/qt/omarchy-qt-gallery`) are the quickest way to browse the components in your own theme.
- [`AGENTS.md`](../AGENTS.md): the rules that make an app read as Omarchy (sharp corners from Hyprland, foreground glass, one font, flat, keyboard-first, live theme).
