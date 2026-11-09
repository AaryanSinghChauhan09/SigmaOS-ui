// The Tauri bridge probe (web/test/tauri.mjs): what the page reads on its first frame, and every
// changed hook.
import { omarchy, fading } from "../../src/index.js";

const root = document.documentElement;
const probe = (window.__probe = { calls: 0, first: null });
omarchy.onChange(() => (probe.calls += 1));
requestAnimationFrame(() => {
  probe.first = {
    body: getComputedStyle(document.body).backgroundColor,
    variable: getComputedStyle(root).getPropertyValue("--omarchy-background").trim(),
    fadeAttribute: root.hasAttribute("data-omarchy-fade"),
    fading: fading(),
    mode: root.dataset.omarchyMode,
  };
});
