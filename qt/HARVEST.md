# Where the Qt binding comes from

The Qt binding is the Omarchy shell's own QML kit, lifted out of Quickshell. `bin/harvest-qt` copies it from an Omarchy checkout (`$OMARCHY_SRC`, default `~/omarchy`, on `quattro`) and records the commit in `HARVEST_REV`.

| Kind | Files | How it gets here |
|---|---|---|
| Harvested | `Ui/*.qml` listed in `bin/harvest-qt`, `Theme/Border.qml`, `Theme/BorderGeometry.js`, `Theme/Numbers.js` | Copied verbatim; only imports change (`qs.Commons` → `Omarchy.Theme`) |
| Patched | `qt/patches/*.patch` | Applied on top of the harvest. Each fixes something the shell doesn't hit because it runs under the desktop's platform style or inside Quickshell. Candidates to send upstream. |
| Generated | `Theme/Strings.js` | The strings the components show, extracted from `default/i18n` for every language (every key an `I18n.t` call in `Ui/` names). The Rune binding reads the same file through `omarchy_theme::strings`. |
| Hand-ported | `Theme/Color.qml`, `Theme/Style.qml`, `Theme/Util.qml`, `Theme/I18n.qml` | The shell's logic with its I/O swapped for `src/themesource.cpp` (see below). Never overwritten by a harvest. |
| Own | `Ui/Card.qml`, `Theme/Tokens.qml`, `src/*` | Written here |

## Updating

```sh
git -C ~/omarchy pull
bin/harvest-qt --check     # what changed upstream, including in the hand-ported files
bin/harvest-qt             # copy, patch, record the rev
bin/review                 # token parity + side-by-side renders
```

If `--check` lists a hand-ported file, port that diff by hand. If a patch stops applying, upstream touched the same lines: re-do the fix or drop the patch if upstream fixed it.

## What the hand ports change

- **Color.qml**: `FileView`s for `colors.toml`, `shell.toml` and `~/.config/omarchy/shell.toml` become `ThemeSource.colorsText` / `shellText` / `userShellText`, reloaded on `ThemeSource.themeChanged`. Surface `text` roles go through `flatColor`, so a role name (`foreground`) works where the shell only takes a hex string. Adds `dark`, from `colors.toml`'s `mode`.
- **Style.qml**: `cornerRadius`, `gapsOut`, `reduceMotion` and `resolvedFontFamily` bind to `ThemeSource` instead of `hyprctl` / `fc-match` processes and Quickshell's Hyprland events. `menuFontFamily` no longer reads `OMARCHY_MENU_FONT`. Adds `windowBorder` (Hyprland's `general:border_size`). Adds `themeFade` and `ease`, the theme-switch fade's duration and curve.
- **Util.qml**: drops `execDetached` / `execArgv` (Quickshell processes) and the bar-layout helpers.
- **I18n.qml**: same interface, backed by `Strings.js` and `~/.config/omarchy/language` instead of the full catalogs; `lookup` lets an app route keys through its own translator.
