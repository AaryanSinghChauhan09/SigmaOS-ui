// Find an Electron to run: $OMARCHY_WEB_ELECTRON, the package's own (npm i -D electron), `electron`
// on PATH, or Arch's versioned electronNN.

import { existsSync, readdirSync } from "node:fs";
import { delimiter, dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const web = resolve(dirname(fileURLToPath(import.meta.url)), "..");

export function findElectron(env = process.env) {
  if (env.OMARCHY_WEB_ELECTRON) return env.OMARCHY_WEB_ELECTRON;
  const local = join(web, "node_modules/.bin/electron");
  if (existsSync(local)) return local;
  const dirs = String(env.PATH || "").split(delimiter).filter(Boolean);
  for (const dir of dirs) {
    if (existsSync(join(dir, "electron"))) return join(dir, "electron");
  }
  // Arch: electron43, electron42, … (newest first).
  const versioned = [];
  for (const dir of dirs) {
    let names = [];
    try {
      names = readdirSync(dir);
    } catch {
      continue;
    }
    for (const n of names) {
      const m = /^electron(\d+)$/.exec(n);
      if (m) versioned.push([Number(m[1]), join(dir, n)]);
    }
  }
  versioned.sort((a, b) => b[0] - a[0]);
  return versioned.length ? versioned[0][1] : null;
}

/** Flags that keep Electron off the desktop: Chromium's headless Ozone platform, no GPU. */
export const HEADLESS_FLAGS = ["--ozone-platform=headless", "--disable-gpu", "--headless"];
