# Using Omarchy UI from Electron and the web

> `<omarchy-ui-repo>` below stands for this repository's clone URL. It has no public remote yet, so substitute wherever you cloned it from. Paths such as `examples/...` are relative to a checkout and work as written.

`@omarchy/ui` (`web/`) gives any Electron app, and any web UI in a Chromium shell, Omarchy's look and live theme-following with one call. It also gives the full component kit as framework-neutral custom elements (`<omarchy-button>`, `<omarchy-toggle>`, `<omarchy-list-row>`, …), drawn to match the Qt kit. It's plain ES modules with TypeScript declarations and no runtime dependencies. It isn't on npm: depend on it by path.

| Piece | Import | What it does |
|---|---|---|
| Renderer | `@omarchy/ui` | Puts the tokens on `:root` as `--omarchy-*` custom properties, fades colors on a theme switch, restyles plain HTML controls and defines every `<omarchy-*>` element. Exposes `omarchy`: `tokens`, `revision`, `onChange`, strings and icons. |
| Electron main | `@omarchy/ui/electron` | `followOmarchy()`, the one call. It spawns `omarchy-theme --watch --state`, pushes each state to every window and serves icon lookups. `omarchyPreload` is the preload's path. |
| Electron preload | `@omarchy/ui/electron/preload` | `omarchy` in the renderer. Works with `contextIsolation`, `sandbox` and no `nodeIntegration`. |
| Tokens only | `@omarchy/ui/tokens` | `tokensToCss(tokens)` and `tokensToCssText(tokens)`: the canonical JSON mapped to custom properties. |
| No Electron | `@omarchy/ui/dev-server`, `web/bin/omarchy-web-css` | An SSE dev helper, or a static snapshot (see [Outside Electron](#outside-electron)). |

Tokens always come from the core: the `omarchy-theme` CLI resolves them, and JavaScript only names them for CSS. Never re-derive a color in JS ([`spec/tokens.md`](../spec/tokens.md)).

## Add the dependency

```json
{ "dependencies": { "@omarchy/ui": "file:../omarchy-ui/web" } }
```

Point the path at your checkout. The package needs:

- **Node 22 or newer** in the main process.
- **The `omarchy-theme` CLI.** The `omarchy-ui` Arch package installs it, or `bin/build core` builds it into `target/release/`. `followOmarchy()` looks for it in this order: `$OMARCHY_THEME_BIN`, then `PATH`, then the checkout's own build.

## Electron: one call in main, the preload, one import

```js
// main.js
import { app, BrowserWindow } from "electron";
import { followOmarchy, omarchyPreload } from "@omarchy/ui/electron";

followOmarchy();                              // the one call; before or after app.whenReady()

app.whenReady().then(() => {
  const win = new BrowserWindow({
    width: 400,
    height: 283,
    webPreferences: { preload: omarchyPreload, contextIsolation: true, sandbox: true },
  });
  win.loadFile("index.html");
});
```

If you already have a preload, `require("@omarchy/ui/electron/preload")` at its top instead of passing `omarchyPreload`. The preload is self-contained, so it also works sandboxed.

```html
<!-- index.html -->
<script type="module">
  import { omarchy } from "@omarchy/ui";          // through your bundler, or a path to web/src/index.js
</script>
<omarchy-pane bordered>
  <omarchy-heading level="heading" text="Network" subtitle="Wi-Fi and saved networks"></omarchy-heading>
  <omarchy-toggle label="Wi-Fi" description="On" checked></omarchy-toggle>
  <omarchy-button bordered text="Forget network"></omarchy-button>
</omarchy-pane>
```

That is the whole adoption. `followOmarchy()` also does the following:

- **Reads the state once at startup** (`omarchy-theme --state`, a few ms), so the first frame is already themed: no flash and no fade.
- **Sets each window's background** to the theme's, and `nativeTheme.themeSource` to its mode, so native menus, scrollbars and dialogs match. Turn either off with `followOmarchy({ windowBackground: false, nativeTheme: false })`.
- **Answers icon lookups** with data: URLs by default, or `{ icons: "file" }` for `file://` URLs.
- **Returns a follower** with `state`, `tokens`, `revision`, `onChange(cb)` (the main process's own changed hook) and `stop()`.

`OMARCHY_UI_*` overrides in the main process's environment reach the CLI, so `OMARCHY_UI_THEME_DIR=…/fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 electron .` previews another theme.

[`examples/web-electron`](../examples/web-electron) is the complete app, the Electron twin of `examples/rune`. Run it with `npm install && npm start`, or `npm start -- --snap out.png` to render it offscreen. `node web/test/examples.mjs` renders it on every test run.

## The elements

Every component of [`spec/components.md`](../spec/components.md) and [`spec/components/`](../spec/components) is an element named after it: `Button` is `<omarchy-button>`, `ListRow` is `<omarchy-list-row>`, `SearchList` is `<omarchy-search-list>`. `CursorModel` is a class, not an element (`import { CursorModel } from "@omarchy/ui"`), as in Qt.

- **Attributes** are the Qt property names in kebab-case: `status-tone`, `action-glyph`, `current-index`. A few are renamed to fit HTML:
  - `hasCursor` is `cursor`
  - `iconText` is `icon`
  - `tooltipText` is `tooltip`
  - `placeholderText` is `placeholder`
  - `enabled: false` is `disabled`
- **Properties** mirror the attributes in camelCase (`row.statusTone = "success"`).
- **Lists and objects** (`options`, `model`, `items`, `pairs`) take JSON in the attribute, or any value through the property.
- **Events** are `CustomEvent`s named after Qt's signals, with the data in `detail`. Most are listed below; for the rest, read the element's source.

| Qt signal | Event |
|---|---|
| `clicked` | `click` |
| `toggled`, `changed` | `change` (`detail.checked` / `detail.value`) |
| `triggered` | `trigger` |
| `activated` | `activate` |
| `accepted`, `rejected` | `accept`, `reject` |
| `actionClicked` | `action` |
| `dismissed` | `dismiss` |
| `picked` | `pick` |
| `closeRequested` | `close` |
| `deleteRequested` | `delete` |

- **Keyboard first.** Every control is focusable, focus looks like hover, and the keys are Qt's: Space and Return press, `h`/`j`/`k`/`l` beside the arrows where Qt has them, Escape closes.
- **Popups** (dropdowns, menus, tooltips) open in the top layer with the Popover API, so no `overflow` clips them.
- **Strings** shown by default ("Cancel", "Search...", "No matches for “%1”") come from the core's catalog in the display language, and follow `omarchy language`.
- **Icons** by name (`<omarchy-avatar icon-name="firefox">`) resolve through the core's lookup.
- **Escape hatches**: elements expose `::part()` names where a part is worth restyling (`box`, `label`, `input`, `filter`, …).

### React

React 19 passes properties and listens to custom events on custom elements. For older versions, or when unsure, use a ref:

```jsx
import "@omarchy/ui";
import { useEffect, useRef } from "react";

export function WifiToggle({ on, onToggle }) {
  const ref = useRef(null);
  useEffect(() => {
    const el = ref.current;
    const handler = (e) => onToggle(e.detail.checked);
    el.addEventListener("change", handler);
    return () => el.removeEventListener("change", handler);
  }, [onToggle]);
  return <omarchy-toggle ref={ref} label="Wi-Fi" description={on ? "On" : "Off"} checked={on || undefined} />;
}

// Lists through the property:
<omarchy-dropdown label="Theme" value={theme} options={JSON.stringify(themes)} />
```

TypeScript: `@omarchy/ui` declares every tag in `HTMLElementTagNameMap`. For JSX, add `declare namespace JSX { interface IntrinsicElements { [tag: \`omarchy-${string}\`]: any } }`, or a typed map of your own.

### Vue

```js
// vite.config.js: tell Vue these are custom elements
vue({ template: { compilerOptions: { isCustomElement: (tag) => tag.startsWith("omarchy-") } } })
```

```vue
<script setup>
import "@omarchy/ui";
import { ref } from "vue";
const wifi = ref(true);
const theme = ref("tokyo-night");
const themes = [{ value: "tokyo-night", label: "Tokyo Night" }, { value: "nord", label: "Nord" }];
</script>

<template>
  <omarchy-toggle label="Wi-Fi" :checked="wifi" @change="wifi = $event.detail.checked" />
  <omarchy-dropdown label="Theme" :value="theme" .options="themes" @change="theme = $event.detail.value" />
</template>
```

`.options` (with the dot) sets the property; a plain `:options` would stringify it.

### Svelte

```svelte
<script>
  import "@omarchy/ui";
  let wifi = true;
  let value = "center";
  const options = [{ value: "left", label: "Left" }, { value: "center", label: "Center" }, { value: "right", label: "Right" }];
</script>

<omarchy-toggle label="Wi-Fi" checked={wifi} on:change={(e) => (wifi = e.detail.checked)} />
<omarchy-button-group {options} {value} on:change={(e) => (value = e.detail.value)} />
```

Svelte sets properties when the element has them, so arrays go through as they are. In Svelte 5 write `onchange={…}`.

### Plain JavaScript

```js
import "@omarchy/ui";
const row = document.createElement("omarchy-list-row");
Object.assign(row, { glyph: "\u{F0928}", title: "Basecamp", status: "Connected", statusTone: "success" });
row.addEventListener("click", () => connect("Basecamp"));
list.append(row);
```

## Plain HTML, restyled

Importing `@omarchy/ui` also restyles the page's own controls with the tokens, the way the kit draws its own:

| Controls | Drawn like |
|---|---|
| `button`, `input[type=button/submit/reset]`, the file button | a bordered Button. `aria-pressed="true"` draws it selected. |
| text-like `input`, `textarea`, `select` | a TextField. `select` uses Chromium's customizable select, so its list is on the popups surface. |
| `checkbox`, `radio` | the kit's Checkbox and Radio boxes |
| `range` | the Slider |
| `progress`, `meter` | the ProgressBar and Meter |
| `dialog` | the Dialog card on the scrim |
| `[popover]` | the popups surface |
| everything else | Scrollbars, links, `hr`, `kbd`, `fieldset` and selection get the matching tokens too. |

An existing page adopts the look without touching its markup. The restyle sits in its own cascade layer (`@layer omarchy.restyle`) inside `:where()`, so every rule of the page's own still wins. To turn it off, add `<meta name="omarchy-ui" content="no-restyle">`. For a `<link>` instead, use `web/src/restyle.css`, which `web/build` generates. [`examples/web-plain`](../examples/web-plain) is such a page.

## Using the tokens in your own CSS

Every token is a custom property on `:root`:

| Token | Property |
|---|---|
| `palette.accent` | `--omarchy-accent` |
| `roles.textDim` | `--omarchy-text-dim` |
| `controls.hover.fill` | `--omarchy-control-hover-fill` |
| `controls.normal.border` | `--omarchy-control-normal-border`. Its widths are `-border-width` (four values) and `-border-top`, `-right`, `-bottom`, `-left`. |
| `surfaces.popups.border` | `--omarchy-popups-border`. `-border-image` is its gradient or `none`; `-border-solid` is transparent when a gradient draws it. |
| `radius` | `--omarchy-radius`, plus `--omarchy-radius-small` (`min(4, radius)`) |
| `spacing.controlPaddingX` | `--omarchy-space-control-padding-x` |
| `font.caption` | `--omarchy-font-caption`. Its line box is `--omarchy-line-caption`, the height Qt gives the same text. |
| `font.resolvedFamily` | `--omarchy-font-family` |
| `motion.fast` | `--omarchy-motion-fast` (`0ms` under reduced motion) |
| `motion.ease` | `--omarchy-ease` (a `cubic-bezier()`) |

A border spec is four widths. Write it with longhands, not the `border` shorthand, which takes a single width:

```css
.my-panel {
  background: var(--omarchy-control-normal-fill);
  border-style: solid;
  border-width: var(--omarchy-control-normal-border-width);
  border-color: var(--omarchy-control-normal-border);
  border-radius: var(--omarchy-radius);
  padding: var(--omarchy-space-panel-padding);
  font-size: var(--omarchy-font-body);
  line-height: var(--omarchy-line-body);
}
```

`:root` also carries the following:

- `data-omarchy-mode="dark|light"`
- `data-omarchy-reduce-motion` when motion is reduced
- `color-scheme`, so native widgets match

## The changed hook

```js
import { omarchy } from "@omarchy/ui";

omarchy.tokens.palette.background;   // "#1a1b26ff": the canonical JSON (spec/tokens.md)
omarchy.revision;                    // 0 at load, plus one per change: a cache key

const off = omarchy.onChange((tokens, state) => {
  // A canvas, a chart, a terminal: whatever the CSS doesn't reach.
  redraw(tokens.palette.accent, state.iconTheme);
});
```

As [`spec/following.md`](../spec/following.md) requires, the hook:

- **fires once per real change**, never for the initial load and never twice for one switch. The CLI coalesces the switch, and the renderer drops a state identical to the last.
- **runs after the new tokens are in effect.** Inside it, `omarchy.tokens` is the new set, and `getComputedStyle(document.documentElement)` already reads the new non-color properties (radius, sizes, font).
- **runs on the page's main thread** and receives the new tokens.

Inside the hook, the colors are fading. Each `--omarchy-*` color is registered with `@property` as a `<color>`, and a change transitions them over `motion.theme` (280 ms) on `motion.ease`. So a computed color read inside the hook is the first frame of the fade, while `tokens` holds the target. Under reduced motion (`tokens.reduceMotion`, `OMARCHY_UI_REDUCE_MOTION=1`, or `prefers-reduced-motion`) and on first paint there's no fade at all. Everything that isn't a color switches at once. `fading()` tells you whether a fade is running.

The page's own CSS that reads these properties fades with them for free. Don't add `transition` to your own color rules on top.

Other state the hook can use:

- `omarchy.string(key, fallback, ...args)`: the kit's strings in the display language.
- `await omarchy.icon("firefox", 48)` or `await omarchy.icons(48, [...])`: icons through the core's lookup, as data: URLs.
- `omarchy.theme`, `omarchy.iconTheme`, `omarchy.language`.

## Outside Electron

A plain browser page, Tauri, or any other Chromium shell has no preload. `@omarchy/ui` connects on import to whichever transport the page declares.

### Live: the dev helper (SSE)

```sh
node web/dev/serve.js --root path/to/site          # http://127.0.0.1:7357, localhost only
```

```html
<meta name="omarchy-events" content="/omarchy/events">
<script type="module" src="/path/to/web/src/index.js"></script>
```

It follows `omarchy-theme --watch --state` and serves each state as Server-Sent Events. It also answers `/omarchy/state` and `/omarchy/icons?size=48&names=a,b`. Inside your own Node server:

```js
import { omarchyHandler } from "@omarchy/ui/dev-server";
const omarchy = omarchyHandler();
http.createServer((req, res) => omarchy(req, res) || yourApp(req, res));
```

You can also call `connectOmarchy(url)` yourself.

- **Tauri**: emit each line of `omarchy-theme --watch --state` (spawned from Rust) as an event, and call `setOmarchyState(line)` from your listener. Each state after the first that differs counts as a change.
- **Any other shell**: feed `setOmarchyState()` from whatever channel you have.

### Static: a snapshot

For a page that only needs the look and not the live switch:

```sh
web/bin/omarchy-web-css > omarchy-theme.css     # :root { --omarchy-…: …; } as of now
web/bin/omarchy-web-css --html                  # <script type="application/json" id="omarchy-state">…
```

- **`--html`**: paste the script tag into the page. `import "@omarchy/ui"` picks it up and gives you the elements, the restyle and `omarchy.tokens`.
- **The CSS file alone**: the tokens without JS. Add `web/src/restyle.css` for the plain controls.

## The gallery

```sh
web/bin/omarchy-web-gallery                       # a window following the desktop (Electron; else Chromium via the dev helper)
web/bin/omarchy-web-gallery --list-sections
web/bin/omarchy-web-gallery --snap out.png --section buttons --size 760x400 --scale 2
OMARCHY_UI_THEME_DIR=fixtures/themes/catppuccin-latte OMARCHY_UI_ROUNDING=8 web/bin/omarchy-web-gallery --snap out.png --section forms
```

`--snap` renders headless with the system Chromium (`/usr/bin/chromium`, or `$OMARCHY_WEB_CHROMIUM`) over the DevTools pipe. No window ever opens. `bin/review --bindings web` diffs every section against Qt.

## Tests

`tools/bindings.py test web` runs them all. Each runs headless and never opens a window.

| Test | What it covers |
|---|---|
| `web/test/unit.mjs` | Every token of every fixture reaches a property. The renderer API's change rules. The preload's copy of the API. |
| `web/test/behavior.mjs [filter]` | The elements under real keys and clicks over the DevTools protocol, mirroring `qt/tests`: buttons, groups, switches, fields, dropdown, slider, pane, dialogs, menus, tabs, rail, toolbar, checkbox, radios, multiselect, password, swatches, table, tree, calendar, split view, lists and the cursor model, banners and toasts, timeline and the editing handles, SearchList, the restyle. |
| `web/test/examples.mjs` | Renders `examples/web-plain` and `examples/web-electron` to `build/examples/`. |
| `tools/live/run node web/test/live.mjs` | The live switch in Chromium through the dev helper: one hook with the new tokens, revision 1, the variable changed, a fade (none with `OMARCHY_UI_REDUCE_MOTION=1`). |
| `tools/live/run node web/test/electron-live.mjs` | The same in Electron, through `followOmarchy()` and the sandboxed preload, on Chromium's headless platform. It skips when no Electron is installed. |

## Limits

- **Chromium only.** The kit uses the following:
  - `@property`
  - the Popover API
  - relative color syntax
  - CSS `round()`
  - constructable stylesheets
  - `field-sizing`
  - the customizable `select`

  That means Electron 36 or newer and Chrome 135 or newer. Tauri on Linux renders with WebKitGTK, not Chromium, so the tokens and the restyle work there but some elements draw imperfectly. Firefox and Safari aren't targets.
- **Text rasterizes differently from Qt.** Chromium places some sizes' baselines up to about 1.5 px higher than Qt does (14, 18 and 24 px in JetBrains Mono), and its glyph antialiasing differs. Metrics, line boxes, colors and geometry match.
- **No Hyprland of its own.** Rounding, gaps and reduced motion come from the core, which reads Hyprland. A page outside a desktop session gets the core's defaults.
- **ColorPicker's palette row** shows only the palette roles the canonical JSON carries (urgent, success, warning, foreground), not every named color in `colors.toml`. The state has no named colors.
- **The changed hook runs per renderer.** Each window's `omarchy` counts its own revision. Main's `follower.onChange` is the process-wide hook.
- **The dev helper is for development.** It binds to 127.0.0.1 and serves the files under `--root`.
