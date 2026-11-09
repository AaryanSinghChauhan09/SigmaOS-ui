#!/usr/bin/env node
// Render each web example once, offscreen, in a fixture theme, to build/examples/<name>.png (as
// bin/test-examples does the Qt and Rune ones):
//
//   web-plain     examples/web-plain/index.html in headless Chromium, the state given as the
//                 dev helper would (no server needed)
//   web-electron  examples/web-electron: `npm install` (its one dependency is file:../../web),
//                 then the app's own --snap, Electron on the headless platform

import { execFileSync, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, readFileSync, statSync } from "node:fs";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import { bridgePage, launch } from "../lib/cdp.js";
import { readState, REPO } from "../lib/core.js";
import { findElectron, HEADLESS_FLAGS } from "../lib/electron.js";

const out = join(REPO, "build/examples");
mkdirSync(out, { recursive: true });
const env = { ...process.env, OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes/tokyo-night"), OMARCHY_UI_ROUNDING: "0", OMARCHY_UI_REDUCE_MOTION: "1" };
const isPng = (p) => existsSync(p) && statSync(p).size > 1000 && readFileSync(p).subarray(1, 4).toString() === "PNG";
let failed = 0;

// web-plain
{
  const png = join(out, "web-plain.png");
  const browser = await launch();
  try {
    const page = await browser.newPage({ width: 456, height: 560, scale: 2 });
    await bridgePage(page, readState({ env }), async () => ({}));
    await page.goto(pathToFileURL(join(REPO, "examples/web-plain/index.html")).href);
    await page.waitFor("customElements.get('omarchy-toggle') && document.fonts.status === 'loaded'");
    await page.eval("new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)))");
    if (page.errors.length) throw new Error(page.errors.join("; "));
    await page.screenshot(png);
  } catch (e) {
    console.log(`examples: web-plain failed: ${e.message}`);
    failed++;
  } finally {
    await browser.close();
  }
  if (isPng(png)) console.log(`examples: web-plain rendered to build/examples/web-plain.png`);
  else failed++;
}

// web-electron
{
  const electron = findElectron();
  const dir = join(REPO, "examples/web-electron");
  const png = join(out, "web-electron.png");
  if (!electron) {
    console.log("examples: web-electron skipped (no Electron)");
  } else {
    try {
      execFileSync("npm", ["install", "--offline", "--no-audit", "--no-fund", "--loglevel=error"], { cwd: dir, stdio: "ignore" });
      const e2 = { ...env };
      delete e2.WAYLAND_DISPLAY;
      delete e2.DISPLAY;
      const r = spawnSync(electron, [...HEADLESS_FLAGS, dir, "--snap", png], { env: e2, timeout: 30000, encoding: "utf8" });
      if (!isPng(png)) throw new Error(`exit ${r.status}`);
      console.log("examples: web-electron rendered to build/examples/web-electron.png");
    } catch (e) {
      console.log(`examples: web-electron failed: ${e.message}`);
      failed++;
    }
  }
}
process.exit(failed ? 1 : 0);
