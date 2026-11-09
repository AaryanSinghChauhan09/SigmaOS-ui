// Drive a page in the Tauri gallery app on WebKitGTK, headless (its --drive mode: a private
// broadway display, no window on the desktop), for the Tauri tests:
//
//   const page = await openTauri("test/pages/live.html", { width: 400, height: 200 });
//   await page.eval("omarchy.revision");          // awaited in the page, JSON back
//   await page.snap("/abs/out.png");             // the page at its device scale
//   await page.reload();
//   await page.close();
//
// PAGE is a path under web/, served by the gallery's gallery: protocol. The app adopts the theme
// as any Tauri app does (tauri-plugin-omarchy), under whatever OMARCHY_UI_* the caller's
// environment has.

import { spawn } from "node:child_process";
import { existsSync } from "node:fs";
import { join } from "node:path";
import { createInterface } from "node:readline";
import { REPO } from "./core.js";

/** The gallery app: $OMARCHY_TAURI_GALLERY, else this checkout's build (web/tauri/build). */
export function findTauriGallery(env = process.env) {
  if (env.OMARCHY_TAURI_GALLERY) return env.OMARCHY_TAURI_GALLERY;
  const p = join(REPO, "web/tauri/target/release/omarchy-tauri-gallery");
  return existsSync(p) ? p : null;
}

export async function openTauri(page, { width = 400, height = 200, scale = 2, env = process.env, timeout = 30000 } = {}) {
  const bin = findTauriGallery(env);
  if (!bin) throw new Error("no omarchy-tauri-gallery (web/tauri/build)");
  const child = spawn(bin, ["--drive", page, "--size", `${width}x${height}`, "--scale", String(scale)], {
    env,
    stdio: ["pipe", "pipe", "pipe"],
  });
  let stderr = "";
  child.stderr.on("data", (d) => (stderr += d));
  const waiting = new Map();
  let ready;
  const readyP = new Promise((resolve, reject) => (ready = { resolve, reject }));
  let exited = false;
  child.on("exit", (code) => {
    exited = true;
    const err = new Error(`omarchy-tauri-gallery exited (${code})\n${stderr.trim().split("\n").slice(-10).join("\n")}`);
    ready.reject(err);
    for (const w of waiting.values()) w.reject(err);
    waiting.clear();
  });
  createInterface({ input: child.stdout }).on("line", (line) => {
    let msg;
    try {
      msg = JSON.parse(line);
    } catch {
      return;
    }
    if ("ready" in msg) {
      if (msg.ready) ready.resolve();
      else ready.reject(new Error(msg.err || "the page didn't load"));
      return;
    }
    const w = waiting.get(msg.id);
    if (!w) return;
    waiting.delete(msg.id);
    if ("err" in msg) w.reject(new Error(msg.err));
    else w.resolve(msg.ok);
  });
  const timer = setTimeout(() => ready.reject(new Error("the page didn't load in time")), timeout);
  try {
    await readyP;
  } catch (e) {
    child.kill("SIGKILL");
    throw e;
  } finally {
    clearTimeout(timer);
  }

  let next = 1;
  const request = (req) => {
    if (exited) return Promise.reject(new Error("omarchy-tauri-gallery has exited"));
    const id = next++;
    return new Promise((resolve, reject) => {
      waiting.set(id, { resolve, reject });
      child.stdin.write(JSON.stringify({ id, ...req }) + "\n");
    });
  };
  return {
    eval: (expression) => request({ eval: expression }),
    snap: (out) => request({ snap: out }),
    size: (w, h) => request({ size: [w, h] }),
    reload: () => request({ reload: true }),
    /** The app's Rust side: { revision, changes (its on_change calls), background }. */
    host: () => request({ host: true }),
    async waitFor(expression, { timeout: ms = 5000, every = 20 } = {}) {
      const start = Date.now();
      for (;;) {
        if (await request({ eval: `!!(${expression})` }).catch(() => false)) return;
        if (Date.now() - start > ms) throw new Error(`timed out waiting for ${expression}`);
        await new Promise((r) => setTimeout(r, every));
      }
    },
    get stderr() {
      return stderr;
    },
    async close() {
      if (exited) return;
      child.stdin.end();
      await new Promise((r) => {
        const t = setTimeout(() => {
          child.kill("SIGKILL");
          r();
        }, 5000);
        child.on("exit", () => {
          clearTimeout(t);
          r();
        });
      });
    },
  };
}
