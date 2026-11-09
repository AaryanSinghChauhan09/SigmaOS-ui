// @omarchy/ui: import it in a renderer and the page follows the Omarchy theme.
//
//   import "@omarchy/ui";                         // or: import { omarchy } from "@omarchy/ui";
//
// On import it connects to whatever delivers the core's state (the Electron preload, or Tauri's
// tauri-plugin-omarchy; a harness's window.omarchyBridge; a <script type="application/json"
// id="omarchy-state"> snapshot; or the dev helper's events, <meta name="omarchy-events"
// content="/omarchy/events">), puts the tokens on :root
// as --omarchy-* custom properties, restyles plain HTML controls (src/restyle.css) and defines every
// <omarchy-*> element. A theme switch fades the colors and then runs omarchy.onChange hooks.

import { omarchyApi, applyState, connectOmarchy, setOmarchyState, fading } from "./runtime.js";
import { restyleSheet, restyleCss } from "./restyle.js";
import { defineElements } from "./elements/index.js";

/** The page's omarchy: tokens, revision, onChange, string, icon(s). Also `window.omarchy`. */
export const omarchy = omarchyApi();

const skipRestyle =
  typeof document !== "undefined" &&
  document.querySelector('meta[name="omarchy-ui"][content~="no-restyle"]');
if (!skipRestyle) {
  // Before the tokens' sheet, in its own layer: the page's own rules win over both.
  const restyle = restyleSheet();
  if (!document.adoptedStyleSheets.includes(restyle)) document.adoptedStyleSheets = [restyle, ...document.adoptedStyleSheets];
}

defineElements();

export { applyState, connectOmarchy, setOmarchyState, fading, defineElements, restyleCss };
export { tokensToCss, tokensToCssText, borderGradient } from "./tokens.js";
export * from "./elements/index.js";
