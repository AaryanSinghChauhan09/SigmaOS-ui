// The core, from Node: find the omarchy-theme CLI, read its state, follow its stream, look icons up.
// Shared by the Electron main process (electron/main.js), the dev helper (dev/serve.js), the
// gallery and the tests. Never resolves a token itself.

import { execFileSync, spawn } from "node:child_process";
import { existsSync, readFileSync } from "node:fs";
import { delimiter, dirname, extname, join, resolve } from "node:path";
import { createInterface } from "node:readline";
import { fileURLToPath } from "node:url";

const here = dirname(fileURLToPath(import.meta.url));
/** This checkout's root (web/lib/../..), for the repo build of the CLI. */
export const REPO = resolve(here, "../..");

/** The omarchy-theme CLI: $OMARCHY_THEME_BIN, else on PATH, else this checkout's build. */
export function findThemeBin(env = process.env) {
  if (env.OMARCHY_THEME_BIN) return env.OMARCHY_THEME_BIN;
  for (const dir of String(env.PATH || "").split(delimiter)) {
    if (!dir) continue;
    const p = join(dir, "omarchy-theme");
    if (existsSync(p)) return p;
  }
  for (const p of [join(REPO, "target/release/omarchy-theme"), join(REPO, "target/debug/omarchy-theme")]) {
    if (existsSync(p)) return p;
  }
  throw new Error(
    "omarchy-theme not found: install the omarchy-ui package, put it on PATH, or set OMARCHY_THEME_BIN",
  );
}

/** The state now, once: { schema, revision, theme, iconTheme, language, tokens, colors, strings }. */
export function readState({ bin = findThemeBin(), env = process.env } = {}) {
  return JSON.parse(execFileSync(bin, ["--state"], { env, encoding: "utf8", maxBuffer: 16 << 20 }));
}

/**
 * Follow `omarchy-theme --watch --state`: `onState(state, isChange)` for the first line (not a
 * change) and every later one (a change, coalesced and deduplicated by the core). Returns
 * { child, stop }.
 */
export function watchState(onState, { bin = findThemeBin(), env = process.env, onExit } = {}) {
  const child = spawn(bin, ["--watch", "--state"], { env, stdio: ["ignore", "pipe", "inherit"] });
  let first = true;
  createInterface({ input: child.stdout }).on("line", (line) => {
    if (!line.trim()) return;
    let state;
    try {
      state = JSON.parse(line);
    } catch {
      return;
    }
    onState(state, !first);
    first = false;
  });
  if (onExit) child.on("exit", onExit);
  return {
    child,
    stop() {
      if (child.exitCode === null) child.kill();
    },
  };
}

const MIME = { ".svg": "image/svg+xml", ".png": "image/png", ".xpm": "image/x-xpixmap", ".jpg": "image/jpeg" };

/** A file as a data: URL (icons for a renderer that can't read files). */
export function dataUrl(path) {
  const type = MIME[extname(path).toLowerCase()] || "application/octet-stream";
  return `data:${type};base64,${readFileSync(path).toString("base64")}`;
}

/** Icon names to file paths at `size` px by the core's lookup: { name: path | null }. */
export function lookupIcons(size, names, { bin = findThemeBin(), env = process.env } = {}) {
  const clean = names.map(String).filter((n) => n && !n.startsWith("-"));
  if (!clean.length) return {};
  const out = execFileSync(bin, ["--icons", String(Math.max(1, Math.round(size))), ...clean], {
    env,
    encoding: "utf8",
  });
  return JSON.parse(out);
}

/** The same, as URLs: data: (default) or file:. */
export function lookupIconUrls(size, names, { as = "data", ...opts } = {}) {
  const paths = lookupIcons(size, names, opts);
  const out = {};
  for (const [n, p] of Object.entries(paths)) {
    if (!p) out[n] = null;
    else if (as === "path") out[n] = p;
    else if (as === "file") out[n] = `file://${p}`;
    else {
      try {
        out[n] = dataUrl(p);
      } catch {
        out[n] = null;
      }
    }
  }
  return out;
}
