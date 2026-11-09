// Every element of the kit (spec/components.md and spec/components/*.md), under the spec's names
// as <omarchy-kebab-name>. Each group lives in its own file and exports `elements`: { tag: class }.

import * as controls from "./controls.js";
import * as surfaces from "./surfaces.js";
import * as forms from "./forms.js";
import * as pickers from "./pickers.js";
import * as lists from "./lists.js";
import * as feedback from "./feedback.js";
import * as navigation from "./navigation.js";
import * as data from "./data.js";
import * as editing from "./editing.js";
import * as composed from "./composed.js";

const groups = [controls, surfaces, forms, pickers, lists, feedback, navigation, data, editing, composed];

/** Define every <omarchy-*> element (idempotent). `import "@omarchy/ui"` does this. */
export function defineElements() {
  for (const g of groups) {
    for (const [tag, cls] of Object.entries(g.elements)) cls.define(tag);
  }
}

/** Every element's tag and class. */
export function elementTags() {
  return Object.assign({}, ...groups.map((g) => g.elements));
}

export * from "./controls.js";
export * from "./surfaces.js";
export * from "./forms.js";
export * from "./pickers.js";
export * from "./lists.js";
export * from "./feedback.js";
export * from "./navigation.js";
export * from "./data.js";
export * from "./editing.js";
export * from "./composed.js";
export { OmarchyElement } from "./base.js";
