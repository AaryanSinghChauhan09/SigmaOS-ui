#!/usr/bin/env node
// Node-only checks of the binding's plumbing: the token → CSS mapping against the core's JSON for
// every fixture, the renderer API's change rules (spec/following.md), and the preload's copy of it.

import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { readdirSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { readState, REPO } from "../lib/core.js";
import { createOmarchy } from "../src/api.js";
import { tokensToCss, tokensToCssText } from "../src/tokens.js";

const web = resolve(dirname(fileURLToPath(import.meta.url)), "..");
let checks = 0;
const ok = (cond, msg) => {
  assert.ok(cond, msg);
  checks++;
};

// Every fixture, both roundings: every color in the canonical JSON reaches a custom property with
// the same value, and every number reaches one too.
const fixtures = readdirSync(join(REPO, "fixtures/themes"));
for (const theme of fixtures) {
  for (const radius of ["0", "8"]) {
    const env = { ...process.env, OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes", theme), OMARCHY_UI_ROUNDING: radius };
    const { tokens } = readState({ env });
    const { props, colors } = tokensToCss(tokens);
    const values = new Set(Object.values(props));
    const walk = (v, path) => {
      if (typeof v === "string" && /^#[0-9a-f]{8}$/.test(v)) ok(values.has(v), `${theme}: ${path} (${v}) has no property`);
      else if (Array.isArray(v) || v === null || typeof v !== "object") return;
      else for (const [k, x] of Object.entries(v)) walk(x, `${path}.${k}`);
    };
    walk(tokens, "");
    ok(props["--omarchy-background"] === tokens.palette.background, `${theme}: background`);
    ok(props["--omarchy-radius"] === `${radius}px`, `${theme}: radius`);
    ok(props["--omarchy-radius-small"] === `${Math.min(4, Number(radius))}px`, `${theme}: small radius`);
    ok(props["--omarchy-space-control-height"] === `${tokens.spacing.controlHeight}px`, `${theme}: spacing`);
    ok(props["--omarchy-font-caption"] === `${tokens.font.caption}px`, `${theme}: font size`);
    ok(props["--omarchy-font-family"].includes(tokens.font.resolvedFamily || "monospace"), `${theme}: family`);
    ok(props["--omarchy-ease"] === `cubic-bezier(${tokens.motion.ease.join(", ")})`, `${theme}: ease`);
    ok(colors.every((c) => c in props), `${theme}: colors are properties`);
    ok(!colors.includes("--omarchy-radius"), `${theme}: radius is no color`);
    const pop = tokens.surfaces.popups.border;
    if (pop.gradient) {
      ok(props["--omarchy-popups-border-image"].startsWith("linear-gradient("), `${theme}: gradient border`);
      ok(props["--omarchy-popups-border-solid"] === "#00000000", `${theme}: gradient hides the solid border`);
    } else {
      ok(props["--omarchy-popups-border-image"] === "none" && props["--omarchy-popups-border-solid"] === pop.color, `${theme}: solid border`);
    }
  }
}

// Reduced motion: every duration is 0 (spec/tokens.md, Motion).
{
  const env = { ...process.env, OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes/tokyo-night"), OMARCHY_UI_REDUCE_MOTION: "1" };
  const { props } = tokensToCss(readState({ env }).tokens);
  ok(props["--omarchy-motion-theme"] === "0ms" && props["--omarchy-motion-fast"] === "0ms", "reduced motion zeroes durations");
  const env2 = { ...env, OMARCHY_UI_REDUCE_MOTION: "0" };
  ok(tokensToCss(readState({ env: env2 }).tokens).props["--omarchy-motion-theme"] === "280ms", "motion.theme is 280ms");
  ok(tokensToCssText(readState({ env }).tokens).startsWith(":root {"), "snapshot CSS");
}

// The API's change rules.
{
  const base = readState({ env: { ...process.env, OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes/tokyo-night") } });
  const other = readState({ env: { ...process.env, OMARCHY_UI_THEME_DIR: join(REPO, "fixtures/themes/catppuccin-latte") } });
  const o = createOmarchy();
  const order = [];
  o._onBeforeChange((s, isChange) => order.push(isChange ? "apply-change" : "apply-initial"));
  o.onChange((tokens) => order.push(`hook:${tokens.palette.background}`));
  o._push(base);
  ok(o.revision === 0 && order.join() === "apply-initial", "the initial load is no change");
  o._push(structuredClone(base));
  ok(o.revision === 0 && order.length === 1, "an identical state is no change");
  o._push(other);
  ok(o.revision === 1, "a change counts");
  ok(order.join() === "apply-initial,apply-change,hook:#eff1f5ff", `the hook runs after the tokens apply: ${order}`);
  ok(o.tokens.palette.background === "#eff1f5ff", "tokens read the new state");
  o._push({ ...other, language: "de", strings: { "common.cancel": "Abbrechen" } });
  ok(o.revision === 2, "a language change is a change");
  ok(o.string("common.cancel", "Cancel") === "Abbrechen", "strings follow the language");
  ok(o.string("common.noMatchesFor", "No matches for “%1”", "x") === "No matches for “x”", "fallback with %1");
  const off = o.onChange(() => order.push("second"));
  off();
  o._push(base);
  ok(!order.includes("second"), "onChange returns an unsubscribe");
  let asked = null;
  o._setIcons(async (size, names) => {
    asked = [size, names];
    return { firefox: "data:x" };
  });
  const got = await o.icons(32, ["firefox", "nope"]);
  ok(got.firefox === "data:x" && got.nope === null && asked[0] === 32, "icon lookup");
  asked = null;
  await o.icon("firefox", 32);
  ok(asked === null, "icon lookups are cached per icon theme and size");
}

// The preload's copy of createOmarchy.
execFileSync(process.execPath, [join(web, "tools/sync-preload.mjs"), "--check"]);
checks++;

console.log(`unit: ${checks} checks over ${fixtures.length} fixtures at radius 0 and 8`);
