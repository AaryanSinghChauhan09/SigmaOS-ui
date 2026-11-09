// What every element shares: a shadow root with the kit's base sheet, attributes reflected as
// properties, events, the kit's strings, and a re-render when the theme's language changes.

import { omarchyApi } from "../runtime.js";

const sheets = new Map();

/** A constructable stylesheet for `css`, built once per distinct text. */
export function sheet(css) {
  let s = sheets.get(css);
  if (!s) {
    s = new CSSStyleSheet();
    s.replaceSync(css);
    sheets.set(css, s);
  }
  return s;
}

/** CSS shared by every element's shadow root. Fills and borders read the --omarchy-* tokens
 *  (src/tokens.js); nothing is hard-coded that a token covers. */
export const BASE_CSS = `
:host {
  box-sizing: border-box;
  font-family: var(--omarchy-font-family);
  font-size: var(--omarchy-font-body); line-height: var(--omarchy-line-body, normal);
  color: var(--omarchy-foreground);
  -webkit-tap-highlight-color: transparent;
}
:host([hidden]) { display: none !important; }
*, *::before, *::after { box-sizing: border-box; }
.glyph { font-family: var(--omarchy-font-family); white-space: pre; }
.label { white-space: pre; overflow: hidden; text-overflow: ellipsis; }
/* A border drawn from a border token (color + 4 widths); a gradient border (surfaces only) draws as
   an overlay masked to the border ring, so corners round with it. */
.gradient-ring { position: absolute; pointer-events: none; border-radius: inherit;
  inset: calc(-1 * var(--ring-top, 0px)) calc(-1 * var(--ring-right, 0px)) calc(-1 * var(--ring-bottom, 0px)) calc(-1 * var(--ring-left, 0px));
  padding: var(--ring-top, 0px) var(--ring-right, 0px) var(--ring-bottom, 0px) var(--ring-left, 0px);
  background: var(--ring-image, none);
  -webkit-mask: linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0);
  mask: linear-gradient(#000 0 0) content-box exclude, linear-gradient(#000 0 0); }
`;

/** Declarations that draw `surface`'s border token (popups, tooltip, menu, notifications, polkit):
 *  a solid border, or a gradient ring when the theme gives one. Pair with a .gradient-ring child. */
export function surfaceBorder(surface, key = "border") {
  const v = `--omarchy-${surface}-${key}`;
  return `border-style: solid;
  border-width: var(${v}-width);
  border-color: var(${v}-solid);
  --ring-top: var(${v}-top); --ring-right: var(${v}-right); --ring-bottom: var(${v}-bottom); --ring-left: var(${v}-left);
  --ring-image: var(${v}-image);`;
}

const kebab = (s) => s.replace(/[A-Z]/g, (c) => "-" + c.toLowerCase());
const camel = (s) => s.replace(/-([a-z])/g, (_, c) => c.toUpperCase());

/**
 * The base element. Subclasses declare
 *   static css = "…"                         their shadow stylesheet
 *   static props = { name: [type, default] } type: "bool" | "string" | "number" | "json"
 *   setup()                                  build the shadow DOM once
 *   update()                                 sync it with the properties (any change, and a theme change)
 */
export class OmarchyElement extends HTMLElement {
  static css = "";
  static props = {};
  static focusable = false;

  static get observedAttributes() {
    return Object.keys(this.props).map(kebab);
  }

  static define(tag) {
    for (const [name, [type, def]] of Object.entries(this.props)) {
      if (Object.prototype.hasOwnProperty.call(this.prototype, name)) continue;
      const attr = kebab(name);
      Object.defineProperty(this.prototype, name, {
        configurable: true,
        get() {
          if (type === "json") {
            if (this._json && name in this._json) return this._json[name];
            const a = this.getAttribute(attr);
            if (a === null) return typeof def === "function" ? def() : def;
            try {
              return JSON.parse(a);
            } catch {
              return a.split(",").map((s) => s.trim()).filter(Boolean);
            }
          }
          const a = this.getAttribute(attr);
          if (type === "bool") return a !== null && a !== "false";
          if (a === null) return def;
          if (type === "number") {
            const n = Number(a);
            return Number.isFinite(n) ? n : def;
          }
          return a;
        },
        set(v) {
          if (type === "json") {
            (this._json ||= {})[name] = v;
            this._changed();
          } else if (type === "bool") {
            this.toggleAttribute(attr, !!v);
          } else if (v === null || v === undefined) {
            this.removeAttribute(attr);
          } else {
            this.setAttribute(attr, String(v));
          }
        },
      });
    }
    if (!customElements.get(tag)) customElements.define(tag, this);
    return this;
  }

  constructor() {
    super();
    this.attachShadow({ mode: "open", delegatesFocus: !!this.constructor.delegatesFocus });
    this.shadowRoot.adoptedStyleSheets = [sheet(BASE_CSS), sheet(this.constructor.css)];
    // A property set before the element upgraded lives on the instance; move it to the accessor.
    for (const name of Object.keys(this.constructor.props)) {
      if (Object.prototype.hasOwnProperty.call(this, name)) {
        const v = this[name];
        delete this[name];
        this[name] = v;
      }
    }
  }

  connectedCallback() {
    if (!this._built) {
      this._built = true;
      if (this.constructor.focusable && !this.hasAttribute("tabindex")) this.tabIndex = 0;
      this.setup();
    }
    this._unfollow = omarchyApi()._onBeforeChange(() => queueMicrotask(() => this._changed()));
    this.update();
  }

  disconnectedCallback() {
    if (this._unfollow) this._unfollow();
    this._unfollow = null;
  }

  attributeChangedCallback() {
    this._changed();
  }

  _changed() {
    if (this._built) this.update();
  }

  setup() {}
  update() {}

  /** `this.$("#id")`: a node in the shadow root. */
  $(sel) {
    return this.shadowRoot.querySelector(sel);
  }

  /** Dispatch a bubbling, composed CustomEvent. */
  emit(name, detail) {
    return this.dispatchEvent(new CustomEvent(name, { detail, bubbles: true, composed: true, cancelable: true }));
  }

  /** One of the kit's strings (common.cancel, …) in the display language. */
  t(key, fallback, ...args) {
    return omarchyApi().string(key, fallback, ...args);
  }
}

/** Escape text for innerHTML. */
export function esc(s) {
  return String(s ?? "").replace(/[&<>"']/g, (c) => ({ "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" })[c]);
}

/** Keys the kit reads like Qt's Util.readingKey: arrows, and h/j/k/l when `vim`. */
export function readingKey(e, vim = false) {
  if (e.ctrlKey || e.altKey || e.metaKey) return e.key;
  if (vim) {
    const m = { h: "ArrowLeft", j: "ArrowDown", k: "ArrowUp", l: "ArrowRight" }[e.key];
    if (m) return m;
  }
  return e.key;
}

/** Option helpers shared by choice components: an option is a string or { value, label, icon }. */
export const optionValue = (o) => (o && typeof o === "object" ? String(o.value) : String(o));
export const optionLabel = (o) => (o && typeof o === "object" ? String(o.label ?? o.value) : String(o));
export const optionIcon = (o) => (o && typeof o === "object" && o.icon ? String(o.icon) : "");

export { kebab, camel };
