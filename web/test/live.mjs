#!/usr/bin/env node
// The web live-switch test (spec/following.md), in headless Chromium:
//
//   tools/live/run node web/test/live.mjs
//   OMARCHY_UI_REDUCE_MOTION=1 tools/live/run node web/test/live.mjs
//
// The dev helper follows `omarchy-theme --watch --state` in the test HOME and serves it as events;
// a page imports @omarchy/ui and records its changed hook and every frame's --omarchy-background.
// Then the theme switches the way `omarchy theme set` does (tools/live/theme-set), and the checks
// of live-check.js run: one hook with the new tokens, revision 1, the variable changed, and a fade
// (none under reduced motion).

import { execFileSync } from "node:child_process";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { launch, sleep } from "../lib/cdp.js";
import { serve } from "../dev/serve.js";
import { check } from "./live-check.js";

const web = resolve(dirname(fileURLToPath(import.meta.url)), "..");
const home = process.env.OMARCHY_UI_LIVE_HOME;
const switcher = process.env.OMARCHY_UI_LIVE_SWITCH;
if (!home || !switcher) {
  console.error("run me under tools/live/run");
  process.exit(2);
}
const reduced = process.env.OMARCHY_UI_REDUCE_MOTION === "1";

const server = await serve({ root: web, port: 0 });
const browser = await launch();
let code = 1;
try {
  const page = await browser.newPage({ width: 400, height: 200 });
  await page.goto(`${server.url}/test/pages/live.html`);
  await page.waitFor("window.__probe && window.__probe.ready", { timeout: 10000 });
  // Let the watch settle before switching (its first event has arrived with the page's state).
  await sleep(300);
  execFileSync(switcher, [home, "catppuccin-latte"]);
  await page.waitFor("window.__probe.calls.length > 0", { timeout: 5000 });
  // Another 600 ms for a second hook that mustn't come; the samples run 900 ms after the first.
  await sleep(1000);
  const probe = await page.eval("({ ...window.__probe, revision: omarchy.revision })");
  if (page.errors.length) throw new Error(page.errors.join("\n"));
  const { fail, summary } = check(probe, { reduced });
  if (fail.length) {
    for (const f of fail) console.log(`FAIL: ${f}`);
  } else {
    console.log(`web live switch: ${summary}`);
    code = 0;
  }
} catch (e) {
  console.log(`FAIL: ${e.message}`);
} finally {
  await browser.close();
  server.stop();
}
process.exit(code);
