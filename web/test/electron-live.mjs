#!/usr/bin/env node
// The Electron live-switch test: tools/live/run node web/test/electron-live.mjs. Runs
// electron-live-main.js in Electron on Chromium's headless platform (no window, no display), and
// passes its verdict through. Skips (exit 0, saying so) when no Electron is installed.

import { spawn } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { findElectron, HEADLESS_FLAGS } from "../lib/electron.js";

const electron = findElectron();
if (!electron) {
  console.log("electron live switch: skipped (no Electron: pacman -S electron, or npm i -D electron in web/)");
  process.exit(0);
}
const main = join(dirname(fileURLToPath(import.meta.url)), "electron-live-main.js");
const env = { ...process.env };
delete env.WAYLAND_DISPLAY;
delete env.DISPLAY;
const child = spawn(electron, [...HEADLESS_FLAGS, main], { env, stdio: ["ignore", "pipe", "pipe"] });
let out = "";
let err = "";
child.stdout.on("data", (d) => (out += d));
child.stderr.on("data", (d) => (err += d));
const timer = setTimeout(() => child.kill("SIGKILL"), 30000);
child.on("exit", (code) => {
  clearTimeout(timer);
  const lines = out.split("\n").filter((l) => l.trim());
  const verdict = lines.filter((l) => /^(FAIL|electron live switch)/.test(l));
  if (verdict.length) console.log(verdict.join("\n"));
  else console.log(`FAIL: no verdict from Electron (exit ${code})\n${err.split("\n").filter((l) => !/gl|egl|angle|dbus|gpu|viz/i.test(l)).slice(-15).join("\n")}`);
  process.exit(code === 0 && verdict.length && !verdict.some((l) => l.startsWith("FAIL")) ? 0 : 1);
});
