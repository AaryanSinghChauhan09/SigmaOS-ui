// Electron main process: follow the Omarchy theme for every window, with one call.
//
//   import { followOmarchy } from "@omarchy/ui/electron";
//   followOmarchy();                       // before or after app.whenReady()
//   new BrowserWindow({ webPreferences: { preload: omarchyPreload } });
//
// It reads the state now (`omarchy-theme --state`, a few ms), then follows
// `omarchy-theme --watch --state` and pushes each change to every window's renderer, where the
// preload (`@omarchy/ui/electron/preload`) and `@omarchy/ui` apply it. Icons and strings are
// served over IPC. Windows also get the theme's background (no flash on open) and nativeTheme
// follows the theme's mode, so native scrollbars, menus and dialogs match.

import { app, BrowserWindow, ipcMain, nativeTheme, webContents } from "electron";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { findThemeBin, lookupIconUrls, readState, watchState } from "../lib/core.js";

const here = dirname(fileURLToPath(import.meta.url));

/** The preload's path, for `webPreferences.preload`. */
export const omarchyPreload = join(here, "preload.cjs");

let follower = null;

const opaque = (hex) => (typeof hex === "string" && hex.length === 9 ? hex.slice(0, 7) : hex);

/**
 * Follow the theme. Idempotent: a second call returns the first follower.
 *
 * @param {object} [options]
 * @param {string} [options.bin]  the omarchy-theme CLI (default: $OMARCHY_THEME_BIN, PATH, this checkout's build)
 * @param {object} [options.env]  its environment (default: process.env, so OMARCHY_UI_* overrides apply)
 * @param {"data"|"file"|"path"} [options.icons]  how icon lookups answer (default "data": data: URLs)
 * @param {boolean} [options.windowBackground]  set each window's background to the theme's (default true)
 * @param {boolean} [options.nativeTheme]  set nativeTheme.themeSource to the theme's mode (default true)
 */
export function followOmarchy(options = {}) {
  if (follower) return follower;
  const bin = options.bin || findThemeBin(options.env || process.env);
  const env = options.env || process.env;
  const iconsAs = options.icons || "data";
  const listeners = new Set();
  let state = readState({ bin, env });
  let revision = 0;

  const dress = (win) => {
    if (options.windowBackground !== false && !win.isDestroyed()) {
      win.setBackgroundColor(opaque(state.tokens.palette.background));
    }
  };
  const native = () => {
    if (options.nativeTheme !== false) nativeTheme.themeSource = state.tokens.mode === "light" ? "light" : "dark";
  };

  const broadcast = () => {
    for (const wc of webContents.getAllWebContents()) {
      if (!wc.isDestroyed()) wc.send("omarchy:state", state);
    }
    for (const win of BrowserWindow.getAllWindows()) dress(win);
    native();
  };

  ipcMain.on("omarchy:state-sync", (e) => {
    e.returnValue = state;
  });
  ipcMain.handle("omarchy:icons", (_e, size, names) => {
    if (!Array.isArray(names)) return {};
    return lookupIconUrls(Number(size) || 48, names.slice(0, 256), { bin, env, as: iconsAs });
  });

  const onWindow = (_e, win) => dress(win);
  const watch = watchState(
    (next, isChange) => {
      // The stream's first line is the state as it starts; it only matters when it differs from
      // the one read a moment earlier (a switch in between).
      const differs = JSON.stringify(next.tokens) !== JSON.stringify(state.tokens) ||
        next.theme !== state.theme || next.iconTheme !== state.iconTheme || next.language !== state.language;
      if (!isChange && !differs) return;
      state = next;
      if (!differs) return;
      revision += 1;
      broadcast();
      for (const cb of listeners) cb(state.tokens, state);
    },
    { bin, env },
  );

  const start = () => {
    native();
    app.on("browser-window-created", onWindow);
    for (const win of BrowserWindow.getAllWindows()) dress(win);
  };
  if (app.isReady()) start();
  else app.whenReady().then(start);
  app.on("will-quit", () => watch.stop());

  follower = {
    /** The state now. */
    get state() { return state; },
    get tokens() { return state.tokens; },
    /** 0, plus one per change delivered. */
    get revision() { return revision; },
    /** The main process's changed hook: `cb(tokens, state)` once per change. */
    onChange(cb) {
      listeners.add(cb);
      return () => listeners.delete(cb);
    },
    /** Stop following (the windows keep the last theme). */
    stop() {
      watch.stop();
      app.off("browser-window-created", onWindow);
      ipcMain.removeAllListeners("omarchy:state-sync");
      ipcMain.removeHandler("omarchy:icons");
      follower = null;
    },
  };
  return follower;
}
