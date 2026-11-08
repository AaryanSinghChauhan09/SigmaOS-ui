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

## Try it

```sh
bin/build
build/qt/omarchy-qt-gallery                  # follows your theme; switch themes and watch
target/release/omarchy-rune-gallery
bin/review --open                            # Qt vs Rune, 6 themes × 2 roundings, every section
target/release/omarchy-theme                 # the resolved tokens as JSON
```

Any of these can render a fixture theme instead of the desktop's: `OMARCHY_UI_THEME_DIR=fixtures/themes/nord OMARCHY_UI_ROUNDING=8 …`.

## Use it

Qt (qmake):

```qml
// app.pro: include(../omarchy-ui/qt/omarchy-ui.pri)   main.cpp: OmarchyUi::registerTypes();
import Omarchy.Theme as Theme
import Omarchy.Ui as O

O.Card {
  title: "Network"
  O.Toggle { label: "Wi-Fi"; description: "Basecamp"; checked: wifi.on; onClicked: wifi.toggle() }
  O.Button { text: "Forget"; bordered: true }
}
```

Rune:

```rust
use omarchy_rune::{Theme, ui::*};

fn view(s: &App, cx: &mut Cx<App>) -> El {
    let t = &s.theme;
    t.apply(cx);
    Card::new().title("Network")
        .child(Toggle::new("Wi-Fi").description("Basecamp").checked(s.wifi).on_toggle(|s: &mut App| s.wifi ^= true).build(t))
        .child(Button::new("Forget").bordered(true).build(t))
        .build(t)
}
// AppSpec { setup: Some(|s, wake| s.theme.follow(wake)), poll: Some(|s, _, _| s.theme.poll()), .. }
```

[`AGENTS.md`](AGENTS.md) has the rules that make an app read as Omarchy, and how to work on the kit.
