// Forms and pickers (spec/components/forms.md), drawn as qt/qml/Omarchy/Ui draws them.

import { OmarchyElement } from "./base.js";
import { space } from "./looks.js";

// ---------------------------------------------------------------------------------------------
// SettingRow: words on the left, any control (the children) on the right.

export class SettingRow extends OmarchyElement {
  static props = { label: ["string", ""], description: ["string", ""], cursor: ["bool"], disabled: ["bool"] };
  static css = `
:host {
  display: flex; align-items: center; gap: var(--omarchy-space-xxl); box-sizing: border-box;
  width: ${space(240)};
  min-height: calc(var(--omarchy-space-control-height) + 2 * var(--omarchy-space-md));
  padding: var(--omarchy-space-md) var(--omarchy-space-row-padding-x);
  border-radius: var(--omarchy-radius);
}
:host([cursor]) { background: var(--omarchy-control-hover-fill); }
.words { flex: 1; min-width: 0; display: flex; flex-direction: column; gap: var(--omarchy-space-xxs); }
.label { font-size: var(--omarchy-font-body); line-height: var(--omarchy-line-body, normal); white-space: pre; overflow: hidden; text-overflow: ellipsis; }
.desc { font-size: var(--omarchy-font-caption); line-height: var(--omarchy-line-caption, normal); color: var(--omarchy-text-dim); overflow-wrap: anywhere; }
.desc:empty { display: none; }
:host([disabled]) .label, :host([disabled]) .desc { color: var(--omarchy-text-faint); }
.control { display: flex; align-items: center; gap: var(--omarchy-space-md); flex: none; }
`;
  setup() {
    this.shadowRoot.innerHTML = `<div class="words"><div class="label" part="label"></div><div class="desc" part="description"></div></div><div class="control"><slot></slot></div>`;
  }
  update() {
    this.$(".label").textContent = this.label;
    this.$(".desc").textContent = this.description;
  }
}

export const elements = { "omarchy-setting-row": SettingRow };
