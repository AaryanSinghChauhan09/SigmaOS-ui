# omarchy-ui

A cross-framework UI kit for apps that look and feel like Omarchy, in any toolkit. It has three parts:

- **One token contract** ([`spec/tokens.md`](spec/tokens.md)): colors, interactive states, borders, rounding, spacing, type and motion, resolved from the live Omarchy theme (`colors.toml`, `shell.toml`, the user's overrides, Hyprland's rounding and gaps, fontconfig's `monospace`) and followed live through theme switches.
- **The same primitives in every framework** ([`spec/components.md`](spec/components.md)): Button, Toggle, TextField, Dropdown, Slider, Card, MenuRow, ConfirmDialog and the rest, with the same names, states and metrics.
- **A way to check they agree** ([`bin/review`](bin/review)): token parity for every theme, plus every gallery section rendered in each framework, side by side.

| Binding | Where | Status |
|---|---|---|
| Qt Quick | [`qt/`](qt) | The Omarchy shell's own QML kit, harvested from `quattro` ([`qt/HARVEST.md`](qt/HARVEST.md)) with its Quickshell I/O replaced by a small C++ `ThemeSource`. |
| Rune | [`rune/`](rune) | `omarchy-rune`: a rune-kit `Kit` built from the live theme, plus the Omarchy primitives drawn to match Qt. |
| GTK 4 / libadwaita | [`gtk/`](gtk) | `libomarchy-gtk-1`: one `omarchy_gtk_init()` restyles stock GTK and libadwaita widgets and follows the theme; the full kit as GObject widgets, usable from C, Python, GJS, Vala (through GObject Introspection) and Rust (`gtk/rust/omarchy-gtk`). |
| KDE / Qt Widgets | [`kde/`](kde) | `OmarchyWidgets`: one `Omarchy::Widgets::apply(app)` sets the palette, a `QProxyStyle` for stock widgets, a live KDE color scheme for KF6 and Kirigami apps; the full kit as QWidgets. |
| GPUI | [`gpui/`](gpui) | `omarchy-gpui`: one `omarchy_gpui::init(cx)`, the full kit as GPUI elements, and gpui-component's theme mapped from the tokens. |
| Ratatui (terminal) | [`tui/ratatui/`](tui/ratatui) | `omarchy-ratatui`: one `omarchy_ratatui::run(app)`, the full kit on the terminal's cell grid ([`spec/terminal.md`](spec/terminal.md)), the reference for the terminal bindings. |
| Bubble Tea (terminal, Go) | [`tui/bubbletea/`](tui/bubbletea) | `omarchy-ui/tui/bubbletea`: one `omarchy.Run(theme, model)` feeds theme changes into the program as `omarchy.ThemeChangedMsg` and fades them; the full kit as Lip Gloss-style values, cell for cell with Ratatui. |
| Electron / web | [`web/`](web) | `@omarchy/ui`: CSS custom properties, a restyle for plain HTML controls, the full kit as Web Components, and `followOmarchy()` for Electron. |
| Tauri | [`web/tauri/`](web/tauri) | `tauri-plugin-omarchy`: one `.plugin(tauri_plugin_omarchy::init())` gives every webview the theme before first paint and every change, for `@omarchy/ui` in the page; checked on WebKitGTK. |
| Rust, any framework | [`crates/omarchy-theme`](crates/omarchy-theme) | The resolver and watcher, with no UI dependency. Start an iced or egui binding here. |
| C, any language | [`crates/omarchy-theme-ffi`](crates/omarchy-theme-ffi) | `libomarchy-theme`: the same resolver and a main-loop watch as a C library (`omarchy-theme.h`, `pkg-config omarchy-theme`), and the `omarchy-theme` CLI's line-per-change stream. Start a binding for any other toolkit here ([`docs/using-c.md`](docs/using-c.md)). |

## Try it

```sh
bin/build
build/qt/omarchy-qt-gallery                  # follows your theme; switch themes and watch
target/release/omarchy-rune-gallery
tui/ratatui/target/release/omarchy-ratatui-gallery   # the terminal kit, in this terminal
build/bubbletea/omarchy-bubbletea-gallery    # the same on Bubble Tea
bin/review --open                            # every binding vs Qt, 6 themes × 2 roundings, every section
target/release/omarchy-theme                 # the resolved tokens as JSON
```

Any of these can render a fixture theme instead of the desktop's: `OMARCHY_UI_THEME_DIR=fixtures/themes/nord OMARCHY_UI_ROUNDING=8 …`.

## Use it

It's a library: apps depend on it rather than copying it. [`docs/`](docs/README.md) covers each route, with a minimal app, the live theme, previews in other themes, snapshots, pinning and troubleshooting:

- **Qt Quick** ([`docs/using-qt.md`](docs/using-qt.md)): install the QML modules (the Arch package in [`packaging/arch/omarchy-ui`](packaging/arch/omarchy-ui), or `make install` from [`qt/module`](qt/module)), and `import Omarchy.Ui as O` works in any QML app: the `qml` tool, C++, PySide6, Quickshell. Or vendor it with `include(omarchy-ui/qt/omarchy-ui.pri)` and one `OmarchyUi::registerTypes()` call.
- **Rune** ([`docs/using-rust.md`](docs/using-rust.md)): one path or git dependency on `omarchy-rune`, which re-exports the `rune` it's built on.
- **GTK 4** ([`docs/using-gtk.md`](docs/using-gtk.md)): `pkg-config omarchy-gtk-1` from C, `gi.require_version("Omarchy", "1")` from Python, `gi://Omarchy` from GJS, `--pkg omarchy-gtk-1` from Vala, `omarchy-gtk` from Rust; then one `init` call.
- **KDE / Qt Widgets** ([`docs/using-kde.md`](docs/using-kde.md)): `find_package(OmarchyWidgets)` and one `Omarchy::Widgets::apply(app)`.
- **GPUI** ([`docs/using-gpui.md`](docs/using-gpui.md)): one path dependency on `omarchy-gpui` and one `omarchy_gpui::init(cx)`.
- **Ratatui** ([`docs/using-ratatui.md`](docs/using-ratatui.md)): one path dependency on `omarchy-ratatui` and one `omarchy_ratatui::run(app)`; any other terminal toolkit follows [`spec/terminal.md`](spec/terminal.md) ([`docs/using-terminal.md`](docs/using-terminal.md)).
- **Bubble Tea** ([`docs/using-bubbletea.md`](docs/using-bubbletea.md)): a `replace` on `omarchy-ui/tui/bubbletea` in `go.mod` and one `omarchy.Run(theme, model)`.
- **Electron / web** ([`docs/using-web.md`](docs/using-web.md)): `followOmarchy()` in main, the preload, and `import "@omarchy/ui"` in the renderer.
- **Tauri** ([`docs/using-tauri.md`](docs/using-tauri.md)): one path or git dependency on `tauri-plugin-omarchy`, one `.plugin(tauri_plugin_omarchy::init())`, and `import "@omarchy/ui"` in the page.
- **iced, egui, any other Rust UI** ([`docs/using-rust.md`](docs/using-rust.md#any-other-rust-framework-omarchy-theme)): `omarchy-theme` gives tokens, a live watch, icons and strings.
- **Anything else** ([`docs/using-c.md`](docs/using-c.md)): `libomarchy-theme` from C or any FFI, or `omarchy-theme --watch --state` from a spawned process.

Every binding follows the theme live the same way ([`spec/following.md`](spec/following.md)): no app code or one call, a changed hook, and a 280 ms color fade on a theme switch.

```qml
import Omarchy.Theme as Theme
import Omarchy.Ui as O

O.Card {
  title: "Network"
  O.Toggle { label: "Wi-Fi"; description: "Basecamp"; checked: wifi.on; onClicked: wifi.toggle() }
  O.Button { text: "Forget"; bordered: true }
}
```

[`examples/`](examples) has a working app for each route, built and rendered by `bin/test`.

[`spec/roadmap.md`](spec/roadmap.md) ranks the primitives still to build. [`AGENTS.md`](AGENTS.md) has the rules that make an app read as Omarchy, and how to work on the kit.
