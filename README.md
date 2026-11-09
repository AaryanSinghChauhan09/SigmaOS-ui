# omarchy-ui

A cross-framework UI kit for apps that look and feel like Omarchy, in any toolkit. It has three parts:

- **One token contract** ([`spec/tokens.md`](spec/tokens.md)): colors, interactive states, borders, rounding, spacing, type and motion, resolved from the live Omarchy theme (`colors.toml`, `shell.toml`, the user's overrides, Hyprland's rounding and gaps, fontconfig's `monospace`) and followed live through theme switches.
- **The same primitives in every framework** ([`spec/components.md`](spec/components.md)): Button, Toggle, TextField, Dropdown, Slider, Card, MenuRow, ConfirmDialog and the rest, with the same names, states and metrics.
- **A way to check they agree** ([`bin/review`](bin/review)): token parity for every theme, plus every gallery section rendered in each framework, side by side.

| Binding | Where | Status |
|---|---|---|
| Qt Quick | [`qt/`](qt) | The Omarchy shell's own QML kit, harvested from `quattro` ([`qt/HARVEST.md`](qt/HARVEST.md)) with its Quickshell I/O replaced by a small C++ `ThemeSource`. |
| Rune | [`rune/`](rune) | `omarchy-rune`: a rune-kit `Kit` built from the live theme, plus the Omarchy primitives drawn to match Qt. |
| Rust, any framework | [`crates/omarchy-theme`](crates/omarchy-theme) | The resolver and watcher, with no UI dependency. Start a GPUI, iced or egui binding here. |
| C, any language | [`crates/omarchy-theme-ffi`](crates/omarchy-theme-ffi) | `libomarchy-theme`: the same resolver and a main-loop watch as a C library (`omarchy-theme.h`, `pkg-config omarchy-theme`), and the `omarchy-theme` CLI's line-per-change stream. Start a GTK, Qt Widgets or Electron binding here ([`docs/using-c.md`](docs/using-c.md)). |

## Try it

```sh
bin/build
build/qt/omarchy-qt-gallery                  # follows your theme; switch themes and watch
target/release/omarchy-rune-gallery
bin/review --open                            # every binding vs Qt, 6 themes × 2 roundings, every section
target/release/omarchy-theme                 # the resolved tokens as JSON
```

Any of these can render a fixture theme instead of the desktop's: `OMARCHY_UI_THEME_DIR=fixtures/themes/nord OMARCHY_UI_ROUNDING=8 …`.

## Use it

It's a library: apps depend on it rather than copying it. [`docs/`](docs/README.md) covers each route, with a minimal app, the live theme, previews in other themes, snapshots, pinning and troubleshooting:

- **Qt Quick** ([`docs/using-qt.md`](docs/using-qt.md)): install the QML modules (the Arch package in [`packaging/arch/omarchy-ui`](packaging/arch/omarchy-ui), or `make install` from [`qt/module`](qt/module)), and `import Omarchy.Ui as O` works in any QML app: the `qml` tool, C++, PySide6, Quickshell. Or vendor it with `include(omarchy-ui/qt/omarchy-ui.pri)` and one `OmarchyUi::registerTypes()` call.
- **Rune** ([`docs/using-rust.md`](docs/using-rust.md)): one path or git dependency on `omarchy-rune`, which re-exports the `rune` it's built on.
- **Any other Rust UI** ([`docs/using-rust.md`](docs/using-rust.md#any-other-rust-framework-omarchy-theme)): `omarchy-theme` gives tokens, a live watch, icons and strings.
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
