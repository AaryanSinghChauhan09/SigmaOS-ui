# Using Omarchy UI from C and other languages

For a toolkit with no Omarchy UI binding yet (GTK, Qt Widgets, wxWidgets, an editor, a terminal, Electron, a script), the kit's resolver is available two ways. Both give exactly the tokens the Qt and Rune kits use, follow the desktop the same way ([`spec/following.md`](../spec/following.md)), and leave mapping them onto your toolkit to you:

| | In process: `libomarchy-theme` | Out of process: the `omarchy-theme` CLI |
|---|---|---|
| For | C, C++, and anything with a C FFI (Python `ctypes`/`cffi`, Vala, Zig, Go `cgo`; GObject bindings build on it) | anything that can spawn a process and read lines (Node/Electron, shell, Python without a C dependency) |
| Tokens | `omarchy_theme_tokens_json()` | `omarchy-theme --compact` |
| Following | an fd for your main loop, or a callback | `omarchy-theme --watch`: one JSON line per change |
| Icons, strings | `omarchy_theme_icon_lookup()`, `omarchy_theme_strings_json()` | `--icons`, `--strings`, or all of it with `--state` |

Never parse `colors.toml` or `shell.toml` yourself: the resolution rules ([`spec/tokens.md`](../spec/tokens.md)) are long, and a second implementation drifts.

## The C library

### Install

The `omarchy-ui` Arch package installs it (`packaging/arch/omarchy-ui`, `makepkg -fsi`):

```text
/usr/lib/libomarchy-theme.so.0        the library (soname libomarchy-theme.so.0)
/usr/lib/libomarchy-theme.so          the link-time name
/usr/include/omarchy-theme.h          the API, every function documented
/usr/lib/pkgconfig/omarchy-theme.pc   pkg-config omarchy-theme
```

Add `omarchy-ui` to your package's `depends`. From a checkout, without installing:

```sh
bin/build core                                   # stages build/ffi/{lib,include,lib/pkgconfig}
export PKG_CONFIG_PATH=$PWD/build/ffi/lib/pkgconfig
export LD_LIBRARY_PATH=$PWD/build/ffi/lib        # at run time
```

Then:

```sh
cc app.c $(pkg-config --cflags --libs omarchy-theme)
```

Meson: `dependency('omarchy-theme')`. CMake: `pkg_check_modules(OMARCHY_THEME REQUIRED IMPORTED_TARGET omarchy-theme)`. qmake: `CONFIG += link_pkgconfig` and `PKGCONFIG += omarchy-theme`.

### The API

[`omarchy-theme.h`](../crates/omarchy-theme-ffi/include/omarchy-theme.h) documents each function. Every `char *` returned is yours: free it with `omarchy_theme_free()`. Everything may be called from any thread.

| Function | Returns |
|---|---|
| `omarchy_theme_tokens_json()` | The tokens now, as the canonical JSON of [`spec/tokens.md`](../spec/tokens.md) on one line. Colors are `"#rrggbbaa"`. |
| `omarchy_theme_state_json()` | `{"schema":1,"revision":0,"theme":…,"iconTheme":…,"language":…,"tokens":{…},"strings":{…}}` |
| `omarchy_theme_name()` | The theme's name (`tokyo-night`) |
| `omarchy_theme_icon_theme()` | The freedesktop icon theme the Omarchy theme names (`Yaru-blue` by default) |
| `omarchy_theme_icon_lookup(name, size)` | The file an icon name resolves to at `size` device pixels, by the lookup Omarchy's launcher uses, or `NULL` |
| `omarchy_theme_strings_json()` | The kit's strings in the display language: `{"language":"de","strings":{"common.cancel":"Abbrechen",…}}` |
| `omarchy_theme_string(key)` | One of them, or `NULL` |
| `omarchy_theme_watch_new()` | A watch for a main loop |
| `omarchy_theme_watch_fd(w)` | The fd that turns readable when a change waits |
| `omarchy_theme_watch_take(w)` | The latest change's tokens JSON, or `NULL`; clears the fd |
| `omarchy_theme_watch_current_json(w)` | The tokens the watch has: the initial load, then the latest change |
| `omarchy_theme_watch_revision(w)` | 0, plus one per change: a cache key |
| `omarchy_theme_watch_callback(cb, data, destroy)` | A watch that calls `cb(json, revision, data)` on a background thread |
| `omarchy_theme_watch_free(w)` | Stops and frees either kind |
| `omarchy_theme_abi_version()` | Compare with `OMARCHY_THEME_ABI` |

A watch delivers once per change that alters the tokens (or the icon theme, language or theme name), coalesced over 150 ms; never the initial load. Create the watch first and read the starting tokens from `omarchy_theme_watch_current_json()`, so no change can fall in between.

### A minimal program

```c
#include <omarchy-theme.h>
#include <poll.h>
#include <stdio.h>

int main(void) {
  OmarchyThemeWatch *w = omarchy_theme_watch_new();
  char *json = omarchy_theme_watch_current_json(w);
  printf("now: %s\n", json);
  omarchy_theme_free(json);

  struct pollfd p = {.fd = omarchy_theme_watch_fd(w), .events = POLLIN};
  while (poll(&p, 1, -1) > 0) {
    if ((json = omarchy_theme_watch_take(w))) {
      printf("changed (revision %llu): %s\n", (unsigned long long)omarchy_theme_watch_revision(w), json);
      omarchy_theme_free(json);
    }
  }
  omarchy_theme_watch_free(w);
}
```

Run it and `omarchy theme set` something else: a line per switch.

### In a GLib / GTK main loop

```c
#include <glib-unix.h>
#include <json-glib/json-glib.h>
#include <omarchy-theme.h>

static gboolean on_theme(gint fd, GIOCondition cond, gpointer watch) {
  char *json = omarchy_theme_watch_take(watch);
  if (json) {
    JsonNode *tokens = json_from_string(json, NULL);
    retheme(tokens);                       /* regenerate your CSS, see below */
    json_node_unref(tokens);
    omarchy_theme_free(json);
  }
  return G_SOURCE_CONTINUE;
}

OmarchyThemeWatch *watch = omarchy_theme_watch_new();
g_unix_fd_add(omarchy_theme_watch_fd(watch), G_IO_IN, on_theme, watch);
```

`on_theme` runs on the main thread, so it can touch widgets: that's the changed hook in GLib terms.

### In a Qt Widgets app

```cpp
#include <QJsonDocument>
#include <QSocketNotifier>
#include <omarchy-theme.h>

auto *watch = omarchy_theme_watch_new();
auto *notifier = new QSocketNotifier(omarchy_theme_watch_fd(watch), QSocketNotifier::Read, &app);
QObject::connect(notifier, &QSocketNotifier::activated, &app, [watch] {
  if (char *json = omarchy_theme_watch_take(watch)) {
    applyPalette(QJsonDocument::fromJson(json).object());   // "#rrggbbaa": read alpha last
    omarchy_theme_free(json);
  }
});
```

(A Qt Quick app uses the QML modules instead, [using-qt.md](using-qt.md), which follow by themselves.)

### The callback form

```c
static void on_change(const char *tokens_json, uint64_t revision, void *data) {
  /* A background thread: copy what you need and hand it to your UI thread
     (g_idle_add, QMetaObject::invokeMethod, a pipe). */
}
OmarchyThemeWatch *w = omarchy_theme_watch_callback(on_change, app, NULL);
```

`omarchy_theme_watch_free()` waits for a callback in progress, so don't call it from inside one.

### From Python, without a binding

```python
import ctypes, json
lib = ctypes.CDLL("libomarchy-theme.so.0")
lib.omarchy_theme_tokens_json.restype = ctypes.c_void_p
lib.omarchy_theme_free.argtypes = [ctypes.c_void_p]
p = lib.omarchy_theme_tokens_json()
tokens = json.loads(ctypes.string_at(p))
lib.omarchy_theme_free(p)
print(tokens["palette"]["accent"])
```

## The CLI stream

`omarchy-theme` is the same resolver as a command (the Arch package installs it; `bin/build core` builds `target/release/omarchy-theme`).

| Command | Prints |
|---|---|
| `omarchy-theme` | The canonical token JSON, indented |
| `omarchy-theme --compact` | The same on one line |
| `omarchy-theme --watch` | One compact token line now, then one per change, until stdout closes |
| `omarchy-theme --state` | One line: `{"schema":1,"revision":0,"theme":"tokyo-night","iconTheme":"Yaru-blue","language":"en","tokens":{…},"strings":{…}}` |
| `omarchy-theme --watch --state` | That line now, then one per change, `revision` counting up |
| `omarchy-theme --strings` | `{"language":"en","strings":{…}}` |
| `omarchy-theme --icon-theme` | The icon theme's name |
| `omarchy-theme --icon NAME [SIZE]` | The file `NAME` resolves to (empty when none) |
| `omarchy-theme --icons SIZE NAME…` | `{"firefox":"/usr/share/icons/…/firefox.svg","nope":null}` |

The stream is newline-delimited JSON, flushed per line: the first line is the starting state (revision 0, not a change), and every later line is one change, with the same rules as the C watch. The process runs until it's killed or its reader goes away.

### From Node and Electron

```js
// main process
const { spawn } = require("node:child_process");
const readline = require("node:readline");

const theme = spawn("omarchy-theme", ["--watch", "--state"], { stdio: ["ignore", "pipe", "inherit"] });
readline.createInterface({ input: theme.stdout }).on("line", line => {
  const state = JSON.parse(line);            // { revision, theme, iconTheme, language, tokens, colors, strings }
  for (const win of BrowserWindow.getAllWindows()) win.webContents.send("omarchy-theme", state);
});
app.on("will-quit", () => theme.kill());
```

```js
// renderer: tokens as CSS custom properties
ipcRenderer.on("omarchy-theme", (_e, { revision, tokens }) => {
  const root = document.documentElement.style;            // "#rrggbbaa" is CSS as it is
  root.setProperty("--omarchy-bg", tokens.palette.background);
  root.setProperty("--omarchy-fg", tokens.palette.foreground);
  root.setProperty("--omarchy-radius", `${tokens.radius}px`);
  document.documentElement.dataset.fade = revision > 0 && !tokens.reduceMotion ? "on" : "off";
});
```

## The transition is yours

The library and the CLI deliver tokens; a binding animates. [`spec/following.md`](../spec/following.md) says how, so every toolkit matches the Qt and Rune kits: on a change (never the first paint, never with `tokens.reduceMotion`), ease every color from old to new over `tokens.motion.theme` ms (280) on `cubic-bezier(tokens.motion.ease)` (`0.25, 0.1, 0.25, 1`, CSS's `ease`), and switch rounding, sizes and fonts at once. In CSS:

```css
:root[data-fade="on"] * {
  transition: background-color 280ms ease, color 280ms ease, border-color 280ms ease, fill 280ms ease;
}
```

## Mapping the tokens

[`spec/tokens.md`](../spec/tokens.md) says what each token is for; [`spec/components.md`](../spec/components.md) how the components use them; [`AGENTS.md`](../AGENTS.md) what makes a UI read as Omarchy (sharp corners from Hyprland, foreground glass for controls, one font, flat, keyboard-first, following live). The Qt kit (`qt/qml/Omarchy/Theme`) and the Rune kit (`rune/omarchy-rune/src/lib.rs`, `build_kit`) are two complete mappings to read from.

## Versioning

Pre-1.0, like the rest of the kit. The library's soname is `libomarchy-theme.so.0`, and `OMARCHY_THEME_ABI` changes with it when a signature does; new functions can arrive within one ABI. The JSON's `schema` is 1; keys are only added within it. The package version names the commit (`pacman -Qi omarchy-ui`).
