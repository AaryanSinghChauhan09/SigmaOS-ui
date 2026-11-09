# Using Omarchy UI from Tauri

> `<omarchy-ui-repo>` below stands for this repository's clone URL. It has no public remote yet, so substitute wherever you cloned it from. Paths such as `examples/...` are relative to a checkout.

A [Tauri v2](https://v2.tauri.app) app gets Omarchy's look and live theme following with one call in Rust and one import in the page. The page side is the web kit, `@omarchy/ui` ([`using-web.md`](using-web.md)): the tokens as `--omarchy-*` custom properties, the restyle for plain HTML controls, and every `<omarchy-*>` element. The Rust side is `tauri-plugin-omarchy` ([`web/tauri/plugin`](../web/tauri/plugin)), which gives each webview the same `omarchy` object the Electron preload gives. On Linux, Tauri renders with WebKitGTK; the kit is checked against Qt in it on every `bin/review`, beside Chromium.

| Piece | Where | What it does |
|---|---|---|
| Rust | `tauri-plugin-omarchy` | `.plugin(tauri_plugin_omarchy::init())`, the one call: the theme before first paint, every change, icons, window backgrounds. |
| Page | `@omarchy/ui` | `import "@omarchy/ui"`: finds the plugin's `omarchy` and applies it. No other code, no `@tauri-apps/api`. |
| Snapshots | the plugin's `snap` feature | `app --snap out.png`, rendered on a private display, never on the desktop. |

## Add the dependencies

```toml
# src-tauri/Cargo.toml
[dependencies]
tauri = { version = "=2.12.5" }
tauri-plugin-omarchy = { path = "../omarchy-ui/web/tauri/plugin" }               # or
tauri-plugin-omarchy = { git = "<omarchy-ui-repo>", rev = "<commit>" }
```

```json
{ "dependencies": { "@omarchy/ui": "file:../omarchy-ui/web" } }
```

The plugin depends on the core (`crates/omarchy-theme`) directly: no `omarchy-theme` CLI to install or spawn. It lives in a Cargo workspace of its own (`web/tauri/Cargo.toml`), which Cargo resolves for a path or git dependency alike. If your crate sits inside another workspace's directory without being a member, give it a `[workspace]` of its own, as [`examples/tauri`](../examples/tauri) does.

There's no Arch package for this route, and none is needed: the plugin is a Cargo path or git dependency compiled into your app, and `@omarchy/ui` is bundled into your frontend.

## The one call, the one import

```rust
// src-tauri/src/main.rs
fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_omarchy::init())
        .run(tauri::generate_context!())
        .expect("tauri");
}
```

```js
// the page's entry (main.js, main.ts), through your bundler
import { omarchy } from "@omarchy/ui";
```

```html
<omarchy-pane bordered>
  <omarchy-heading level="heading" text="Network" subtitle="Wi-Fi and saved networks"></omarchy-heading>
  <omarchy-toggle label="Wi-Fi" description="On" checked></omarchy-toggle>
  <omarchy-button bordered text="Forget network"></omarchy-button>
</omarchy-pane>
```

That is the whole adoption. No capability entry, no command permissions: the plugin has no commands, so nothing in `capabilities/` changes. Everything in [`using-web.md`](using-web.md) about the elements, the restyle, the tokens in your own CSS and the frameworks (React, Vue, Svelte) applies as written.

If your app sets a Content Security Policy, let the page reach the plugin's protocol for icons and the state:

```json
"security": { "csp": "default-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' omarchy: http://omarchy.localhost; connect-src omarchy: http://omarchy.localhost" }
```

[`examples/tauri`](../examples/tauri) is the complete app, the Tauri twin of `examples/web-electron`. It has no Node step: its `build.rs` copies `web/src` next to the page, where a real app's bundler would put `@omarchy/ui`. `cargo run --release` in it opens the window; `web/tauri/test-example` renders it on every test run.

## How the bridge works

- **Before first paint.** The plugin returns an initialization script for every webview it creates, which Tauri runs before the page's own scripts. It installs `window.omarchy`, built by `createOmarchy` from `web/src/api.js` (the same source the Electron preload carries, embedded at compile time), holding the state of the moment: `{ theme, iconTheme, language, tokens, colors, strings }`. It reads that state from the plugin's `omarchy:` protocol, so a reload after a switch starts on the new theme; the state as of the webview's creation is baked in as a fallback. `import "@omarchy/ui"` then applies it at once, so the first frame shows the theme with no fade.
- **Windows** get the theme's background (no white flash before the page paints) and its light or dark mode (`Window::set_theme`, which GTK's own dialogs and decorations follow), and follow both.
- **Changes.** The plugin runs the core's watch (`omarchy_theme::watch`: theme switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads, the language and the icon theme, coalesced as [`spec/following.md`](../spec/following.md) says). Each real change is evaluated into every webview window, where `createOmarchy` applies it and then runs `omarchy.onChange` hooks. So the changed hook fires once per change, after the new tokens are in effect, never on load.
- **Icons.** `omarchy.icon("firefox", 48)` asks the plugin's protocol, which looks the name up in the active icon theme by the core's lookup and answers `omarchy://localhost/icon/48/firefox`, a URL `<img>` loads from the same protocol. Only icon names resolve: a path from the page (`/etc/passwd`, `../x`) is never read.

## The changed hook

In the page, as in Electron ([`using-web.md`](using-web.md#the-changed-hook)):

```js
omarchy.tokens.palette.background;   // "#1a1b26ff"
omarchy.revision;                    // 0 at load, plus one per change
omarchy.onChange((tokens, state) => redraw(tokens.palette.accent));
```

Colors fade over `motion.theme` (280 ms) on `motion.ease` through `@property` transitions, which WebKitGTK runs as Chromium does; no fade under reduced motion or on first paint.

In Rust, for anything outside the webviews (a tray icon, a native menu):

```rust
use tauri_plugin_omarchy::OmarchyExt;

let o = app.omarchy();               // on an AppHandle, Window or Webview
o.tokens().palette.accent;           // omarchy_theme::Tokens
o.revision();
o.on_change(|tokens| { /* once per change, on the main thread */ });
o.icon_path("firefox", 48);
```

`on_change` runs on the main thread, after every webview was sent the new state, so a handler can touch windows.

## Previews and snapshots

`OMARCHY_UI_*` overrides in the app's environment reach the plugin, so `OMARCHY_UI_THEME_DIR=…/fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 cargo run` previews another theme and rounding.

The plugin's `snap` feature gives an app `--snap out.png [--size WxH] [--scale N]`:

```rust
fn main() {
    let snap = tauri_plugin_omarchy::snap::from_args();   // first: it starts a private display
    let mut app = tauri::Builder::default().plugin(tauri_plugin_omarchy::init());
    if let Some(snap) = snap {
        app = app.plugin(snap.plugin());
    }
    app.run(tauri::generate_context!()).expect("tauri");
}
```

It renders the first window at its configured size (or `--size` points) once the page has loaded and its fonts are in (and `window.omarchySnapReady` has settled, when the page defines it), at device scale 2, then exits. No window reaches the desktop: the app runs on a GTK broadway display of its own (`broadwayd`, from GTK 3), WebKit renders in software, and the PNG comes from `webkit_web_view_get_snapshot`. Broadway is always 1×; for 2×, export the GTK function the snap module answers for WebKit's views by adding one line to `build.rs`:

```rust
println!("cargo:rustc-link-arg-bins=-Wl,--export-dynamic-symbol=gtk_widget_get_scale_factor");
```

Without it the snap is 1×.

## The gallery and the tests

```sh
web/tauri/build                                                        # bin/build tauri
web/tauri/target/release/omarchy-tauri-gallery                         # a window following the desktop
web/tauri/target/release/omarchy-tauri-gallery --list-sections
web/tauri/target/release/omarchy-tauri-gallery --snap out.png --section buttons --size 760x400 --scale 2
bin/review --bindings tauri,web                                        # WebKitGTK and Chromium against Qt
```

The gallery is `web/gallery` in a real Tauri app with the plugin, served from `web/` at run time (an edit to the kit shows up without a rebuild). `tools/bindings.py test tauri` runs:

| Test | What it covers |
|---|---|
| `plugin` | The plugin's unit tests: icon names never paths, the initialization script. |
| `node web/test/tauri.mjs` | The bridge on WebKitGTK: `omarchy` before the page's first script, the core's tokens, strings and colors, the first frame on the theme with no fade, revision 0 and no hook on load, icons by name through the protocol (and a path refused). |
| `tools/live/run node web/test/tauri-live.mjs` | The live switch: one hook with the new tokens, revision 1, the variable changed, a fade (none with `OMARCHY_UI_REDUCE_MOTION=1`), the Rust `on_change` once, and a reload starting on the new theme. |
| `web/tauri/test-example` | Builds `examples/tauri` and renders it. |

The Node tests drive the gallery app's `--drive PAGE` mode through `web/lib/tauri.js`, headless.

## Pins

`tauri =2.12.5` and `tauri-build =2.7.4` (`web/tauri/Cargo.toml`, `examples/tauri/Cargo.toml`); the `snap` feature pins the WebKitGTK and GTK 3 bindings Tauri's `wry` uses (`webkit2gtk =2.0.2`, `gtk =0.18.2`). An app on another Tauri 2.x builds against them as long as Cargo can unify the versions; move them together. The system needs `webkit2gtk-4.1` (2.54 tested) and, for snapshots, `broadwayd` (`gtk3`).

## Limits

- **Linux only.** The plugin compiles elsewhere (the protocol's URL follows the platform), but Omarchy is Linux and only WebKitGTK is checked.
- **Text rasterizes differently.** WebKitGTK rounds a font's ascent and descent differently from Chromium and Qt, so baselines sit up to a pixel lower at some sizes; glyph antialiasing differs. Layout, metrics and colors match (`bin/review --bindings tauri`).
- **Webview windows only.** Changes reach every `WebviewWindow`; a second webview inside one window (Tauri's `unstable` multiwebview) gets its state on load but not the changes.
- **Snapshots need the export line for 2×** (above), and `broadwayd` on `PATH`.
