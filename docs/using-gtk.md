# Using Omarchy UI from GTK 4

> Paths such as `examples/...` and `gtk/...` are relative to a checkout.

The GTK binding is one C library, **libomarchy-gtk-1** (namespace `Omarchy`, prefix `omarchy_`, `pkg-config omarchy-gtk-1`, `#include <omarchy-gtk.h>`), built on libomarchy-theme ([using-c.md](using-c.md)), so its tokens are exactly the ones the Qt and Rune kits use. It gives any GTK 4 app, in any language:

| | What |
|---|---|
| The one call | `omarchy_gtk_init()` (`Omarchy.init()` through GObject Introspection): stock GTK and libadwaita widgets restyled the Omarchy way, following the theme live |
| `OmarchyTheme` | The live theme as a GObject: the `changed` hook, a `revision`, token accessors, icons by name, the kit's strings |
| The kit | The components of [`spec/components.md`](../spec/components.md) as GObject widgets under the spec's names: `OmarchyButton`, `OmarchyToggle`, `OmarchyListRow`, ... |
| Introspection | `Omarchy-1.gir` / `Omarchy-1.typelib` for Python (PyGObject), GJS and Vala (built when `g-ir-scanner` is installed) |
| Rust | the `omarchy-gtk` crate (`gtk/rust/omarchy-gtk`) for gtk4-rs apps, binding the same library |

## Install

From a checkout:

```sh
cd packaging/arch/omarchy-ui && makepkg -fsi        # libomarchy-theme (the resolver)
cd ../omarchy-ui-gtk && makepkg -fsi                # libomarchy-gtk-1, Omarchy-1.typelib, the gallery
```

Add `omarchy-ui-gtk` to your package's `depends`. Without installing, from a checkout:

```sh
bin/build core && bin/build gtk                     # build/ffi and build/gtk
export PKG_CONFIG_PATH=$PWD/build/gtk/meson-uninstalled:$PWD/build/ffi/lib/pkgconfig
export LD_LIBRARY_PATH=$PWD/build/gtk:$PWD/build/ffi/lib
export GI_TYPELIB_PATH=$PWD/build/gtk                # Python, GJS
```

`gtk/build` uses the system's meson and ninja, or installs them into `build/gtk-venv` with pip.

## Adopting it, per language

Every example below is the same tiny "Network" panel (`examples/gtk-*`), built and rendered by the binding's tests, so each route is proven on every `bin/test`.

### C

```c
#include <omarchy-gtk.h>

static void
activate (GtkApplication *app, gpointer data)
{
  OmarchyTheme *theme = omarchy_gtk_init ();          /* the one call */

  GtkWidget *pane = omarchy_pane_new (NULL);
  omarchy_pane_set_bordered (OMARCHY_PANE (pane), TRUE);
  omarchy_pane_append (OMARCHY_PANE (pane), omarchy_toggle_new ("Wi-Fi", "On"));
  omarchy_pane_append (OMARCHY_PANE (pane), omarchy_button_new ("Forget network"));

  GtkWidget *window = gtk_application_window_new (app);
  gtk_window_set_child (GTK_WINDOW (window), pane);
  gtk_window_present (GTK_WINDOW (window));
}
```

```sh
cc app.c $(pkg-config --cflags --libs omarchy-gtk-1)
```

Meson: `dependency('omarchy-gtk-1')`. CMake: `pkg_check_modules(OMARCHY_GTK REQUIRED IMPORTED_TARGET omarchy-gtk-1)`. The whole example is `examples/gtk-c/main.c`.

### Python (PyGObject)

```python
import gi
gi.require_version("Gtk", "4.0")
gi.require_version("Omarchy", "1")
from gi.repository import Gtk, Omarchy

def activate(app):
    theme = Omarchy.init()                            # the one call
    pane = Omarchy.Pane(bordered=True)
    pane.append(Omarchy.Toggle(label="Wi-Fi", description="On", checked=True))
    pane.append(Omarchy.Button(label="Forget network", bordered=True))
    Gtk.ApplicationWindow(application=app, child=pane).present()

app = Gtk.Application(application_id="org.example.Network")
app.connect("activate", activate)
app.run([])
```

`examples/gtk-python/network.py`.

### GJS

```js
import Gtk from "gi://Gtk?version=4.0";
import Omarchy from "gi://Omarchy?version=1";

const app = new Gtk.Application({ application_id: "org.example.Network" });
app.connect("activate", () => {
  const theme = Omarchy.init();                       // the one call
  const pane = new Omarchy.Pane({ bordered: true });
  pane.append(new Omarchy.Toggle({ label: "Wi-Fi", description: "On", checked: true }));
  new Gtk.ApplicationWindow({ application: app, child: pane }).present();
});
app.run([]);
```

`gjs -m examples/gtk-gjs/network.js`.

### Vala

```vala
var theme = Omarchy.init ();                          // the one call
var pane = new Omarchy.Pane (null);
pane.bordered = true;
pane.append (new Omarchy.Toggle ("Wi-Fi", "On"));
```

`valac --pkg gtk4 --pkg omarchy-gtk-1 network.vala`. The `omarchy-gtk-1.vapi` is generated from the gir by `vapigen` at build time. `examples/gtk-vala/network.vala`.

### Rust (gtk4-rs)

```toml
[dependencies]
omarchy-gtk = { git = "https://github.com/omacom/omarchy-ui" }          # or a path to gtk/rust/omarchy-gtk
```

```rust
use omarchy_gtk::gtk::{self, prelude::*};

fn build(app: &gtk::Application) {
    let theme = omarchy_gtk::init();                  // the one call
    let pane = omarchy_gtk::Pane::new("");
    pane.set_property("bordered", true);
    pane.append(&omarchy_gtk::Toggle::new("Wi-Fi", "On"));
    theme.connect_changed(|theme, tokens| eprintln!("revision {}: {}", theme.revision(), tokens.palette_background()));
    gtk::ApplicationWindow::builder().application(app).child(&pane).build().present();
}
```

The crate re-exports `gtk` (gtk4 0.11) and binds the C library through pkg-config, so the widgets aren't written twice: every kit type is a `glib::wrapper!` (generated from the C headers by `gtk/rust/gen-widgets.py`), with typed methods for the common calls and GObject properties for the rest. It has its own `[workspace]`, outside the repository's. `examples/gtk-rust`.

## What the one call does

`omarchy_gtk_init()` (idempotent; call it in `activate`, or after `gtk_init()`, before the first window shows):

1. **Initializes libadwaita** and sets `AdwStyleManager`'s color scheme from the theme's `mode` (forced dark or light: the Omarchy theme decides, not the GNOME setting).
2. **Installs two CSS providers** on the default display at `GTK_STYLE_PROVIDER_PRIORITY_SETTINGS` (above Adwaita, below your app's CSS and the user's `gtk.css`):
   - *colors*: `:root { --om-*: …; --window-bg-color: …; --accent-bg-color: …; … }`, the palette, the roles, every control state and surface, and libadwaita's own color variables (window, view, headerbar, sidebar, card, dialog, popover, accent, destructive, success, warning, error), so everything libadwaita derives from them follows too;
   - *structure*: the stock widgets restyled from the tokens, referring to colors only through `var()`. Foreground glass for buttons and entries (the normal fill and border, the hover-cursor fill and border under the pointer and on keyboard focus, the selected fill and bold selected text for checked toggles), Hyprland's rounding (`radius`; `min(4, radius)` for small parts; switches are pills only when rounded), the fontconfig `monospace` font at the token sizes (`.title-1` … `.caption`), flat (no shadows, no gradients except a theme's border gradient), focus drawn like hover (no outline), and the accent only for what's chosen (checks, radios, the selected tab's underline, progress, links).
3. **Follows the desktop** through libomarchy-theme's main-loop watch (`omarchy_theme_watch_fd()` added with `g_unix_fd_add()`): theme switches, `omarchy font set`, `omarchy display text size`, Hyprland reloads, `omarchy language`.
4. **Fades colors** on a change, per [`spec/following.md`](../spec/following.md): the colors provider is rewritten on every frame of a 280 ms `ease` fade (`motion.theme`, `motion.ease`), and the kit's widgets redraw from the same interpolated colors; rounding, sizes and fonts switch at once. No fade under reduced motion (Hyprland's animations off, `OMARCHY_UI_REDUCE_MOTION=1`) or on first paint; a second change mid-fade starts from the colors on screen; the fade lands exactly on the tokens.
5. **Registers the kit's types**, so GtkBuilder `.ui` files can name `OmarchyButton` and the rest.

Nothing else is needed: no loop, timer or file watch in the app.

## The changed hook

`OmarchyTheme` is a singleton (`omarchy_theme_get_default()`, also returned by the one call):

| | |
|---|---|
| `::changed (GVariant *tokens)` | Once per real change, never for the initial load, on the main thread, after the new tokens are in effect (every accessor already reads them), with the target tokens as an `a{sv}` in the canonical JSON's shape. Python: `tokens.unpack()["palette"]["background"]`; GJS: `tokens.recursiveUnpack()`; Rust: `Tokens::string("palette.background")`. |
| `revision` (property) | 0 for the initial load, plus one per change: a cache key. |
| `::repaint` | Every frame the colors on screen move (the fade), for custom drawing that should fade with the kit. |
| `::restyle` | Sizes, rounding or fonts may have changed: relayout. |
| `fading`, `dark`, `radius`, `reduce-motion`, `theme-name`, `icon-theme`, `language`, `tokens-json` | Properties, with `notify::` |
| `get_color (path)` | A color by its canonical JSON path (`palette.accent`, `controls.hover.fill`, `surfaces.popups.border.color`), the target |
| `get_paint_color (path)` | The color on screen (mid-fade, a frame between) |
| `get_number (path)`, `get_spacing (token)`, `get_font_size (token)`, `space (px)`, `get_font (token, bold)` | Numbers and fonts |
| `get_tokens_json ()`, `get_tokens ()` | Everything |
| `lookup_icon (name, size)`, `lookup_icon_file` | The file an icon name resolves to in the theme's icon theme (Omarchy's launcher lookup; honors `OMARCHY_UI_ICON_DIRS`, `OMARCHY_UI_ICON_THEME`) |
| `translate (key, fallback)`, `format (key, fallback, arg)` | The kit's strings in the display language (`common.cancel`), as Qt's `I18n.t` |

```c
static void
on_changed (OmarchyTheme *theme, GVariant *tokens, gpointer data)
{
  g_autoptr (GdkRGBA) bg = omarchy_theme_get_color (theme, "palette.background");
  my_renderer_rebuild (bg, omarchy_theme_get_revision (theme));
}
g_signal_connect (omarchy_gtk_init (), "changed", G_CALLBACK (on_changed), NULL);
```

Re-read the icon theme and strings in the hook too: a change can be to those alone.

## Restyled stock widgets versus kit widgets

| | Stock GTK / libadwaita, restyled | Kit widgets (`Omarchy*`) |
|---|---|---|
| How | CSS from the tokens; nothing to change in the app | GObject widgets drawn as the Qt kit draws them |
| Look | Close: the right colors, fonts, rounding, states and flatness, within what CSS reaches in Adwaita's layout | Exact: the spec's metrics, `bin/review` diffs them against Qt |
| Behavior | GTK's | The kit's (the same keys as Qt and Rune: j/k, h/l, Return, Space, Escape, Home/End) |
| For | Existing apps, dialogs, file choosers, preferences windows | New apps and anything that must match Omarchy's own UIs |

Mix freely: they share one theme and fade together. `omarchy-gtk-gallery --section stock` shows the restyled stock widgets beside the kit's sections.

Kit widgets update their own state on a click or key and then emit (GTK's way): `OmarchyToggle` flips `checked` and emits `::toggled`, `OmarchyDropdown` sets `value` and emits `::changed`. Qt's components instead leave that to the caller; read the new state in the handler or bind `notify::`. Containers (`OmarchyCard`, `OmarchyPane`, `OmarchyHeading`, `OmarchySettingRow`, `OmarchySurface`) take children with `append`/`add_*` and from `<child>` in `.ui` files.

## The kit

The components of [`spec/components.md`](../spec/components.md) and `spec/components/*.md`, one GObject type each, under the spec's names:

| Group | Types | Gallery |
|---|---|---|
| Core ([`components.md`](../spec/components.md)) | `OmarchyButton`, `OmarchyIconButton`, `OmarchyButtonGroup`, `OmarchyToggleSwitch`, `OmarchyToggle`, `OmarchyTextField`, `OmarchyNumberField`, `OmarchyDropdown`, `OmarchySlider`, `OmarchyCard`, `OmarchySectionHeader`, `OmarchySeparator`, `OmarchyMenuRow`, `OmarchyTooltip`, `OmarchyConfirmDialog` | `buttons`, `toggles`, `inputs`, `slider`, `surfaces`, `tokens` |
| Surfaces | `OmarchyPane`, `OmarchyHeading` | `surfaces` |
| Lists | `OmarchyListRow`, `OmarchyCursorModel` (a GObject), `OmarchyScrollArea`, `OmarchyKeyValue`, `OmarchyHero`, `OmarchyBadge`, `OmarchyStatusDot`, `OmarchyAvatar`, `OmarchyTileGrid` | `lists`, `tiles` |
| Feedback | `OmarchyMeter`, `OmarchyProgressBar`, `OmarchySpinner`, `OmarchyEmptyState`, `OmarchyBanner`, `OmarchyToast` | `progress`, `messages` |
| Navigation | `OmarchyMenu`, `OmarchyMenuItem`, `OmarchyMenuSection`, `OmarchyMenuSeparator`, `OmarchyContextMenu`, `OmarchyKeycap`, `OmarchyKeyCombo`, `OmarchyShortcutSheet`, `OmarchyToolbar`, `OmarchyToolbarSeparator`, `OmarchyTabs`, `OmarchyIconRail` (and `OmarchyTooltip:shortcut`) | `menus`, `navigation` |
| Forms | `OmarchySettingRow`, `OmarchyCheckbox`, `OmarchyRadio`, `OmarchyRadioGroup`, `OmarchyPasswordField`, `OmarchyTextArea`, `OmarchyMultiSelect`, `OmarchyColorWell`, `OmarchySwatchStrip`, `OmarchyDialog` | `forms`, `pickers` |
| Data and layout | `OmarchyTable`, `OmarchyTree`, `OmarchySplitView`, `OmarchySidebar`, `OmarchyCalendar` | `data`, `layout` |
| Editing | `OmarchyTimeline`, `OmarchyScrubber`, `OmarchyTrimHandle`, `OmarchyCropFrame`, `OmarchyDragHandle`, `OmarchyDropIndicator`, `OmarchyColorPicker` | `timeline`, `color` |
| Composed | `OmarchySearchList` | `composed` |

Each type's properties, signals and keys are in its gtk-doc comment (and so in `Omarchy-1.gir`); its metrics and states are the spec's. `omarchy-gtk-gallery` shows them all in the live theme; `--section NAME` one group.

Plus `OmarchyLabel` (text in a token size and palette role) and `OmarchySurface` (Qt's BorderSurface: a fill and a border spec by token path, for an app's own chips and panels).

## Preview other themes, roundings and icon themes

Everything reads the tokens through libomarchy-theme, so the `OMARCHY_UI_*` overrides ([`spec/tokens.md`](../spec/tokens.md)) apply to any app:

```sh
OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 ./my-app
```

## Render snapshots

Any app that made the one call renders its first window to a PNG and exits when `OMARCHY_GTK_SNAP` is set; `omarchy-gtk-headless` gives it a private broadway display nobody sees (GTK 4 has no offscreen backend), so nothing shows on the desktop:

```sh
OMARCHY_GTK_SNAP=out.png OMARCHY_GTK_SNAP_SCALE=2 omarchy-gtk-headless ./my-app
OMARCHY_GTK_SNAP=out.png omarchy-gtk-headless python3 my-app.py
omarchy-gtk-gallery --snap out.png --section buttons --size 760x220 --scale 2
```

## Limits

- **Stock widgets are close, not exact.** CSS reaches colors, borders, rounding, padding, fonts and states, not every metric Adwaita lays out in code (header bar heights, row layouts of `AdwActionRow`). Where a view must match Omarchy exactly, use the kit widget.
- **`gtk.css` and app CSS win.** The providers sit below `GTK_STYLE_PROVIDER_PRIORITY_APPLICATION` and the user's `~/.config/gtk-4.0/gtk.css`, by design.
- **libadwaita is initialized by the one call**, so a plain GTK app gets Adwaita's stylesheet as the base the Omarchy CSS restyles. The color scheme is forced from the Omarchy theme.
- **Introspection needs `g-ir-scanner` at build time.** Without it (the `gobject-introspection` package), the library, the C and Rust routes and the gallery build; `Omarchy-1.gir`/`.typelib` and the vapi don't, and the Python, GJS and Vala examples report themselves skipped.
- **Window decorations** are the app's (GTK's client-side header bar, restyled). Hyprland draws the border.
- **The fade redraws stock widgets through CSS**, a restyle per frame for 280 ms: cheap for normal windows, measurable for very large widget trees. Reduced motion turns it off.

## Where things are documented

- [`spec/tokens.md`](../spec/tokens.md), [`spec/following.md`](../spec/following.md), [`spec/components.md`](../spec/components.md): the contracts.
- The C API: gtk-doc comments in `gtk/src/omarchy-*.c`, the same text in `Omarchy-1.gir`.
- [using-c.md](using-c.md): libomarchy-theme underneath, for anything this binding doesn't cover.

## Troubleshooting

- **`Package omarchy-gtk-1 was not found`**: install `omarchy-ui-gtk`, or point `PKG_CONFIG_PATH` at `build/gtk/meson-uninstalled` and `build/ffi/lib/pkgconfig`.
- **`Namespace Omarchy not available`** (Python, GJS): the typelib isn't installed or on `GI_TYPELIB_PATH`; it's built only when `g-ir-scanner` was present.
- **Stock widgets don't change**: the one call ran before a display existed (`gdk_display_get_default()` was NULL): call it in `activate`.
