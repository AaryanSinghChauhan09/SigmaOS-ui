// Every element of the kit (spec/components.md and spec/components/*.md), under the spec's names
// as <omarchy-kebab-name>. Each group lives in its own file.

import * as controls from "./controls.js";

const groups = [controls];

/** Define every <omarchy-*> element (idempotent). `import "@omarchy/ui"` does this. */
export function defineElements() {
  for (const g of groups) {
    for (const [tag, cls] of Object.entries(g.elements)) cls.define(tag);
  }
}

export * from "./controls.js";
