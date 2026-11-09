// Electron preload: the renderer's `omarchy`, safe under contextIsolation and the sandbox, with no
// nodeIntegration.
//
//   new BrowserWindow({ webPreferences: { preload: omarchyPreload } })   // from @omarchy/ui/electron
//
// The page gets one global, `omarchy`: omarchy.tokens, omarchy.revision, omarchy.onChange(cb),
// omarchy.string(key, fallback), omarchy.icon(name, size) / omarchy.icons(size, names). It holds
// only the state the main process (followOmarchy) sent; it can't reach Node, files or IPC beyond the
// two channels below. `import "@omarchy/ui"` in the page then applies it as CSS and defines the
// elements.
//
// A sandboxed preload can't load other files, so createOmarchy below is a verbatim copy of
// src/api.js's (the unit test keeps them identical).

const { contextBridge, ipcRenderer } = require("electron");

const initial = ipcRenderer.sendSync("omarchy:state-sync");
let deliver = null;
const queued = [];
ipcRenderer.on("omarchy:state", (_e, state) => {
  if (deliver) deliver(state);
  else queued.push(state);
});

const bridge = {
  initial,
  subscribe(cb) {
    deliver = cb;
    while (queued.length) cb(queued.shift());
  },
  icons(size, names) {
    return ipcRenderer.invoke("omarchy:icons", size, Array.from(names || [], String));
  },
};

function install(bridge) {
  // BEGIN createOmarchy (src/api.js)
  function createOmarchy() {
    let state = null;
    let revision = 0;
    let iconSource = null;
    const iconCache = new Map();
    const before = new Set();
    const after = new Set();

    const same = (a, b) =>
      !!a && !!b && a.theme === b.theme && a.iconTheme === b.iconTheme && a.language === b.language &&
      JSON.stringify(a.tokens) === JSON.stringify(b.tokens);

    const format = (text, args) =>
      String(text).replace(/%(\d+)/g, (m, n) => (args[n - 1] === undefined ? m : String(args[n - 1])));

    const api = {
      /** The whole state: { theme, iconTheme, language, tokens, colors, strings }, or null before the first. */
      get state() { return state; },
      /** The canonical tokens (spec/tokens.md): `omarchy.tokens.palette.background` is "#1a1b26ff". */
      get tokens() { return state ? state.tokens : null; },
      /** 0 for the tokens the page started with, plus one per change since. A cheap cache key. */
      get revision() { return revision; },
      get theme() { return state ? state.theme : ""; },
      get iconTheme() { return state ? state.iconTheme : ""; },
      get language() { return state ? state.language : "en"; },
      /** Every color in the theme's colors.toml by its key (`red`, `bright_blue`, …), as `#rrggbbaa`. */
      get colors() { return (state && state.colors) || {}; },
      get strings() { return (state && state.strings) || {}; },

      /** One of the kit's strings in the display language, `%1`… filled from `args`. */
      string(key, fallback, ...args) {
        const s = state && state.strings && state.strings[key];
        return format(s === undefined || s === null ? (fallback === undefined ? key : fallback) : s, args);
      },

      /** The changed hook: `cb(tokens, state)` once per real change, after the new tokens are in
       *  effect, never for the initial load. Returns a function that removes it. */
      onChange(cb) {
        after.add(cb);
        return () => after.delete(cb);
      },

      /** Icon names to URLs (data: or file:) at `size` device pixels, by the core's lookup:
       *  `{ firefox: "data:image/svg+xml;base64,…", nope: null }`. */
      async icons(size, names) {
        const key = (n) => `${state ? state.iconTheme : ""}\n${size}\n${n}`;
        const out = {};
        const missing = [];
        for (const n of names) {
          if (iconCache.has(key(n))) out[n] = iconCache.get(key(n));
          else missing.push(n);
        }
        if (missing.length && iconSource) {
          const found = (await iconSource(size, missing)) || {};
          for (const n of missing) {
            out[n] = found[n] || null;
            iconCache.set(key(n), out[n]);
          }
        }
        for (const n of missing) if (!(n in out)) out[n] = null;
        return out;
      },

      /** One icon name to a URL, or null. */
      async icon(name, size = 48) {
        return (await api.icons(size, [name]))[name];
      },

      // For transports and the kit, not apps.
      _onBeforeChange(cb) {
        before.add(cb);
        return () => before.delete(cb);
      },
      _setIcons(fn) {
        iconSource = fn;
      },
      _push(next) {
        if (!next || !next.tokens) return false;
        if (state === null) {
          state = next;
          for (const cb of before) cb(state, false);
          return false;
        }
        if (same(state, next)) {
          state = next;
          return false;
        }
        state = next;
        revision += 1;
        iconCache.clear();
        for (const cb of before) cb(state, true);
        for (const cb of after) {
          try {
            cb(state.tokens, state);
          } catch (e) {
            setTimeout(() => { throw e; });
          }
        }
        return true;
      },
    };
    return api;
  }
  // END createOmarchy

  const api = createOmarchy();
  api._setIcons((size, names) => bridge.icons(size, names));
  api._push(bridge.initial);
  bridge.subscribe((state) => api._push(state));
  Object.defineProperty(globalThis, "omarchy", { value: api, configurable: true });
}

contextBridge.executeInMainWorld({ func: install, args: [bridge] });
