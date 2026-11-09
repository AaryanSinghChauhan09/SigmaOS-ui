#!/usr/bin/env node
// The Tauri bridge (tauri-plugin-omarchy) in a real Tauri app on WebKitGTK, headless:
//
//   node web/test/tauri.mjs
//
// - before first paint: the page's first script already has `omarchy` with the core's state, and
//   the first frame shows the theme's background with no fade;
// - the state is the core's (`omarchy-theme --state`): tokens, strings, colors, theme, icon theme;
// - revision 0 and no changed hook on load;
// - icons: the core's lookup through the plugin's protocol, files served by name only, never by
//   a path from the page.

import { join } from "node:path";
import { readState, REPO } from "../lib/core.js";
import { openTauri } from "../lib/tauri.js";

const icons = join(REPO, "fixtures/icons");
const env = {
  ...process.env,
  OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes/catppuccin-latte"),
  OMARCHY_UI_ROUNDING: "8",
  OMARCHY_UI_ICON_DIRS: `${icons}/local:${icons}/system:${icons}/pixmaps`,
  OMARCHY_UI_ICON_THEME: "Fixture-Accent",
};
const fail = [];
const expect = (ok, what) => {
  if (!ok) fail.push(what);
};

const core = readState({ env });
const opaque = core.tokens.palette.background.slice(0, 7);
const rgb = (hex) => `rgb(${parseInt(hex.slice(1, 3), 16)}, ${parseInt(hex.slice(3, 5), 16)}, ${parseInt(hex.slice(5, 7), 16)})`;

let page;
try {
  page = await openTauri("test/pages/tauri.html", { width: 400, height: 200, env });
  await page.waitFor("window.__probe && window.__probe.first", { timeout: 10000 });

  const early = await page.eval("window.__early");
  expect(early.hasApi, "no omarchy before the page's first script");
  expect(early.background === core.tokens.palette.background, `the first script read ${early.background}, not ${core.tokens.palette.background}`);
  expect(early.revision === 0, `revision ${early.revision} in the first script`);
  expect(early.strings > 0, "no strings in the first script");
  expect(early.colors > 0, "no colors in the first script");

  const first = await page.eval("window.__probe.first");
  expect(first.body === rgb(opaque), `the first frame's background was ${first.body}, not ${rgb(opaque)}`);
  expect(!first.fadeAttribute && !first.fading, "the first frame was fading");
  expect(first.mode === core.tokens.mode, `data-omarchy-mode ${first.mode} on the first frame`);

  const st = await page.eval("({ tokens: omarchy.tokens, strings: omarchy.strings, colors: omarchy.colors, theme: omarchy.theme, iconTheme: omarchy.iconTheme, language: omarchy.language, revision: omarchy.revision, calls: window.__probe.calls })");
  expect(JSON.stringify(st.tokens) === JSON.stringify(core.tokens), "omarchy.tokens differ from the core's");
  expect(JSON.stringify(st.strings) === JSON.stringify(core.strings), "omarchy.strings differ from the core's");
  expect(JSON.stringify(st.colors) === JSON.stringify(core.colors), "omarchy.colors differ from the core's");
  expect(st.theme === core.theme && st.iconTheme === core.iconTheme && st.language === core.language,
    `theme/iconTheme/language ${st.theme}/${st.iconTheme}/${st.language}, not ${core.theme}/${core.iconTheme}/${core.language}`);
  expect(st.revision === 0, `revision ${st.revision} after load`);
  expect(st.calls === 0, `the changed hook fired ${st.calls} times on load`);
  expect((await page.eval("omarchy.string('Cancel', 'x')")) === (core.strings.Cancel ?? "x"), "omarchy.string differs from the core's");
  expect((await page.eval("getComputedStyle(document.documentElement).getPropertyValue('--omarchy-radius').trim()")) === "8px", "--omarchy-radius isn't the core's 8px");

  // Icons: by name through the core's lookup, as URLs on the plugin's protocol that load.
  const found = await page.eval("omarchy.icons(48, ['firefox', 'steam', 'btop', 'no-such-icon', '/etc/passwd', '../../etc/passwd'])");
  for (const n of ["firefox", "steam", "btop"]) expect(typeof found[n] === "string" && found[n].startsWith("omarchy://"), `icon ${n} looked up as ${found[n]}`);
  for (const n of ["no-such-icon", "/etc/passwd", "../../etc/passwd"]) expect(found[n] === null, `icon ${n} looked up as ${found[n]}`);
  const loaded = await page.eval(`Promise.all(${JSON.stringify([found.firefox, found.steam])}.map((src) => new Promise((resolve) => {
    const img = new Image();
    img.onload = () => resolve(img.naturalWidth);
    img.onerror = () => resolve(0);
    img.src = src;
  })))`);
  expect(loaded.every((w) => w > 0), `icons didn't load from the protocol (widths ${loaded})`);
  // Only names: a path, absolute or relative, is never read.
  const probes = await page.eval(`Promise.all(["icon/48/%2Fetc%2Fpasswd", "icon/48/..%2F..%2Fetc%2Fpasswd", "icon/48/passwd", "nothing"].map((p) =>
    fetch("omarchy://localhost/" + p).then((r) => r.status, () => 0)))`);
  expect(probes.every((s) => s === 404 || s === 0), `the protocol answered a path: ${probes}`);
  const one = await page.eval("omarchy.icon('utilities-terminal', 48)");
  expect(typeof one === "string", `omarchy.icon('utilities-terminal') was ${one}`);

  // The element that shows icons, end to end.
  const avatar = await page.eval(`(async () => {
    const a = document.createElement("omarchy-avatar");
    a.setAttribute("icon-name", "firefox");
    document.body.append(a);
    for (let i = 0; i < 200 && (window.__omarchyPending || 0) > 0; i++) await new Promise((r) => setTimeout(r, 10));
    await new Promise((r) => setTimeout(r, 50));
    const img = a.shadowRoot.querySelector(".icon");
    return { src: img.getAttribute("src") || "", w: img.naturalWidth };
  })()`);
  expect(avatar.src.startsWith("omarchy://") && avatar.w > 0, `<omarchy-avatar icon-name=firefox> showed ${JSON.stringify(avatar)}`);
} catch (e) {
  fail.push(e.message);
} finally {
  if (page) await page.close();
}

if (fail.length) {
  for (const f of fail) console.log(`FAIL: ${f}`);
  process.exit(1);
}
console.log("tauri bridge: state before first paint, the core's tokens, strings and colors, revision 0, icons by name only");
