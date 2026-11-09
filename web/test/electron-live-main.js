// The Electron side of electron-live.mjs: a hidden window (never shown; Chromium's headless
// platform) adopting the theme exactly as an app does, followOmarchy() in main and the preload,
// then the switch and the checks of live-check.js. Prints one line and exits 0 when it passed.

import { app, BrowserWindow } from "electron";
import { execFileSync } from "node:child_process";
import { followOmarchy, omarchyPreload } from "../electron/main.js";
import { check } from "./live-check.js";

const follower = followOmarchy();
let mainHooks = 0;
follower.onChange(() => (mainHooks += 1));

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
const finish = (code, line) => {
  console.log(line);
  app.exit(code);
};

app.whenReady().then(async () => {
  try {
    const win = new BrowserWindow({
      show: false,
      width: 400,
      height: 200,
      // Offscreen: it paints (so frames, the timeline and the fade run) without ever being shown.
      webPreferences: { preload: omarchyPreload, contextIsolation: true, sandbox: true, nodeIntegration: false, offscreen: true, backgroundThrottling: false },
    });
    await win.loadFile(new URL("./pages/live.html", import.meta.url).pathname);
    const js = (e) => win.webContents.executeJavaScript(e);
    for (let i = 0; !(await js("!!(window.__probe && window.__probe.ready)")); i++) {
      if (i > 250) throw new Error("the page never got the state");
      await sleep(20);
    }
    const api = await js("({ type: typeof omarchy, keys: Object.keys(omarchy).join(','), node: typeof require, proc: typeof process })");
    if (api.node !== "undefined" || api.proc !== "undefined") throw new Error(`the page reaches Node (${JSON.stringify(api)})`);
    await sleep(300);
    execFileSync(process.env.OMARCHY_UI_LIVE_SWITCH, [process.env.OMARCHY_UI_LIVE_HOME, "catppuccin-latte"]);
    for (let i = 0; !(await js("window.__probe.calls.length > 0")); i++) {
      if (i > 250) throw new Error("the changed hook never fired");
      await sleep(20);
    }
    await sleep(1000);
    const probe = await js("({ ...window.__probe, revision: omarchy.revision })");
    const icon = await js("omarchy.icon('no-such-icon', 32)");
    const { fail, summary } = check(probe, { reduced: process.env.OMARCHY_UI_REDUCE_MOTION === "1" });
    if (mainHooks !== 1) fail.push(`main's onChange fired ${mainHooks} times`);
    if (follower.revision !== 1) fail.push(`main's revision is ${follower.revision}`);
    if (icon !== null) fail.push(`an unknown icon looked up as ${icon}`);
    if (fail.length) finish(1, fail.map((f) => `FAIL: ${f}`).join("\n"));
    else finish(0, `electron live switch: ${summary}; main hook once`);
  } catch (e) {
    finish(1, `FAIL: ${e.message}`);
  }
});
