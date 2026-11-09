#!/usr/bin/env node
// Copy src/api.js's createOmarchy into electron/preload.cjs (between the BEGIN/END markers), or
// with --check, exit 1 when the copy is stale. A sandboxed preload can't import it.

import { readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { createOmarchy } from "../src/api.js";

const web = join(dirname(fileURLToPath(import.meta.url)), "..");
const path = join(web, "electron/preload.cjs");
const text = readFileSync(path, "utf8");
const re = /( *)\/\/ BEGIN createOmarchy \(src\/api\.js\)\n(?:[\s\S]*?\n)?( *)\/\/ END createOmarchy/;
const m = text.match(re);
if (!m) {
  console.error("preload.cjs: no BEGIN/END createOmarchy markers");
  process.exit(1);
}
const indent = m[1];
const body = createOmarchy
  .toString()
  .split("\n")
  .map((l) => (l ? indent + l : l))
  .join("\n");
const next = text.replace(re, `${indent}// BEGIN createOmarchy (src/api.js)\n${body}\n${indent}// END createOmarchy`);
if (process.argv.includes("--check")) {
  if (next !== text) {
    console.error("electron/preload.cjs: createOmarchy differs from src/api.js (run web/tools/sync-preload.mjs)");
    process.exit(1);
  }
  console.log("preload: createOmarchy matches src/api.js");
} else if (next !== text) {
  writeFileSync(path, next);
  console.log("preload: createOmarchy copied from src/api.js");
}
