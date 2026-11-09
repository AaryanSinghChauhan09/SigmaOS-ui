# Using Omarchy UI in an app

Omarchy UI is meant to be used from your own app, not copied into it. Pick your toolkit:

| You're building with | Read | Ways in |
|---|---|---|
| Qt Quick: C++, the `qml` tool, PySide6, Quickshell | [using-qt.md](using-qt.md) | the installed QML modules (Arch package or `make install`), or vendoring `qt/omarchy-ui.pri` |
| Rune | [using-rust.md](using-rust.md) | `omarchy-rune` as a Cargo path or git dependency |
| Any other Rust UI (GPUI, iced, egui) | [using-rust.md](using-rust.md#any-other-rust-framework-omarchy-theme) | `omarchy-theme`: tokens, live watch, icons, strings |
| C, C++, GTK, Qt Widgets, Python, Vala, anything with a C FFI | [using-c.md](using-c.md) | `libomarchy-theme` (`pkg-config omarchy-theme`): tokens, a main-loop watch, icons, strings |
| Node, Electron, scripts, anything that spawns a process | [using-c.md](using-c.md#the-cli-stream) | `omarchy-theme --watch --state`: one JSON line per change |

Each route has a working example under [`examples/`](../examples), which `bin/test` builds and renders on every run:

- [`examples/qt-installed`](../examples/qt-installed): one QML file run with the `qml` tool against the installed modules.
- [`examples/qt-vendored`](../examples/qt-vendored): a qmake app that includes `qt/omarchy-ui.pri`.
- [`examples/rune`](../examples/rune): a Rune app outside this workspace with a path dependency on `omarchy-rune`.

What the kit contains, and the numbers behind it:

- [`spec/components.md`](../spec/components.md) and [`spec/components/`](../spec/components): every component, its name in each binding, its states and metrics.
- [`spec/tokens.md`](../spec/tokens.md): the token contract (colors, states, borders, rounding, spacing, type, motion) and the `OMARCHY_UI_*` overrides.
- [`spec/following.md`](../spec/following.md): how every binding follows the theme while an app runs: the changed hook, the revision, the 280 ms color fade on a theme switch.
- [`AGENTS.md`](../AGENTS.md#a-new-framework): what a new binding (GTK, Qt Widgets, GPUI, Electron) needs, and its registry entry.
- [`spec/gallery.md`](../spec/gallery.md): what each gallery section shows. The galleries (`bin/build`, then `build/qt/omarchy-qt-gallery` or `target/release/omarchy-rune-gallery`) are the quickest way to browse the components in your own theme.
- [`AGENTS.md`](../AGENTS.md): the rules that make an app read as Omarchy (sharp corners from Hyprland, foreground glass, one font, flat, keyboard-first, live theme).
