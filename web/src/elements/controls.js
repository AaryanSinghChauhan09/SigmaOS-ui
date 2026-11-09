import { OmarchyElement, esc } from "./base.js";

export class SectionHeader extends OmarchyElement {
  static props = { text: ["string", ""] };
  static css = `:host { display: block; color: var(--omarchy-text-dim); font-size: var(--omarchy-font-caption); font-weight: bold; padding-top: calc(var(--omarchy-font-caption) * 0.15); line-height: normal; white-space: pre; }`;
  setup() { this.shadowRoot.innerHTML = `<span id="t"></span><slot></slot>`; }
  update() { this.$("#t").textContent = this.text; }
}

export const elements = { "omarchy-section-header": SectionHeader };
