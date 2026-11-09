# Using Omarchy UI from Qt Widgets and KDE apps

> `<omarchy-ui-repo>` below stands for this repository's clone URL. It has no public remote yet, so substitute wherever you cloned it from. Paths such as `examples/...` and `kde/...` are relative to a checkout and work as written.

`OmarchyWidgets` (`kde/`) brings a Qt Widgets, KDE Frameworks or Kirigami app onto the live Omarchy theme with one call, and gives Qt Widgets apps the whole component kit ([`spec/components.md`](../spec/components.md)) as widgets. Its tokens come from `libomarchy-theme`, the same resolver every other binding reads ([`docs/using-c.md`](using-c.md)); it never parses the theme itself.

| | What you get |
|---|---|
| The one call, `Omarchy::Widgets::apply(app)` | Qt's own widgets (buttons, line edits, checkboxes, radios, combos, sliders, spin boxes, tabs, menus, scrollbars, tooltips, item views, group boxes) drawn the Omarchy way, the palette and font from the theme, KDE's color scheme and Kirigami kept in step, all following the desktop live |
| `Omarchy::Theme` | The tokens as Qt types, the changed hook, a revision, icon lookup and the kit's strings |
| The kit, `Omarchy::Button`, `Omarchy::Toggle`, `Omarchy::ListRow`, ... | Every component of the spec as a `QWidget`, drawn as the Qt Quick kit draws it (same metrics, states, keys and strings) |
| `omarchy-widgets-gallery` | Every component, live; `--snap` renders a section offscreen |

## Install

The Arch package needs `omarchy-ui` (for `libomarchy-theme`) first:

```sh
cd packaging/arch/omarchy-ui && makepkg -fsi
cd ../omarchy-ui-kde && makepkg -fsi
```

```text
/usr/lib/libOmarchyWidgets.so.0                         the library
/usr/include/OmarchyWidgets/Omarchy/*.h                  #include <Omarchy/Widgets.h>
/usr/lib/cmake/OmarchyWidgets/                           find_package(OmarchyWidgets)
/usr/lib/pkgconfig/omarchy-widgets.pc                    pkg-config omarchy-widgets
/usr/bin/omarchy-widgets-gallery
```

Add `omarchy-ui-kde` to your package's `depends`. From a checkout, without installing:

```sh
bin/build core && bin/build kde               # build/ffi (libomarchy-theme), build/kde
cmake -S myapp -B myapp/build -DOmarchyWidgets_DIR=$PWD/build/kde
```

CMake:

```cmake
find_package(OmarchyWidgets REQUIRED)
target_link_libraries(myapp PRIVATE Omarchy::Widgets)
```

pkg-config (qmake: `PKGCONFIG += omarchy-widgets`; Meson: `dependency('omarchy-widgets')`):

```sh
c++ main.cpp $(pkg-config --cflags --libs omarchy-widgets)
```

## Qt Widgets apps

One call after the `QApplication` exists ([`examples/kde-widgets`](../examples/kde-widgets)):

```cpp
#include <QApplication>
#include <Omarchy/Widgets.h>

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  Omarchy::Widgets::apply(app);          // or Omarchy::init(app)

  QWidget window;                        // stock widgets, kit widgets, or both
  ...
  return app.exec();
}
```

Nothing else: no timer, no file watch, no restart. Stock widgets you already have restyle themselves; kit widgets are there when you want Omarchy's own components.

### What the one call does

1. **Reads the theme** through `libomarchy-theme` and **watches it**: the library's watch fd sits on a `QSocketNotifier` on the GUI thread, so `omarchy theme set`, `omarchy font set`, `omarchy display text size`, a Hyprland reload (rounding, gaps, animations) and `omarchy language` reach the app as one change each ([`spec/following.md`](../spec/following.md)).
2. **Sets the palette** from the tokens (`Omarchy::Widgets::palette(tokens)`): Window and Base are the background, text the foreground, placeholder `textFaint`, Highlight the selection fill composited on the background, Link and Accent the accent, tooltips the tooltip surface; disabled text is `textFaint`.
3. **Installs `Omarchy::Style`**, a `QProxyStyle` over Fusion that draws the stock widgets the Omarchy way: controls are foreground glass with the theme's state fills and borders, corners are Hyprland's rounding (small parts `min(4, radius)`), everything is flat, focus looks like hover, and accent marks only what is chosen (a check, a radio dot, the current tab's underline, a menu row's selected text). Paddings, row heights, the slider, scrollbar and progress thickness come from the spacing tokens.
4. **Sets the application font**: fontconfig's `monospace` at the `body` size (menus get `body`, tooltips `bodySmall`).
5. **With KF6** (KColorScheme and KConfig, built in when installed): writes the theme as a KDE color scheme to `$XDG_RUNTIME_DIR/omarchy-ui/Omarchy-<pid>.colors` and applies it the way `KColorSchemeManager` applies one (the `KDE_COLOR_SCHEME_PATH` application property, then the palette), so every `KColorScheme` in the app, and Kirigami, read Omarchy's colors.
6. **Picks KDE's desktop Qt Quick Controls style** (`org.kde.desktop`, from qqc2-desktop-style) for any Qt Quick views in the app, unless `QT_QUICK_CONTROLS_STYLE` is already set: it draws its controls with the QStyle, so they get `Omarchy::Style` too.
7. **Follows**: on a change, sizes, rounding and fonts switch at once (widgets relayout as on a style change), and colors fade over `motion.theme` (280 ms) on `motion.ease`, re-applying the palette and color scheme each frame; under reduced motion, and on the first paint, they land at once.

Each part can be turned off. To keep your own style and take only the palette, font and scheme:

```cpp
Omarchy::Widgets::Options options;
options.style = false;                 // also: palette, font, kdeColorScheme, quickControlsStyle
Omarchy::Widgets::apply(app, options);
```

### The changed hook

For what the binding can't reach (a custom-painted widget with cached colors, a cache of measured text, a terminal you theme yourself):

```cpp
#include <Omarchy/Theme.h>

QObject::connect(Omarchy::Theme::instance(), &Omarchy::Theme::changed, view, [view](const QJsonObject &tokens) {
  view->setAccent(Omarchy::Tokens::parseColor(tokens["palette"].toObject()["accent"].toString()));
  view->update();
});
```

- `changed(QJsonObject tokens)` fires once per real change (coalesced over 150 ms, only when something differs), on the GUI thread, never for the theme the app started with. When it runs, `tokens()`, `revision()`, `name()`, `iconTheme()`, `language()` and `string()` already hold the new values. The argument is the canonical JSON of [`spec/tokens.md`](../spec/tokens.md): the target values, not a frame of the fade.
- `revision()` is 0 at start, plus one per change: a cache key (`if (cachedRevision != theme->revision()) rebuild();`).
- `colorsChanged()` fires on the change and on every frame of the fade; `shown()` is the tokens with the colors on screen. Paint from `shown()` and repaint on `colorsChanged` to fade along with everything else. `metricsChanged()` fires when sizes or fonts change. `fadingChanged(bool)` and `fading()` say whether a fade runs.

### Tokens in your own painting

`Omarchy::Theme::instance()->tokens()` (or `shown()` while painting) is an `Omarchy::Tokens`: `foreground()`, `background()`, `accent()`, `urgent()`, `textDim()`, `textFaint()`, `track()`; `color("surfaces.popups.background")` for any color by its JSON path; `border("surfaces.menu.border")` and `border(Omarchy::State::Hover)` for border specs; `fill(Omarchy::State::Selected)` for state fills; `spacing("panelPadding")`, `space(22)`, `radius()`, `smallRadius()`; `font("title", bold)`; `duration(ms)` (0 under reduced motion). `Omarchy::Paint` draws surfaces with border specs and text the way the kit does. `Theme::iconPath(name, size)` is Omarchy's icon lookup and `Theme::string(key, english)` the kit's strings in the display language.

## KDE Frameworks apps

KDE apps outside Plasma pick their colors in two places, and the one call covers both:

- **`KColorScheme`** (KColorScheme, used by KDE widgets and many apps' own painting) reads the color scheme file named by the application property `KDE_COLOR_SCHEME_PATH` (what `KColorSchemeManager::activateScheme()` sets), else `kdeglobals`. Outside Plasma nothing usually writes `kdeglobals`, so apps fall back to Breeze. `apply()` writes the Omarchy scheme and sets the property; `KColorScheme(QPalette::Active, KColorScheme::View)` and friends read it, and re-read it on every change.
- **The palette**: Qt widgets in a KDE app read `QPalette`, which `apply()` sets (KDE's own platform theme isn't loaded outside Plasma).

So a KXmlGui / KF6 app needs the same single call, right after constructing its `QApplication` (before `KAboutData::setApplicationData` is fine either way):

```cpp
QApplication app(argc, argv);
Omarchy::Widgets::apply(app);
KAboutData about(...);
KAboutData::setApplicationData(about);
```

If the app has a "Color Scheme" menu (`KColorSchemeManager`), picking a scheme there overrides Omarchy's until the next theme change. The scheme Omarchy writes isn't listed in that menu (it lives in the runtime directory, per process). Use `Omarchy::Widgets::kdeColorScheme(tokens)` to export it as a `.colors` file if you want it listed.

## Kirigami and other QML apps

**Kirigami apps** take the same one call ([`examples/kde-kirigami`](../examples/kde-kirigami)), with a `QApplication`:

```cpp
QApplication app(argc, argv);
Omarchy::Widgets::apply(app);                   // before the engine loads QML
QQmlApplicationEngine engine(QUrl("qrc:/Main.qml"));
```

The QML stays plain Kirigami. `Kirigami.Theme` colors come from the KDE color scheme through Kirigami's `org.kde.desktop` platform plugin, and Qt Quick Controls (buttons, fields, combos, checkboxes, sliders) are drawn by `Omarchy::Style` through qqc2-desktop-style. Both follow live (`omarchy-widgets-kirigami-live-test` checks it). Kirigami's own pieces (its switch, inline messages, page headers) keep Kirigami's shapes in Omarchy's colors. Set `QT_QUICK_CONTROLS_STYLE` yourself and `apply()` leaves the controls style alone.

**QML apps that want Omarchy's own components** use the Qt Quick kit, `Omarchy.Ui` ([`using-qt.md`](using-qt.md)): `import Omarchy.Theme as Theme; import Omarchy.Ui as O`, which follows the theme by itself and is what this binding's widgets are drawn to match. Run those views under the Basic controls style (`QT_QUICK_CONTROLS_STYLE=Basic`), set before `apply()` so it isn't replaced. A Widgets app embedding such a view (`QQuickWidget`) can use both.

## Restyled stock widgets or kit widgets

| | Stock Qt widgets under `Omarchy::Style` | Kit widgets (`Omarchy::*`) |
|---|---|---|
| Use for | an existing app; standard dialogs; item views over your models; anything a KDE app already builds | new UI that should look and behave exactly like Omarchy's shell and the other bindings |
| Look | Omarchy's colors, glass, rounding, font and paddings, within Qt's widget anatomy (a spin box keeps its editor; a combo its popup list) | the spec's anatomy and metrics, pixel-matched to the Qt Quick kit (`bin/review`) |
| Keyboard | Qt's own | the kit's (vim keys where the kit has them, a panel cursor via `hasCursor`) |
| Works with another style | no (it is the style) | yes: kit widgets draw themselves and only read the theme |

Mixing is normal: an app keeps its `QTreeView` and `QDialogButtonBox` and adds `Omarchy::Toggle` and `Omarchy::ListRow` where it wants Omarchy's components.

## The component kit

`#include <Omarchy/Widgets.h>` brings all of them, in namespace `Omarchy`; each group also has its own header.

| Group | Header | Widgets | Gallery sections |
|---|---|---|---|
| Buttons | `Omarchy/Button.h` | `Button`, `IconButton`, `ButtonGroup` | `buttons` |
| Toggles | `Omarchy/Toggle.h` | `ToggleSwitch`, `Toggle` | `toggles` |
| Inputs | `Omarchy/Inputs.h`, `Omarchy/Slider.h` | `TextField` (a `QLineEdit`), `NumberField`, `Dropdown`, `Slider` | `inputs`, `slider` |
| Surfaces ([`surfaces.md`](../spec/components/surfaces.md)) | `Omarchy/Surfaces.h` | `Card`, `Pane`, `Heading`, `SectionHeader`, `Separator`, `MenuRow`, `TooltipFrame`, `ConfirmDialog`, plus `Surface` and `Label` for your own views | `surfaces`, `tokens` |
| Lists ([`lists.md`](../spec/components/lists.md)) | `Omarchy/Lists.h` | `ListRow`, `CursorModel`, `ScrollArea`, `KeyValue`, `Hero`, `Badge`, `StatusDot`, `Avatar`, `TileGrid` | `lists`, `tiles` |
| Feedback ([`feedback.md`](../spec/components/feedback.md)) | `Omarchy/Feedback.h` | `Meter`, `ProgressBar`, `Spinner`, `EmptyState`, `Banner`, `Toast` | `progress`, `messages` |
| Navigation ([`navigation.md`](../spec/components/navigation.md)) | `Omarchy/Navigation.h` | `Menu` (`MenuItem`, `MenuSection`, `MenuSeparator`, submenus), `ContextMenu`, `Keycap`, `KeyCombo`, `ShortcutSheet`, `Tooltip` (with a shortcut), `Toolbar`, `ToolbarSeparator`, `Tabs`, `IconRail` | `menus`, `navigation` |
| Forms ([`forms.md`](../spec/components/forms.md)) | `Omarchy/Forms.h` | `SettingRow`, `Checkbox`, `Radio`, `RadioGroup`, `PasswordField`, `TextArea` (a `QPlainTextEdit`), `MultiSelect`, `ColorWell`, `SwatchStrip`, `Dialog` | `forms`, `pickers` |
| Data and layout ([`data.md`](../spec/components/data.md)) | `Omarchy/Data.h` | `Table` (rows, or any `QAbstractItemModel`), `Tree`, `SplitView` (a `QSplitter`), `Sidebar`, `Calendar` | `data`, `layout` |
| Editing ([`editing.md`](../spec/components/editing.md)) | `Omarchy/Editing.h` | `Timeline`, `Scrubber`, `TrimHandle`, `CropFrame`, `DragHandle`, `DropIndicator`, `ColorPicker` | `timeline`, `color` |
| Composed ([`composed.md`](../spec/components/composed.md)) | `Omarchy/Composed.h` | `SearchList` | `composed` |

The names are the spec's; the API is Qt's way of saying the same thing (`setChecked` and `toggled(bool)` for `checked` and `onToggled`, `setOptions({{value, label}})` for `options`, a `QWidget` you pass for a QML slot). Where the Qt Quick kit leaves a value to its caller (a `Toggle` emits `clicked` and the app flips `checked`), the widget flips it by default and says so in its header (`setAutoToggle(false)` hands it back). The comment over each class in its header lists its properties, signals and keys; [`kde/gallery/sections`](../kde/gallery/sections) has every one in use.

Every kit widget:

- follows the theme on its own (repaints through the fade, relayouts when sizes change);
- has `hasCursor`, which forces the hover-cursor look for keyboard-driven lists and still renders;
- reserves the widest border any of its states paints, so hovering never relayouts;
- lays out with `Omarchy::Row` / `Omarchy::Column` when it composes others: box layouts with Qt Quick Layouts' rules (implicit sizes rounded up, children centered on whole pixels), which you can use in your own views too.

## The gallery

```sh
build/kde/omarchy-widgets-gallery                                   # a window, following the desktop
build/kde/omarchy-widgets-gallery --list-sections
build/kde/omarchy-widgets-gallery --snap out.png --section buttons --size 760x400 --scale 2
OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 build/kde/omarchy-widgets-gallery --snap latte.png --section toggles
```

It has every [`spec/gallery.md`](../spec/gallery.md) section, plus `stock` (Qt's own widgets under the style). `gallery/sections/*.cpp` is example code for every component. `bin/review --bindings kde` renders it beside Qt Quick and diffs every section; every `OMARCHY_UI_*` override works ([`spec/tokens.md`](../spec/tokens.md)).

## Limits

- **Qt 6 only** (6.5 or newer), Widgets required. KF6 is optional at build time; without it, KDE's color scheme isn't written and Kirigami keeps its own colors.
- **Window decorations** are the compositor's: Hyprland draws none, other desktops draw their own.
- **Rounded popups** (menus, tooltips, the combo list) need a compositor for their translucent corners; on a sharp theme (Omarchy's default) nothing changes.
- **Stock widgets keep Qt's anatomy.** A `QSpinBox` puts − and + inside the field like `NumberField`, but a `QComboBox` popup, a `QTabWidget` or a `QFileDialog` is Qt's layout in Omarchy's colors, not the kit's component.
- **Kirigami's own controls** (its switch, cards, inline messages) are Kirigami's shapes in Omarchy's colors; only Qt Quick Controls go through `Omarchy::Style`.
- **Explicit palettes and style sheets win.** A widget that sets its own `QPalette` or style sheet keeps it, and doesn't follow the theme for those colors.
- **One fade at a time.** A switch during a fade starts a new fade from the colors on screen (as the spec asks); custom painting follows only if it paints from `shown()` on `colorsChanged`.
