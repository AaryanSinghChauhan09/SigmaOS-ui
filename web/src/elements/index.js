// Every element of the kit (spec/components.md and spec/components/*.md), under the spec's names
// as <omarchy-kebab-name>. Each group lives in its own file.

import * as controls from "./controls.js";
import * as surfaces from "./surfaces.js";
import * as forms from "./forms.js";

const groups = [controls, surfaces, forms];

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

export { Button, IconButton, ButtonGroup, ToggleSwitch, Toggle, TextField, NumberField, Dropdown, Slider } from "./controls.js";
export { SectionHeader, Separator, Card, MenuRow, Tooltip, Dialog, ConfirmDialog, Heading, Pane } from "./surfaces.js";
export { SettingRow } from "./forms.js";
export { OmarchyElement } from "./base.js";
