# Following the Omarchy theme

Every binding follows the desktop live: a theme switch, `omarchy font set`, `omarchy display text size`, a Hyprland reload or `omarchy language` reaches a running app without a restart. This is the contract each binding implements, so an app behaves the same in Qt, Rune, GTK, Qt Widgets, GPUI, Electron or Tauri. [`tokens.md`](tokens.md) says what the tokens are; this file says how they change while an app runs.

## What is watched

| Source | Changed by | Watched as |
|---|---|---|
| `~/.local/state/omarchy/current/` | `omarchy theme set`: the whole `current/theme` directory is replaced (`rm -rf theme; mv next-theme theme`), then `current/theme.name` is rewritten | the `current/` directory |
| `current/theme/` (`colors.toml`, `shell.toml`, `icons.theme`) | edits in place | the theme directory itself, re-armed after every change (a swap leaves the watch on the old directory) |
| `~/.config/omarchy/shell.toml` | `omarchy display text size`, hand edits | its directory (editors and the CLI replace it by rename) |
| `~/.config/omarchy/language` | `omarchy language` | the same directory |
| `~/.config/fontconfig/` | `omarchy font set` | the directory; `monospace` is resolved again with `fc-match` |
| Hyprland's `.socket2.sock` | `configreloaded>>` once new rounding, gaps, border size or the animations toggle are in effect | a line reader, reconnecting every 2 s when Hyprland isn't up |

`HYPRLAND_INSTANCE_SIGNATURE` names the instance; without it, the most recently started instance under `$XDG_RUNTIME_DIR/hypr/` is used. With `OMARCHY_UI_THEME_DIR` set, only that directory is watched (an edit there is followed; the desktop is not).

Nothing is polled. The Rust resolver (`crates/omarchy-theme`, `watch.rs`) is the reference implementation; the Qt binding has its own (`qt/src/themesource.cpp`). Any other toolkit uses the Rust one through the C library or the CLI ([`docs/using-c.md`](../docs/using-c.md)) rather than writing a third.

## Coalescing

A theme switch is a burst: the directory swap, `theme.name`, `theme.generation`, the background link, then Hyprland's reload. Events are coalesced until **150 ms** pass with no new one, then everything is resolved again, once, and compared with what the app has. Only a difference is delivered. So:

- one `omarchy theme set` is one change, not three;
- an event that alters nothing (the background link, a `shell.toml` saved unchanged, a Hyprland reload with the same rounding) is no change at all;
- a change that lands later than 150 ms after the rest (Hyprland applying a new rounding a moment after the colors) can be a second change. That is correct: the tokens did change twice.

## Adoption

Following takes no app code, or one call at startup. Never a loop, a timer or a file watch in the app.

| Binding | What the app does |
|---|---|
| Qt Quick | Nothing. Bind to `Theme.Color.*` / `Theme.Style.*`; the singletons follow by themselves. |
| Rune | `omarchy_rune::run(spec)` instead of `rune::run(spec)`, with `impl Themed for App` (two accessor methods). |
| C (any main loop) | `omarchy_theme_watch_new()`, then add `omarchy_theme_watch_fd()` to the loop (`g_unix_fd_add`, `QSocketNotifier`, `poll`) and call `omarchy_theme_watch_take()` when it's readable. Or `omarchy_theme_watch_callback()`. |
| Rust, other UI | `omarchy_theme::watch(initial, callback)`; marshal to the UI thread. |
| Tauri | `.plugin(tauri_plugin_omarchy::init())`; the page imports `@omarchy/ui`. |
| Process-spawning (Electron, scripts) | Spawn `omarchy-theme --watch` (or `--watch --state`) and read one JSON line per change. |

A binding that wraps a toolkit (GTK, Qt Widgets, GPUI, Electron) makes the toolkit side zero-code or one-call too: re-theme the toolkit's own palette or CSS from each change, so the app never sees the mechanism.

## The changed hook

Every binding exposes a public hook an app can use for what bindings can't reach: a custom renderer, a cache of measured text, a terminal it themes itself.

1. **Once per real change.** It fires exactly once for each delivered change (after coalescing, and only when the resolved tokens differ), never twice for one switch.
2. **Never for the initial load.** The tokens an app starts with are not a change. An app that needs them reads them directly (`Tokens.canonical()`, `theme.tokens()`, `omarchy_theme_tokens_json()`).
3. **After the new tokens are in effect.** When it runs, everything the binding exposes already reads the new values: a handler that reads `Theme.Color.background` or `theme.tokens()` gets the new one, not the old.
4. **On the UI thread.** The thread the toolkit's widgets live on, so a handler can touch them. The Rust core's `watch` and the C library's callback variant are below the UI layer and run on a background thread; that's why bindings wrap them. The C library's fd + take is called from the main loop and so runs wherever the loop runs.
5. **With the new tokens.** The handler receives the tokens (an object in canonical-JSON shape, or the binding's typed equivalent): the target values, never a frame of the fade below.
6. **A revision.** Each binding keeps an integer revision: 0 for the initial load, plus one per delivered change. It's a cheap cache key (`if (rev != cachedRev) rebuild()`).

What counts as "the tokens" for rules 1 and 2: the canonical JSON ([`tokens.md`](tokens.md)), plus the icon theme, the display language and the theme name, which bindings expose beside it. A change to only the icon theme or the language is a change; the canonical JSON in it can be identical to the last one, so consumers re-read the icon theme and strings on every change.

## The theme-switch transition

Colors ease from the old theme to the new; nothing else moves.

- **Colors fade over `motion.theme` (280 ms) on `motion.ease`, `cubic-bezier(0.25, 0.1, 0.25, 1)`** (CSS's `ease`). Every color: palette, roles, state fills and borders, surfaces. Interpolate each channel, alpha included, in sRGB (what Qt's `ColorAnimation`, CSS transitions and Rune do).
- **Rounding, gaps, spacing, sizes, the font and its sizes, the icon theme and strings switch instantly**, at the start of the fade. A moving layout during a recolor reads as a glitch, not a transition.
- **No transition when motion is reduced**: `reduceMotion` is true (Hyprland's `animations:enabled` off, `omarchy toggle animations`) or `OMARCHY_UI_REDUCE_MOTION=1`. The new colors land at once.
- **No transition on first paint.** The first frame shows the tokens exactly, so stills (`--snap`, `bin/review`) and token dumps never catch a fade.
- **Interrupted**: a second change during a fade starts a new fade from the colors on screen, not from the old theme.
- **Exact at the end**: when the fade lands, every color equals its token exactly (no rounding drift from the interpolation).

### Why 280 ms on `ease`

Rune animates every color change, a theme switch included, on its element springs: by default `tok::motion::SETTLE`, `Spring::new(0.3, 0.0)`, a critically damped spring (ω = 2π / 0.3 s), with text color leading on a spring half as long (0.15 s). A critically damped spring from rest starts slowly, peaks in speed at 1/ω ≈ 48 ms and is 99% there by 300 ms: an S-curve, front-loaded. Fitted against curves the token scale and CSS name, the closest fixed-duration curve is `ease` at 280 ms, within 3.5% of the spring at every instant (the kit's `easeOut`, the press curve, is a poor fit at any duration: 8% off at best, because it starts at full speed). So a Qt or CSS fade at 280 ms `ease` reads the same as Rune's spring beside it. Rune keeps its spring (the kit sets it explicitly for a theme change, `Tx::color_motion`); every other binding uses the curve.

Bindings may lead text by half the duration, as Rune does, so text never dips through low contrast mid-fade; it isn't required.

## The live-switch test

Every binding has an automated test that switches the theme the way Omarchy does and checks the result, run by `bin/test`. The shared steps:

1. **A home of its own.** `HOME` is a fresh temp directory, `OMARCHY_UI_THEME_DIR` is unset, `HYPRLAND_INSTANCE_SIGNATURE` is unset and `XDG_RUNTIME_DIR` points at an empty temp directory, so no running Hyprland is found (the fallback would otherwise pick the desktop's). `OMARCHY_UI_ROUNDING=0` pins the rounding. `tools/live/run CMD` sets all of this up (and steps 2's starting theme), runs `CMD` with `OMARCHY_UI_LIVE_HOME` and `OMARCHY_UI_LIVE_SWITCH` naming the HOME and the switch script, and cleans up; a test that can't take a wrapper (a Rust integration test) does the same in-process, as `crates/omarchy-theme/tests/live_switch.rs` does.
2. **A starting theme.** `tools/live/theme-set "$HOME" tokyo-night` copies the fixture into `~/.local/state/omarchy/current/theme` and writes `current/theme.name`.
3. **The app starts** and follows (its one call, or none).
4. **The switch.** `tools/live/theme-set "$HOME" catppuccin-latte` does what `omarchy-theme-set` does: fills `current/next-theme`, `rm -rf current/theme`, `mv current/next-theme current/theme`, writes `theme.generation`, then rewrites `theme.name`.
5. **Checks**, within 5 s:
   - the binding's background reads `#eff1f5` (catppuccin-latte's) through its public API;
   - the changed hook fired **once**, with the new tokens (`palette.background` is `#eff1f5ff`), and still once after another 600 ms;
   - the revision is 1.
6. **Where it fades**, also: a fade ran (a color between the two was on screen, or the binding says it is fading) and landed exactly; under `OMARCHY_UI_REDUCE_MOTION=1` no fade ran.

The tests today: `crates/omarchy-theme/tests/live_switch.rs` (the Rust watch), `crates/omarchy-theme-ffi/tests/live.c` (the C library, both watch forms), `rune/omarchy-rune/tests/live_switch.rs` (Rune, with the transition and reduced motion), `qt/tests-live/tst_follow.qml` (Qt, run with and without `OMARCHY_UI_REDUCE_MOTION=1`), and each newer binding's `live switch` entries in `tools/bindings.d/` (GTK `build/gtk/omarchy-gtk-live-test`, KDE `build/kde/omarchy-widgets-live-test` and its Kirigami twin, GPUI `omarchy-gpui-live-test`, web `web/test/live.mjs` in Chromium and `web/test/electron-live.mjs` in Electron, Tauri `web/test/tauri-live.mjs` in WebKitGTK).

`tools/live/theme-set` is the one simulation every test uses, so they all exercise the same sequence. A binding's registry entry (`tools/bindings.d/<name>.json`) lists its live test with its other tests.

## In each binding

| Binding | Watch | Changed hook | Revision | Fade |
|---|---|---|---|---|
| Rust core (`omarchy-theme`) | `watch(initial, f)` | `f(tokens)` on a background thread | the app counts | none (no UI) |
| C library (`libomarchy-theme`) | `omarchy_theme_watch_new` + `_fd` + `_take`, or `omarchy_theme_watch_callback` | each `take` that returns JSON / each callback | `omarchy_theme_watch_revision` | none (no UI) |
| CLI | `omarchy-theme --watch [--state]` | each line after the first | the line count | none |
| Rune (`omarchy-rune`) | `omarchy_rune::run` / `app`, or `Theme::follow` + `Theme::poll_tx` | `Themed::theme_changed(&mut self, &Tokens)` | `Theme::revision()` | Rune's color spring, layout instant |
| Qt Quick (`Omarchy.Theme`) | built in (`ThemeSource`) | `Tokens.changed(tokens)` in QML, `OmarchyUi::Theme::changed(QJsonObject)` in C++ | `Tokens.revision`, `OmarchyUi::Theme::revision()` | `Color.qml`, 280 ms `ease` |
| GTK 4 (`libomarchy-gtk-1`) | `omarchy_gtk_init()` (the C library's fd on `g_unix_fd_add`) | `OmarchyTheme::changed (GVariant *tokens)` | the `revision` property | the generated CSS and the kit widgets, 280 ms `ease` |
| KDE / Qt Widgets (`OmarchyWidgets`) | `Omarchy::Widgets::apply(app)` (the fd on a `QSocketNotifier`) | `Omarchy::Theme::changed(QJsonObject)` | `Omarchy::Theme::revision()` | palette, style and KDE color scheme, 280 ms `ease` |
| GPUI (`omarchy-gpui`) | `omarchy_gpui::init(cx)` (`watch` onto the app thread) | `omarchy_gpui::on_change(cx, f)`, or `cx.observe_global::<Theme>()` | `Theme::get(cx).revision()` | the `Paint` global, 280 ms `ease`, layout instant |
| Electron / web (`@omarchy/ui`) | `followOmarchy()` in main (the CLI stream), applied on import in the renderer | `omarchy.onChange(cb)` | `omarchy.revision` | `@property` transitions on the `--omarchy-*` colors, 280 ms `ease` |
| Tauri (`tauri-plugin-omarchy` + `@omarchy/ui`) | `.plugin(tauri_plugin_omarchy::init())` (the core's `watch`, evaluated into each webview), applied on import in the page | `omarchy.onChange(cb)` in the page, `app.omarchy().on_change(f)` in Rust | `omarchy.revision`, `app.omarchy().revision()` | the web kit's `@property` transitions, in WebKitGTK |
