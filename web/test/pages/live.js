// The live-switch probe (spec/following.md): records the changed hook and every frame's computed
// background from the moment the hook fires, for test/live.mjs and test/electron-live.mjs.
import { omarchy, fading } from "../../src/index.js";

const root = document.documentElement;
const css = (name) => getComputedStyle(root).getPropertyValue(name).trim();
const probe = (window.__probe = { calls: [], samples: [], ready: false });

omarchy.onChange((tokens, state) => {
  probe.calls.push({
    at: performance.now(),
    revision: omarchy.revision,
    background: tokens.palette.background,
    theme: state.theme,
    // What the page reads inside the hook: the API's tokens, and an instant (non-color) property.
    apiBackground: omarchy.tokens.palette.background,
    radius: css("--omarchy-radius"),
    fadingInHook: fading(),
    computedInHook: css("--omarchy-background"),
  });
  const start = performance.now();
  const sample = () => {
    probe.samples.push({ t: performance.now() - start, bg: css("--omarchy-background"), fading: fading() });
    if (performance.now() - start < 900) requestAnimationFrame(sample);
  };
  sample();
});

const wait = () => (omarchy.state ? Promise.resolve() : new Promise((r) => setTimeout(() => r(wait()), 20)));
wait().then(() => {
  probe.start = { background: css("--omarchy-background"), revision: omarchy.revision, theme: omarchy.theme };
  probe.ready = true;
});
