#!/usr/bin/env node
// Behavior tests of the elements in headless Chromium, driving real keys and clicks through the
// DevTools protocol, as qt/tests (tst_*.qml) and the Rune gallery tests drive theirs.
//
//   node web/test/behavior.mjs [filter]

import assert from "node:assert/strict";
import { dirname, join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import { bridgePage, launch, sleep, SHIFT } from "../lib/cdp.js";
import { readState, REPO } from "../lib/core.js";
import { readdirSync } from "node:fs";

const here = dirname(fileURLToPath(import.meta.url));
// Every file under test/cases/ exports `tests`: { name: async (t) => … }.
const tests = {};
for (const f of readdirSync(join(here, "cases")).filter((f) => f.endsWith(".js")).sort()) {
  Object.assign(tests, (await import(pathToFileURL(join(here, "cases", f)).href)).tests);
}
const filter = process.argv[2] || "";
const env = { ...process.env, OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes/tokyo-night"), OMARCHY_UI_ROUNDING: "0", OMARCHY_UI_REDUCE_MOTION: "1" };
const state = readState({ env });

const browser = await launch();
let passed = 0;
const failed = [];
try {
  const page = await browser.newPage({ width: 900, height: 700 });
  await bridgePage(page, state, async () => ({}));
  const url = pathToFileURL(join(here, "pages/harness.html")).href;
  for (const [name, fn] of Object.entries(tests)) {
    if (filter && !name.includes(filter)) continue;
    await page.goto(url);
    await page.waitFor("window.harnessReady");
    const t = {
      page,
      assert,
      SHIFT,
      sleep,
      async mount(html) {
        await page.eval(`document.body.innerHTML = ${JSON.stringify(html)}; new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)))`);
      },
      eval: (e) => page.eval(e),
      key: (k, o) => page.key(k, o),
      type: (s) => page.type(s),
      click: (sel) => page.clickOn(sel),
      box: (sel) => page.box(sel),
      async focus(sel) {
        // A real Tab-free focus that counts as keyboard focus (:focus-visible): focus, then a no-op key.
        await page.eval(`__q(${JSON.stringify(sel)}).focus()`);
      },
      events: () => page.eval("window.events"),
      clearEvents: () => page.eval("window.events = []"),
      prop: (sel, p) => page.eval(`__q(${JSON.stringify(sel)})[${JSON.stringify(p)}]`),
      frame: () => page.eval("new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)))"),
    };
    try {
      await fn(t);
      if (page.errors.length) throw new Error(`page errors: ${page.errors.join("; ")}`);
      passed++;
    } catch (e) {
      failed.push(`${name}: ${e.message.replace(/\s+/g, " ").slice(0, 300)}`);
    }
    page.errors.length = 0;
  }
} finally {
  await browser.close();
}
for (const f of failed) console.log(`FAIL ${f}`);
console.log(`behavior: ${passed} passed, ${failed.length} failed`);
process.exit(failed.length ? 1 : 0);
