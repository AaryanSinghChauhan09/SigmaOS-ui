# Omarchy UI

A cross-framework kit for apps that look and feel like Omarchy. One token contract (`spec/tokens.md`), resolved from the live Omarchy theme, and the same primitives (`spec/components.md`) in every framework. Qt and Rune today.

## Building an app with it

Use the kit's components and tokens. Never hard-code a color, radius, font, size or duration that a token covers.

- **Qt Quick** ([`docs/using-qt.md`](docs/using-qt.md)): with the modules installed (`packaging/arch/omarchy-ui`, or `qt/module`), any QML file can `import Omarchy.Theme as Theme` and `import Omarchy.Ui as O`. A vendoring app instead does `include(path/to/omarchy-ui/qt/omarchy-ui.pri)` and calls `OmarchyUi::registerTypes()` before loading QML. Colors are `Theme.Color.*`, structure is `Theme.Style.*`, borders are `Theme.Border.*` specs drawn with `O.BorderSurface`. Qualify the imports (`O.Button`): the kit has `Button`, `TextField` and `Color` like Qt Quick does. Run with `QT_QUICK_CONTROLS_STYLE=Basic`.
- **Rune** ([`docs/using-rust.md`](docs/using-rust.md)): depend on `omarchy-rune` only and use its `omarchy_rune::rune` / `rune_kit` re-exports. Keep an `omarchy_rune::Theme` in the app state (`Theme::load()`), `impl omarchy_rune::Themed for App` (`theme()`, `theme_mut()`), start with `omarchy_rune::run(AppSpec::new(...))` instead of `rune::run` (it follows the theme, sets the font, applies the theme every frame and keeps your own `setup`/`poll`), and build with `omarchy_rune::ui::*`. `Themed::theme_changed` is the changed hook; `theme.revision()` a cache key. Use rune-kit through `theme.kit()` only when the kit has no Omarchy primitive for the job.
- **Any other Rust framework (GPUI, iced, egui)**: depend on `omarchy-theme`, map `Tokens` onto the framework's theme, and follow `omarchy_theme::watch`. Then add a binding here (below).

Look at what you built before calling it done, in more than one theme and rounding:

- Qt: `build/qt/omarchy-qt-gallery --qml view.qml --snap out.png` renders any QML file (an Item or a Window) with the kit, offscreen. Against the installed modules, `QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 qml6 examples/qt-installed/snap.qml -- /abs/view.qml /abs/out.png` does the same for a Window.
- Rune: every app gets `--snap out.png [--scene name] [--size WxH]` from Rune's own tooling.
- Either: prefix with `OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8` to see a light theme with rounded corners, or any fixture under `fixtures/themes/`.

What makes it read as Omarchy, in order of how often agents get it wrong:

1. **Corners come from Hyprland** (`radius`, usually 0: sharp). Don't round things because they look nicer rounded. Small parts cap at `min(4, radius)`.
2. **Controls are foreground glass**, not accent-filled: a faint foreground fill and border, stronger under the cursor. Accent only marks what is chosen. Urgent only marks destructive actions.
3. **One font**: fontconfig's `monospace`, at the token sizes. Bold only for selection and titles.
4. **Flat**: no drop shadows, no gradients except a theme's border gradient.
5. **Keyboard-first**: every control is focusable, and focus looks like hover.
6. **Follow the theme live**: never cache colors at startup; bindings re-resolve on theme switch, `omarchy font set`, `omarchy display text size` and Hyprland reloads. Something the kit can't reach (a custom renderer, a cache) listens to the binding's changed hook ([`spec/following.md`](spec/following.md)).

## Working on the kit

```sh
bin/build                  # both galleries + the token CLI
bin/test                   # Rust tests, Qt Quick tests (qt/tests), Qt/Rune token and icon parity, examples/
bin/test-examples          # build each example app (examples/) and render it to build/examples/
bin/review --open          # render every section in both, compare in the browser
bin/review --live          # both galleries as windows, following the desktop
bin/harvest-qt --check     # what changed in the shell since the last harvest
bin/fixtures               # regenerate fixtures/themes from ~/omarchy
```

The Omarchy checkout is `~/omarchy` on `quattro` (`$OMARCHY_SRC` to override). Pull it before harvesting or regenerating fixtures.

- **Following**: `spec/following.md` is the contract for live changes: what is watched, 150 ms coalescing, the changed hook (once per real change, after the new tokens are in effect, on the UI thread, never for the initial load), the 280 ms `ease` color fade, and the live-switch test every binding has.
- **Tokens**: `spec/tokens.md` is the contract. Change it first, then `crates/omarchy-theme` (Rust resolver) and the Qt side (`qt/qml/Omarchy/Theme`), and keep `bin/test` green: the two must dump byte-identical JSON for every fixture.
- **Qt components** are harvested from the shell (`qt/HARVEST.md`). Don't edit harvested files: fix them in the shell, or add a patch under `qt/patches/` when the fix only matters outside the shell.
- **A new component** goes in `spec/components.md` with its metrics, then in every binding under the same name, then into every gallery as `spec/gallery.md` lists it. Check it with `bin/review`. Behavior gets a test in both: a `qt/tests/tst_*.qml` case and a Rune gallery test (`rune/gallery/src/main.rs`) driving the same keys and clicks.
- **A new framework** gets `<framework>/` with a binding that resolves tokens (reuse `omarchy-theme` from Rust), a gallery with the `spec/gallery.md` sections, `--dump-tokens`, a still-render mode at 2×, and the `OMARCHY_UI_*` overrides; then a column in `tools/review.py`.
- **How apps consume the kit** (installed QML modules from `qt/module`, the vendored `qt/omarchy-ui.pri`, Cargo path/git dependencies) is documented in `docs/` and proven by `examples/`. Keep both building: `qt/src/native.pri` is the C++ both Qt routes compile, and `OmarchyUi::registerTypes()` is the one place native types are registered.
- **Commits**: short, imperative, one change each.
