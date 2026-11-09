# Omarchy UI

A cross-framework kit for apps that look and feel like Omarchy. One token contract (`spec/tokens.md`), resolved from the live Omarchy theme, and the same primitives (`spec/components.md`) in every framework: Qt Quick, Rune, GTK 4, KDE / Qt Widgets, GPUI and Electron / web today, and Ratatui in the terminal ([`spec/terminal.md`](spec/terminal.md)), each registered in `tools/bindings.d/`.

## Building an app with it

Use the kit's components and tokens. Never hard-code a color, radius, font, size or duration that a token covers.

- **Qt Quick** ([`docs/using-qt.md`](docs/using-qt.md)): with the modules installed (`packaging/arch/omarchy-ui`, or `qt/module`), any QML file can `import Omarchy.Theme as Theme` and `import Omarchy.Ui as O`. A vendoring app instead does `include(path/to/omarchy-ui/qt/omarchy-ui.pri)` and calls `OmarchyUi::registerTypes()` before loading QML. Colors are `Theme.Color.*`, structure is `Theme.Style.*`, borders are `Theme.Border.*` specs drawn with `O.BorderSurface`. Qualify the imports (`O.Button`): the kit has `Button`, `TextField` and `Color` like Qt Quick does. Run with `QT_QUICK_CONTROLS_STYLE=Basic`.
- **Rune** ([`docs/using-rust.md`](docs/using-rust.md)): depend on `omarchy-rune` only and use its `omarchy_rune::rune` / `rune_kit` re-exports. Keep an `omarchy_rune::Theme` in the app state (`Theme::load()`), `impl omarchy_rune::Themed for App` (`theme()`, `theme_mut()`), start with `omarchy_rune::run(AppSpec::new(...))` instead of `rune::run` (it follows the theme, sets the font, applies the theme every frame and keeps your own `setup`/`poll`), and build with `omarchy_rune::ui::*`. `Themed::theme_changed` is the changed hook; `theme.revision()` a cache key. Use rune-kit through `theme.kit()` only when the kit has no Omarchy primitive for the job.
- **GTK 4 / libadwaita** ([`docs/using-gtk.md`](docs/using-gtk.md)): call `omarchy_gtk_init()` (`Omarchy.init()` through GI) once after GTK starts; it restyles stock widgets and follows the theme. Use the kit's `Omarchy*` widgets for anything in `spec/components.md`. Connect `OmarchyTheme::changed` for anything drawn by hand.
- **KDE / Qt Widgets / Kirigami** ([`docs/using-kde.md`](docs/using-kde.md)): call `Omarchy::Widgets::apply(app)` once on the `QApplication`; use `Omarchy::*` widgets; `Omarchy::Theme::instance()->changed` for custom painting.
- **GPUI** ([`docs/using-gpui.md`](docs/using-gpui.md)): depend on `omarchy-gpui` only (it re-exports `gpui`), call `omarchy_gpui::init(cx)` once, build with `omarchy_gpui::ui::*`, and read colors through the `Paint` global so they fade.
- **Electron / web** ([`docs/using-web.md`](docs/using-web.md)): `followOmarchy()` in main and `omarchyPreload` on every window; `import "@omarchy/ui"` in the renderer; `<omarchy-*>` elements and `var(--omarchy-*)`, never literal colors.
- **Ratatui / terminal** ([`docs/using-ratatui.md`](docs/using-ratatui.md)): depend on `omarchy-ratatui` only (it re-exports `ratatui` and `crossterm`), keep an `omarchy_ratatui::Theme` in the app (`Theme::load()`), `impl Themed` (`theme()`, `theme_mut()`, and `theme_changed` as the changed hook) and `impl App` (`draw`, `event`), and start with `omarchy_ratatui::run(app)`. Build with `omarchy_ratatui::ui::*` (`.with(&theme)` makes any of them a widget); never paint the theme background. Any other terminal toolkit maps the kit per [`spec/terminal.md`](spec/terminal.md).
- **Any other Rust framework (iced, egui)**: depend on `omarchy-theme`, map `Tokens` onto the framework's theme, and follow `omarchy_theme::watch`. Then add a binding here (below).
- **Any other toolkit or language** ([`docs/using-c.md`](docs/using-c.md)): link `libomarchy-theme` (`pkg-config omarchy-theme`: tokens JSON, a main-loop watch fd, icons, strings), or spawn `omarchy-theme --watch --state` and read a JSON line per change (Node, Electron). Fade colors on a change as [`spec/following.md`](spec/following.md) says.

Look at what you built before calling it done, in more than one theme and rounding:

- Qt: `build/qt/omarchy-qt-gallery --qml view.qml --snap out.png` renders any QML file (an Item or a Window) with the kit, offscreen. Against the installed modules, `QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 qml6 examples/qt-installed/snap.qml -- /abs/view.qml /abs/out.png` does the same for a Window.
- Rune: every app gets `--snap out.png [--scene name] [--size WxH]` from Rune's own tooling.
- GTK: `OMARCHY_GTK_SNAP=out.png OMARCHY_GTK_SNAP_SCALE=2 omarchy-gtk-headless ./app` renders any app that made the one call (a private broadway display, no window shown).
- KDE / Qt Widgets: `QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2` and `QWidget::grab()`, as `kde/` does; the gallery has `--snap`.
- Ratatui: `--snap out.ans [--size COLSxROWS]` (`omarchy_ratatui::snap::from_args`) writes an ANSI snapshot; `target/release/omarchy-ansi png out.ans out.png` renders it.
- GPUI: the `snap` feature's `omarchy_gpui::snap::from_args` gives an app `--snap out.png [--size WxH]` (headless wgpu, no window).
- Electron / web: `web/bin/omarchy-web-gallery --snap`, or `npm start -- --snap out.png` in `examples/web-electron` (headless Chromium).
- Any of them: prefix with `OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8` to see a light theme with rounded corners, or any fixture under `fixtures/themes/`.

What makes it read as Omarchy, in order of how often agents get it wrong:

1. **Corners come from Hyprland** (`radius`, usually 0: sharp). Don't round things because they look nicer rounded. Small parts cap at `min(4, radius)`.
2. **Controls are foreground glass**, not accent-filled: a faint foreground fill and border, stronger under the cursor. Accent only marks what is chosen. Urgent only marks destructive actions.
3. **One font**: fontconfig's `monospace`, at the token sizes. Bold only for selection and titles.
4. **Flat**: no drop shadows, no gradients except a theme's border gradient.
5. **Keyboard-first**: every control is focusable, and focus looks like hover.
6. **Follow the theme live**: never cache colors at startup; bindings re-resolve on theme switch, `omarchy font set`, `omarchy display text size` and Hyprland reloads. Something the kit can't reach (a custom renderer, a cache) listens to the binding's changed hook ([`spec/following.md`](spec/following.md)).

## Working on the kit

```sh
bin/build                  # the core (omarchy-theme CLI, C library in build/ffi) + every binding in tools/bindings.d
bin/test                   # core tests and live switches, each binding's tests and live switch, parity with the core, examples/
bin/test-examples          # build each example app (examples/) and render it to build/examples/
bin/review --open          # render every section in every binding, diff each against Qt, compare in the browser
bin/review --live          # every gallery as a window, following the desktop
bin/harvest-qt --check     # what changed in the shell since the last harvest
bin/fixtures               # regenerate fixtures/themes from ~/omarchy
```

The Omarchy checkout is `~/omarchy` on `quattro` (`$OMARCHY_SRC` to override). Pull it before harvesting or regenerating fixtures.

- **Following**: `spec/following.md` is the contract for live changes: what is watched, 150 ms coalescing, the changed hook (once per real change, after the new tokens are in effect, on the UI thread, never for the initial load), the 280 ms `ease` color fade, and the live-switch test every binding has.
- **Tokens**: `spec/tokens.md` is the contract. Change it first, then `crates/omarchy-theme` (Rust resolver) and the Qt side (`qt/qml/Omarchy/Theme`), and keep `bin/test` green: the two must dump byte-identical JSON for every fixture. Everything else reads the Rust resolver: Rust crates directly, other languages through `libomarchy-theme` (`crates/omarchy-theme-ffi`, `omarchy-theme.h`) or the `omarchy-theme` CLI ([`docs/using-c.md`](docs/using-c.md)). Never write a third resolver.
- **Qt components** are harvested from the shell (`qt/HARVEST.md`). Don't edit harvested files: fix them in the shell, or add a patch under `qt/patches/` when the fix only matters outside the shell.
- **A new component** goes in `spec/components.md` with its metrics, then in every binding under the same name, then into every gallery as `spec/gallery.md` lists it. Check it with `bin/review`. Behavior gets a test in both: a `qt/tests/tst_*.qml` case and a Rune gallery test (`rune/gallery/src/main.rs`) driving the same keys and clicks.
- **How apps consume the kit** (installed QML modules from `qt/module`, the vendored `qt/omarchy-ui.pri`, Cargo path/git dependencies) is documented in `docs/` and proven by `examples/`. Keep both building: `qt/src/native.pri` is the C++ both Qt routes compile, and `OmarchyUi::registerTypes()` is the one place native types are registered.
- **Commits**: short, imperative, one change each.

## A new framework

A binding (GTK, Qt Widgets, GPUI, Electron, ...) is a directory of its own plus one registry file. Nothing shared needs editing: `bin/build`, `bin/test`, `bin/review` and `tools/review.py` read `tools/bindings.d/*.json` (through `tools/bindings.py`).

1. **Tokens from the core, never a new resolver.** Rust: depend on `omarchy-theme`. C, GObject (and through GI: Python, GJS, Vala), C++: link `libomarchy-theme` (`pkg-config omarchy-theme`, [`docs/using-c.md`](docs/using-c.md)). Node/Electron: spawn `omarchy-theme --watch --state` and read lines. Map the canonical JSON onto the toolkit's own theme (GTK CSS, a `QPalette`, GPUI's theme, CSS custom properties).
2. **Follow live, per [`spec/following.md`](spec/following.md)**: zero app code or one call; a public changed hook (once per real change, after the new tokens are in effect, on the UI thread, with the new tokens, never for the initial load) and a revision; colors fade over `motion.theme` on `motion.ease`, everything else switches at once, no fade under reduced motion or on first paint.
3. **The components** of [`spec/components.md`](spec/components.md), under the same names, and a **gallery** with the [`spec/gallery.md`](spec/gallery.md) sections that can: list its sections; render one section to a PNG at 2× and a given size in points, offscreen; honor the `OMARCHY_UI_*` overrides; and (unless it reads the core's tokens directly) print its tokens as canonical JSON.
4. **Tests**, run by `bin/test`: its behavior tests, and a **live-switch test** (`tools/live/run` + `tools/live/theme-set`, spec/following.md) that checks the new background, a changed hook that fires once, and the fade.
5. **Docs**: `docs/using-<framework>.md`, linked from `docs/README.md`.
6. **The registry file**, `tools/bindings.d/<name>.json` (the file name is the `name`):

```json
{
  "name": "gtk",
  "title": "GTK 4",
  "order": 30,
  "docs": "docs/using-gtk.md",
  "available": { "commands": ["meson", "ninja"], "check": "pkg-config --exists gtk4 libadwaita-1", "reason": "no gtk4/libadwaita-1 development files" },
  "build": "gtk/build",
  "gallery": {
    "sections": "build/gtk/omarchy-gtk-gallery --list-sections",
    "snap": "build/gtk/omarchy-gtk-gallery --snap {out} --section {section} --size {width}x{height} --scale {scale}",
    "window": "build/gtk/omarchy-gtk-gallery"
  },
  "tokens": "core",
  "icons": "core",
  "tests": [
    { "name": "behavior", "run": "build/gtk/omarchy-gtk-tests" },
    { "name": "live switch", "run": "tools/live/run build/gtk/omarchy-gtk-live-test" },
    { "name": "live switch, reduced motion", "run": "tools/live/run build/gtk/omarchy-gtk-live-test", "env": { "OMARCHY_UI_REDUCE_MOTION": "1" } }
  ]
}
```

| Key | Meaning |
|---|---|
| `name`, `title` | Registry name (= the file name; `bin/build gtk`) and the column title on the review page |
| `order` | Column order after the reference (Qt is 10, Rune 20) |
| `family` | `"gui"` (the default) or `"terminal"` ([`spec/terminal.md`](spec/terminal.md)). Bindings are only diffed within their family |
| `reference` | `true` for the reference of its family: Qt (gui), Ratatui (terminal). Every other binding of the family is diffed against it and renders at its size |
| `available` | `commands` that must be on `PATH`, and an optional shell `check` (exit 0 = available) with a `reason`. When it fails the binding is skipped with a message, never failed. |
| `build` | Shell command from the repository root that builds everything below |
| `gallery.sections` | Prints the gallery's sections, one per line |
| `gallery.snap` | Renders `{section}` to the PNG `{out}` at `{width}`×`{height}` points and device scale `{scale}` (2), offscreen, under whatever `OMARCHY_UI_*` the caller set. The reference gets `{width}` 760 and sizes itself; the rest get the reference's size. A terminal binding writes an ANSI snapshot instead (below). |
| `gallery.window` | The gallery as a window following the desktop (`bin/review --live`) |
| `tokens` | `"core"` when the binding reads the core's tokens directly (Rust crate, C library, CLI), or `{ "dump": "<command printing canonical JSON>" }` for a resolver of its own; a dump is compared with the core for every fixture at radius 0 and 8 |
| `icons` | `"core"`, or `{ "lookup": "<command printing the path for {name} at {size}>" }`, compared with the core on `fixtures/icons`, or `"none"` for a binding that draws no icons by name (a terminal) |
| `tests` | Each `{ name, run, env? }` runs from the repository root; exit 0 passes and its last output line is shown |

**A terminal binding** (`"family": "terminal"`) follows [`spec/terminal.md`](spec/terminal.md). Its `gallery.snap` writes `{section}` as an ANSI snapshot (`.ans`: UTF-8, truecolor SGR, exactly `{width}` columns × `{height}` rows; `--size COLSxROWS`, rows 0 = the section's natural height). The reference (Ratatui) gets 96 columns and 0 rows; the rest get its size. `bin/review` renders every terminal snapshot through one rasterizer (`target/release/omarchy-ansi png`, the core's font and colors), diffs each against Ratatui by cells (`omarchy-ansi diff`) and pixels, and shows Qt beside them to eyeball. `gallery.window` opens it in a terminal: `tools/terminal-window <gallery>`.

Placeholders are shell-quoted when filled in. `tools/bindings.d/qt.json` and `rune.json` are working entries. `tools/bindings.py list` shows what's available here; `tools/bindings.py test gtk` runs one binding's tests.
