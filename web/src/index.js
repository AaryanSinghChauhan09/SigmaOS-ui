// @omarchy/ui: import it in a renderer and the page follows the Omarchy theme.
//
//   import "@omarchy/ui";                         // or: import { omarchy } from "@omarchy/ui";
//
// On import it connects to whatever delivers the core's state (the Electron preload; a harness's
// window.omarchyBridge; a <script type="application/json" id="omarchy-state"> snapshot; or the dev
// helper's events, <meta name="omarchy-events" content="/omarchy/events">), puts the tokens on :root
// as --omarchy-* custom properties, restyles plain HTML controls (src/restyle.css) and defines every
// <omarchy-*> element. A theme switch fades the colors and then runs omarchy.onChange hooks.

import { omarchyApi, applyState, connectOmarchy, setOmarchyState, fading } from "./runtime.js";
import restyle from "./restyle.css" with { type: "css" };
import { defineElements } from "./elements/index.js";

/** The page's omarchy: tokens, revision, onChange, string, icon(s). Also `window.omarchy`. */
export const omarchy = omarchyApi();

const skipRestyle =
  typeof document !== "undefined" &&
  document.querySelector('meta[name="omarchy-ui"][content~="no-restyle"]');
if (!skipRestyle && !document.adoptedStyleSheets.includes(restyle)) {
  document.adoptedStyleSheets = [...document.adoptedStyleSheets, restyle];
}

defineElements();

export { applyState, connectOmarchy, setOmarchyState, fading, defineElements };
export { tokensToCss, tokensToCssText, borderGradient } from "./tokens.js";
export * from "./elements/index.js";
