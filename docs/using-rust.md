# Using Omarchy UI from Rust

> `<omarchy-ui-repo>` below stands for this repository's clone URL. It has no public remote yet, so substitute wherever you cloned it from. Paths such as `examples/...` are relative to a checkout and work as written.

Two crates, neither on crates.io. Depend on them through Cargo by path or git:

| Crate | For | What it gives you |
|---|---|---|
| `omarchy-rune` (`rune/omarchy-rune`) | Rune apps | `Theme` (the live theme, plus a rune-kit `Kit` built from it), `ui::*` (Omarchy's primitives, drawn to match the Qt kit), and re-exports of `rune`, `rune_kit` and `omarchy_theme` |
| `omarchy-theme` (`crates/omarchy-theme`) | any other Rust UI, or no UI | `load()` → `Tokens`, `watch()`, `icons`, `strings`; no UI dependency |

## Rune

### Add the dependency

From a checkout next to your app, or a submodule:

```toml
[dependencies]
omarchy-rune = { path = "../omarchy-ui/rune/omarchy-rune" }
```

From the repository, pinned to a commit:

```toml
[dependencies]
omarchy-rune = { git = "<omarchy-ui-repo>", rev = "<commit>" }
```

That one line is all. Don't add `rune` or `rune-kit` yourself: use `omarchy_rune::rune` and `omarchy_rune::rune_kit`, which are exactly the Rune `omarchy-rune` was built against. Rune is pre-release and pinned to one git rev in omarchy-ui's `Cargo.toml`. A second `rune` dependency at any other rev is a second, incompatible crate, and you get errors like ``expected `rune::El`, found `rune::El` ``. If you need Rune directly (a feature flag, say), copy the `rev` from omarchy-ui's `[workspace.dependencies]` and update it whenever you update omarchy-ui.

`omarchy-rune`'s manifest inherits from omarchy-ui's workspace (`edition.workspace`, `rune.workspace`, ...). Cargo resolves that against omarchy-ui's root `Cargo.toml` for a path dependency and a git dependency alike, so your crate needs no workspace of its own and nothing copied from it. [`examples/rune`](../examples/rune) builds that way on every `bin/test`. A git dependency on a local clone resolves the same way.

If your crate sits inside another Cargo workspace's directory without being a member, Cargo stops with "current package believes it's in a workspace when it's not". Add it to that workspace's `exclude` (as omarchy-ui does for `examples/rune`) or give it its own `[workspace]`.

### A minimal app

[`examples/rune/src/main.rs`](../examples/rune/src/main.rs):

```rust
use omarchy_rune::Theme;
use omarchy_rune::rune::{self, AppSpec, Cx, El, col, row};
use omarchy_rune::ui::{Button, Heading, HeadingLevel, ListRow, Pane, Toggle, Tone};

const WIDTH: f32 = 400.0;

struct App {
    theme: Theme,
    wifi: bool,
}

fn view(s: &App, cx: &mut Cx<App>) -> El {
    let t = &s.theme;
    t.apply(cx); // first, every frame
    let pad = t.tokens().spacing.panel_padding as f32;
    let inner = WIDTH - 4.0 * pad;

    let panel = Pane::new()
        .bordered(true)
        .width(WIDTH - 2.0 * pad)
        .child(Heading::new("Network").subtitle("Wi-Fi and saved networks").level(HeadingLevel::Heading).build(t))
        .child(
            Toggle::new("Wi-Fi")
                .description(if s.wifi { "On" } else { "Off" })
                .checked(s.wifi)
                .width(inner)
                .on_toggle(|s: &mut App| s.wifi ^= true)
                .build(t),
        )
        .child(ListRow::new("Basecamp").glyph("\u{F0928}").status("Connected", Tone::Success).current(s.wifi).width(inner).build(t))
        .child(row().child(Button::new("Forget network").bordered(true).build(t)))
        .build(t);

    col().w_full().h_full().fill(t.bg()).padding(pad).child(panel)
}

fn init() -> App {
    App { theme: Theme::load(), wifi: true }
}

fn main() {
    rune::run(AppSpec {
        families: Theme::load().families(),
        setup: Some(|s: &mut App, wake| s.theme.follow(wake)),
        poll: Some(|s: &mut App, _, _| s.theme.poll()),
        ..AppSpec::new("Network", (WIDTH, 283.0), init, view)
    });
}
```

```sh
cargo run --release
```

Name the state type in the `setup` and `poll` closures (`|s: &mut App, ...|`). Rust checks those fields before `AppSpec::new(...)`, so it can't infer it from there.

### The pieces

- **`Theme`** lives in your app state. `Theme::load()` resolves the current theme. `families()` hands Rune the theme's font. `apply(cx)` at the top of the view sets the colors, rounding and font for everything under it.
- **`omarchy_rune::ui::*`** are the Omarchy primitives: `Button`, `IconButton`, `Toggle`, `TextField`, `Dropdown`, `Slider`, `Card`, `Pane`, `Heading`, `ListRow`, `Menu`, `Dialog`, `Table`, `Tree` and the rest. Each is a builder ending in `.build(&theme)`, with handlers on the builder (`on_press`, `on_toggle`, `on_change`). Same names, states and metrics as the Qt kit ([`spec/components.md`](../spec/components.md)).
- **rune-kit** components (Select, Popover, Markdown, Diff, ...) are themed through `theme.kit()`. Reach for them only when the kit has no Omarchy primitive for the job.
- **Tokens:** `theme.tokens()` is the full `omarchy_theme::Tokens` (palette, roles, radius, spacing, font, motion), and `omarchy_rune::color()` turns a token color into a Rune `Color`. A color, radius, font, size or duration a token covers is never a literal. [`AGENTS.md`](../AGENTS.md) has the rules that make a view read as Omarchy.

### The theme follows itself

`setup: Some(|s, wake| s.theme.follow(wake))` starts a watcher, and `poll: Some(|s, _, _| s.theme.poll())` swaps in new tokens when it fires. Views that read the theme re-render, with Rune's color springs, on theme switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads (rounding, gaps, animations), language and icon-theme changes. There's no polling, and nothing to cache: read `s.theme` in the view.

### Preview other themes, roundings and icon themes

The same overrides as the Qt kit ([`spec/tokens.md`](../spec/tokens.md)):

| Variable | Effect |
|---|---|
| `OMARCHY_UI_THEME_DIR` | Read `colors.toml` / `shell.toml` from this rendered theme directory; the user's `shell.toml` is ignored and nothing is watched |
| `OMARCHY_UI_ROUNDING` | Hyprland's `decoration:rounding` (`0` sharp, `8` rounded) |
| `OMARCHY_UI_GAPS_OUT` | Hyprland's `general:gaps_out` |
| `OMARCHY_UI_REDUCE_MOTION` | `1` reduced motion, `0` motion on |
| `OMARCHY_UI_FONT_FAMILY` | The font family, instead of fontconfig's `monospace` |
| `OMARCHY_UI_ICON_THEME` | The icon theme, instead of the one the theme names |
| `OMARCHY_UI_ICON_DIRS` | Icon search roots (colon-separated) |

Rendered themes: `fixtures/themes/<name>` in a checkout, or `/usr/share/omarchy-ui/themes/<name>` from the Arch package. Omarchy's source theme directories have no `shell.toml`, so they won't do.

### Render snapshots for review

Every Rune app gets a headless render from Rune's tooling, with no code of its own:

```sh
OMARCHY_UI_THEME_DIR=../omarchy-ui/fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 \
  cargo run --release -- --snap latte-r8.png                # also --size WxH, --scale N, --scene NAME
```

Look at a view in more than one theme and rounding before calling it done.

## Any other Rust framework: `omarchy-theme`

For GPUI, iced, egui, a TUI, or a tool with no UI. Map `Tokens` onto the framework's own theme and follow the watcher; everything else is framework-specific.

```toml
[dependencies]
omarchy-theme = { path = "../omarchy-ui/crates/omarchy-theme" }           # or
omarchy-theme = { git = "<omarchy-ui-repo>", rev = "<commit>" }
```

```rust
// Tokens: everything spec/tokens.md defines, resolved from the live theme.
let tokens = omarchy_theme::load();
println!("{} on {}, radius {}", tokens.palette.foreground, tokens.palette.background, tokens.radius);
let accent = tokens.palette.accent.hex();                                // Rgba, as "#rrggbbaa"

// Watch: the callback runs on a background thread with fresh tokens, after a change settles
// (coalesced over 150 ms; only when something actually changed). Keep the Watch alive.
let _watch = omarchy_theme::watch(tokens.clone(), |fresh| {
    // send `fresh` to your UI thread and re-theme
});

// Icons: the freedesktop lookup Omarchy's app launcher uses, in the theme's icon theme.
let icons = omarchy_theme::icons::IconLookup::current();
let firefox = icons.find("firefox", 96);                                // Option<PathBuf>

// Strings: the kit's own strings (Cancel, Search…) in the display language.
let cancel = omarchy_theme::strings::t(&tokens.language, "common.cancel", "Cancel");
```

The names in that snippet are from the crate's docs (`cargo doc -p omarchy-theme --open` in a checkout). `omarchy_rune`'s `Theme` (`rune/omarchy-rune/src/lib.rs`) is a complete worked example of a binding, and [`AGENTS.md`](../AGENTS.md) lists what a new binding in this repository needs.

### The `omarchy-theme` CLI

The same resolver as a command, for scripts and other languages. The Arch package installs it, and `bin/build` builds it to `target/release/omarchy-theme`.

```sh
omarchy-theme                    # the resolved tokens as canonical JSON
omarchy-theme --watch            # again on every change
omarchy-theme --icon firefox 96  # the file an icon name resolves to (empty if none)
```

## Versioning and pinning

Both crates are `0.1.0` and pre-1.0. There are no release tags yet, and APIs can change between commits.

- Pin a git dependency with `rev = "<commit>"`, and commit your `Cargo.lock`. A path dependency follows whatever is checked out there; pin the checkout (a submodule) instead.
- Updating omarchy-ui can move its Rune rev. Through the re-exports you move with it. If you also depend on `rune` directly, move that `rev` in the same change.
- omarchy-ui builds with the Rust toolchain in its `rust-toolchain.toml`. Use that or newer.

## Where things are documented

- [`spec/components.md`](../spec/components.md) and [`spec/components/`](../spec/components): every component's Rune and Qt names, states and metrics.
- [`spec/tokens.md`](../spec/tokens.md): every token and how it's resolved.
- The Rune gallery (`bin/build rune && target/release/omarchy-rune-gallery`, `--snap out.png --scene <section>`): every primitive live. `rune/gallery/src/sections/*.rs` is example code for each.
- `cargo doc -p omarchy-rune -p omarchy-theme --open` in a checkout.

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| ``expected `rune::...`, found `rune::...` `` or two `rune` versions in `cargo tree` | You depend on `rune` at a different rev. Drop it and use `omarchy_rune::rune`, or match the rev in omarchy-ui's `Cargo.toml`. |
| `type annotations needed` on `setup` / `poll` | Name the state type: `\|s: &mut App, wake\|`. |
| "current package believes it's in a workspace when it's not" | Your crate is inside another workspace's directory. `exclude` it there, or add `[workspace]` to your `Cargo.toml`. |
| The app doesn't follow theme switches | `setup`/`poll` aren't wired, `apply(cx)` isn't called at the top of the view, or `OMARCHY_UI_THEME_DIR` is set (a pinned theme is never watched). |
| Glyphs show as boxes | The kit's glyphs are Nerd Font icons from the theme's `monospace` font (Omarchy's default fonts have them). |
| A long first build | Rune and its GPU stack compile once per target directory. Sharing one `CARGO_TARGET_DIR` between apps on the same Rune rev reuses it; `bin/test-examples` does this for `examples/rune`. |
