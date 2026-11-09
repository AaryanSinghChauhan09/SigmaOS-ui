// The checks of spec/following.md's live-switch test, over what test/pages/live.js recorded. Shared
// by the browser test (live.mjs) and the Electron one (electron-live.mjs).

export const FROM = "#1a1b26"; // tokyo-night's background
export const TO = "#eff1f5"; // catppuccin-latte's

/** "rgb(239, 241, 245)" / "#eff1f5ff" → [r, g, b, a(0-255)] */
export function rgba(text) {
  const s = String(text).trim();
  let m = /^#([0-9a-f]{6})([0-9a-f]{2})?$/i.exec(s);
  if (m) {
    const n = parseInt(m[1], 16);
    return [(n >> 16) & 255, (n >> 8) & 255, n & 255, m[2] ? parseInt(m[2], 16) : 255];
  }
  m = /^rgba?\(([^)]+)\)$/.exec(s);
  if (m) {
    const p = m[1].split(/[\s,/]+/).filter(Boolean).map(Number);
    return [p[0], p[1], p[2], p.length > 3 ? Math.round(p[3] * 255) : 255];
  }
  return null;
}

const same = (a, b) => {
  const x = rgba(a), y = rgba(b);
  return !!x && !!y && x.every((v, i) => v === y[i]);
};
const between = (c, a, b) => {
  const x = rgba(c), p = rgba(a), q = rgba(b);
  if (!x || !p || !q) return false;
  if (same(c, a) || same(c, b)) return false;
  return x.slice(0, 3).every((v, i) => v >= Math.min(p[i], q[i]) && v <= Math.max(p[i], q[i]));
};

/** Failures (empty when it passed) and a summary line. */
export function check(probe, { reduced }) {
  const fail = [];
  if (!same(probe.start.background, FROM)) fail.push(`started on ${probe.start.background}, not tokyo-night's ${FROM}`);
  if (probe.start.revision !== 0) fail.push(`revision ${probe.start.revision} at start, not 0`);
  if (probe.calls.length !== 1) fail.push(`the changed hook fired ${probe.calls.length} times, not once`);
  const call = probe.calls[0];
  if (call) {
    if (call.background !== "#eff1f5ff") fail.push(`the hook got palette.background ${call.background}, not #eff1f5ff`);
    if (call.apiBackground !== "#eff1f5ff") fail.push(`omarchy.tokens read ${call.apiBackground} in the hook`);
    if (call.revision !== 1) fail.push(`revision ${call.revision} in the hook, not 1`);
    if (call.radius !== "0px") fail.push(`--omarchy-radius read ${call.radius} in the hook`);
  }
  if (probe.revision !== 1) fail.push(`revision ${probe.revision} at the end, not 1`);
  const last = probe.samples[probe.samples.length - 1];
  if (!last || !same(last.bg, TO)) fail.push(`--omarchy-background ended at ${last && last.bg}, not ${TO}`);
  const mid = probe.samples.filter((s) => between(s.bg, FROM, TO));
  const fadingSeen = probe.samples.some((s) => s.fading);
  if (reduced) {
    if (mid.length) fail.push(`faded under reduced motion (${mid.length} frames between, e.g. ${mid[0].bg})`);
    if (probe.samples[0] && !same(probe.samples[0].bg, TO)) fail.push(`the first frame after the hook read ${probe.samples[0].bg}`);
  } else {
    if (!mid.length) fail.push("no frame between the two backgrounds: it didn't fade");
    if (!fadingSeen) fail.push("fading() never said it was fading");
    const landed = probe.samples.find((s) => same(s.bg, TO));
    if (landed && landed.t < 200) fail.push(`landed after ${Math.round(landed.t)} ms, sooner than motion.theme`);
  }
  const landed = probe.samples.find((s) => same(s.bg, TO));
  const summary = reduced
    ? `hook once, revision 1, background ${TO} at once (reduced motion)`
    : `hook once, revision 1, faded over ${mid.length} frames to ${TO} in ${landed ? Math.round(landed.t) : "?"} ms`;
  return { fail, summary };
}
