// qt/gallery/sections/timeline.qml: a Timeline, a Scrubber, rows reordered by their grips with a
// drop slot, and a crop frame over a rectangle.
const tracks = [
  [{ start: 0, end: 7.5, label: "Intro" }, { start: 8, end: 18, label: "Demo" }, { start: 18.5, end: 30, label: "Outro" }],
  [{ start: 2, end: 12, label: "Voiceover" }, { start: 14, end: 26, label: "Music" }],
];
const rows = ["Workspaces", "Clock", "Battery", "Network"];

export default {
  title: "Timeline",
  html: `
<style>
  .tl-rows { position: relative; width: 360px; }
  .tl-row { display: flex; align-items: center; gap: var(--omarchy-space-sm); height: var(--omarchy-space-popup-row-height);
    padding-left: var(--omarchy-space-sm); box-sizing: border-box; border-radius: var(--omarchy-radius); }
  .tl-row.cur { background: var(--omarchy-control-hover-fill); }
  .tl-rows omarchy-drop-indicator { position: absolute; left: 0; width: 100%;
    top: calc(var(--omarchy-space-popup-row-height) * 3 - round(calc(max(1px, round(calc(2px * var(--omarchy-space-scale)), 1px)) / 2), 1px)); }
  .tl-crop { width: 240px; height: 140px; background: var(--omarchy-control-normal-fill); }
  .tl-crop omarchy-crop-frame { width: 100%; height: 100%; }
</style>
<omarchy-timeline style="width: 720px; height: 160px" duration="30" position="12" selected-track="0" selected-clip="1" tracks='${JSON.stringify(tracks)}'></omarchy-timeline>
<omarchy-scrubber style="width: 360px" position="42" duration="130"></omarchy-scrubber>
<div class="row top" style="gap: var(--omarchy-space-huge)">
  <div class="tl-rows">
    ${rows.map((r, i) => `<div class="tl-row${i === 1 ? " cur" : ""}"><omarchy-drag-handle${i === 1 ? " cursor" : ""}></omarchy-drag-handle><span class="text">${r}</span></div>`).join("")}
    <omarchy-drop-indicator></omarchy-drop-indicator>
  </div>
  <div class="tl-crop"><omarchy-crop-frame></omarchy-crop-frame></div>
</div>`,
};
