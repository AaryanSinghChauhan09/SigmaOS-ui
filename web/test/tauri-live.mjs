#!/usr/bin/env node
// The Tauri live-switch test (spec/following.md), in a real Tauri app on WebKitGTK, headless:
//
//   tools/live/run node web/test/tauri-live.mjs
//   OMARCHY_UI_REDUCE_MOTION=1 tools/live/run node web/test/tauri-live.mjs
//
// The app adopts the theme with tauri-plugin-omarchy (the core's watch, in process) and the page
// imports @omarchy/ui, recording its changed hook and every frame's --omarchy-background
// (test/pages/live.js). Then the theme switches the way `omarchy theme set` does
// (tools/live/theme-set) and the checks of live-check.js run: one hook with the new tokens,
// revision 1, the variable changed, and a fade (none under reduced motion). Last, a reload
// starts on the new theme at revision 0.

import { execFileSync } from "node:child_process";
import { check, TO } from "./live-check.js";
import { openTauri } from "../lib/tauri.js";

const home = process.env.OMARCHY_UI_LIVE_HOME;
const switcher = process.env.OMARCHY_UI_LIVE_SWITCH;
if (!home || !switcher) {
  console.error("run me under tools/live/run");
  process.exit(2);
}
const reduced = process.env.OMARCHY_UI_REDUCE_MOTION === "1";
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

let page;
let code = 1;
try {
  page = await openTauri("test/pages/live.html", { width: 400, height: 200 });
  await page.waitFor("window.__probe && window.__probe.ready", { timeout: 10000 });
  // Let the watch settle before switching.
  await sleep(300);
  execFileSync(switcher, [home, "catppuccin-latte"]);
  await page.waitFor("window.__probe.calls.length > 0", { timeout: 5000 });
  // Another 600 ms for a second hook that mustn't come; the samples run 900 ms after the first.
  await sleep(1000);
  const probe = await page.eval("({ ...window.__probe, revision: omarchy.revision })");
  const { fail, summary } = check(probe, { reduced });
  // The Rust side: app.omarchy() and its changed hook.
  const host = await page.host();
  if (host.changes !== 1) fail.push(`the app's Rust on_change fired ${host.changes} times`);
  if (host.revision !== 1) fail.push(`app.omarchy().revision() is ${host.revision}`);
  if (!String(host.background).startsWith(TO)) fail.push(`app.omarchy().tokens() background ${host.background}`);

  // A reload after the switch starts on the new theme, not the one the window was created with.
  await page.reload();
  await page.waitFor("window.__probe && window.__probe.ready", { timeout: 10000 });
  const again = await page.eval("({ ...window.__probe.start, revision: omarchy.revision, calls: window.__probe.calls.length })");
  if (again.background !== `rgb(239, 241, 245)` && again.background !== TO) fail.push(`after a reload the page started on ${again.background}, not ${TO}`);
  if (again.revision !== 0 || again.calls !== 0) fail.push(`after a reload: revision ${again.revision}, ${again.calls} hooks`);
  if (again.theme !== "catppuccin-latte") fail.push(`after a reload the theme is ${again.theme}`);

  if (fail.length) {
    for (const f of fail) console.log(`FAIL: ${f}`);
  } else {
    console.log(`tauri live switch: ${summary}; Rust hook once; a reload starts on the new theme`);
    code = 0;
  }
} catch (e) {
  console.log(`FAIL: ${e.message}`);
} finally {
  if (page) await page.close();
}
process.exit(code);
