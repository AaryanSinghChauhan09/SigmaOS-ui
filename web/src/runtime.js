// Puts the core's tokens into the page: the custom properties of tokens.js on :root (in a
// stylesheet layer of their own, so the page's own CSS can still override them), their colors
// registered with @property so a theme switch fades them (spec/following.md), and the transport
// that delivers them (the Electron preload, a harness, Server-Sent Events, or a snapshot).

import { createOmarchy } from "./api.js";
import { tokensToCss } from "./tokens.js";

const registered = new Set();
const sheets = { tokens: null };
let api = null;
let lastColors = [];

const reducedByMedia = () =>
  typeof matchMedia === "function" && matchMedia("(prefers-reduced-motion: reduce)").matches;

function register(colors) {
  if (typeof CSS === "undefined" || !CSS.registerProperty) return;
  for (const name of colors) {
    if (registered.has(name)) continue;
    registered.add(name);
    try {
      CSS.registerProperty({ name, syntax: "<color>", inherits: true, initialValue: "transparent" });
    } catch {
      // Registered already (another copy of the kit): the transition still works.
    }
  }
}

function adopt(sheet) {
  if (!document.adoptedStyleSheets.includes(sheet)) {
    document.adoptedStyleSheets = [...document.adoptedStyleSheets, sheet];
  }
}

// Line heights as Qt lays text out (a Text's height is ceil(size × (ascent + descent)), where
// Chromium's line-height: normal rounds ascent and descent apart, a pixel short at most sizes), from
// the resolved font's own metrics. Measured once per family; the sizes are the core's.
const metrics = new Map();
function fontRatio(family) {
  if (metrics.has(family)) return metrics.get(family);
  let ratio = 0;
  try {
    const c = document.createElement("canvas").getContext("2d");
    c.font = `1000px ${family}`;
    const m = c.measureText("Ag");
    ratio = (m.fontBoundingBoxAscent + m.fontBoundingBoxDescent) / 1000;
  } catch {
    ratio = 0;
  }
  if (ratio > 0) metrics.set(family, ratio);
  return ratio;
}

function lineHeights(props) {
  const ratio = fontRatio(props["--omarchy-font-family"]);
  if (!ratio) return [];
  const out = [];
  for (const [k, v] of Object.entries(props)) {
    const m = /^--omarchy-font-(.+)$/.exec(k);
    if (!m || m[1] === "family" || m[1] === "base-size" || !v.endsWith("px")) continue;
    out.push(`--omarchy-line-${m[1]}: ${Math.ceil(parseFloat(v) * ratio - 1e-6)}px;`);
  }
  return out;
}

/** The tokens' rule: every custom property on :root, and the theme-switch fade on its colors. */
function tokenRule(tokens) {
  const { props, colors } = tokensToCss(tokens);
  register(colors);
  lastColors = colors;
  const decls = [...Object.entries(props).map(([k, v]) => `${k}: ${v};`), ...lineHeights(props)].join("\n    ");
  return `@layer omarchy.tokens {
  :root {
    color-scheme: ${tokens.mode === "light" ? "light" : "dark"};
    ${decls}
  }
  :root[data-omarchy-fade] {
    transition-property: ${colors.join(", ")};
    transition-duration: var(--omarchy-motion-theme);
    transition-timing-function: var(--omarchy-ease);
  }
}`;
}

/** Apply one state: the first one (and any with reduced motion) lands at once; a change fades. */
export function applyState(state, isChange) {
  const tokens = state.tokens;
  const root = document.documentElement;
  if (!sheets.tokens) sheets.tokens = new CSSStyleSheet();
  const fade = !!isChange && !tokens.reduceMotion && !reducedByMedia();
  // The attribute and the new values land in the same style update, so the transition (or its
  // absence, which also stops a fade in progress) applies to exactly this change.
  root.toggleAttribute("data-omarchy-fade", fade);
  root.toggleAttribute("data-omarchy-reduce-motion", !!tokens.reduceMotion);
  root.dataset.omarchyMode = tokens.mode === "light" ? "light" : "dark";
  sheets.tokens.replaceSync(tokenRule(tokens));
  adopt(sheets.tokens);
}

/** Whether a theme-switch fade is running now. */
export function fading() {
  const set = new Set(lastColors);
  return document.documentElement
    .getAnimations()
    .some((a) => typeof CSSTransition !== "undefined" && a instanceof CSSTransition && set.has(a.transitionProperty));
}

function fromBridge(bridge) {
  const o = createOmarchy();
  if (bridge.icons) o._setIcons((size, names) => bridge.icons(size, names));
  o._push(bridge.initial || (bridge.state && bridge.state()));
  if (bridge.subscribe) bridge.subscribe((s) => o._push(s));
  return o;
}

/** The page's `omarchy`, connected to whatever delivers the state, and applying it. */
export function omarchyApi() {
  if (api) return api;
  const g = globalThis;
  if (g.omarchy && typeof g.omarchy._onBeforeChange === "function") {
    api = g.omarchy; // the Electron preload's
  } else if (g.omarchyBridge) {
    api = fromBridge(g.omarchyBridge); // a harness (the gallery, tests) or another shell
  } else {
    api = createOmarchy();
  }
  api._onBeforeChange((state, isChange) => applyState(state, isChange));
  if (api.state) applyState(api.state, false);
  if (!g.omarchy) {
    try {
      Object.defineProperty(g, "omarchy", { value: api, configurable: true });
    } catch {
      // A page that defined its own `omarchy`: import { omarchy } instead.
    }
  }
  if (!api.state) autoConnect(api);
  return api;
}

/** Feed a state yourself: a snapshot (`omarchy-theme --state`), or each state of a transport of
 *  your own. The first is the initial load; each later one that differs is a change. */
export function setOmarchyState(state) {
  return omarchyApi()._push(typeof state === "string" ? JSON.parse(state) : state);
}

/** Follow the dev helper's Server-Sent Events (web/dev/serve.js): the plain-browser fallback. */
export function connectOmarchy(url = "/omarchy/events") {
  const o = omarchyApi();
  const base = new URL(url, location.href);
  o._setIcons(async (size, names) => {
    const u = new URL("icons", base.href.replace(/[^/]*$/, ""));
    u.searchParams.set("size", String(size));
    u.searchParams.set("names", names.join(","));
    const r = await fetch(u);
    return r.ok ? r.json() : {};
  });
  const events = new EventSource(base.href);
  events.onmessage = (e) => o._push(JSON.parse(e.data));
  return () => events.close();
}

function autoConnect(o) {
  if (typeof document === "undefined") return;
  // A snapshot in the page: <script type="application/json" id="omarchy-state">{…}</script>
  const snap = document.getElementById("omarchy-state");
  if (snap && snap.textContent.trim()) {
    o._push(JSON.parse(snap.textContent));
    return;
  }
  // The dev helper: <meta name="omarchy-events" content="/omarchy/events">
  const meta = document.querySelector('meta[name="omarchy-events"]');
  if (meta) connectOmarchy(meta.content || "/omarchy/events");
}
