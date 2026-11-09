// qt/gallery/sections/layout.qml: a SplitView framed by a track line (a Sidebar and a content
// pane), then a Calendar.
const sections = [
  { title: "Library", items: [
    { id: "all", label: "All files", glyph: "\u{F024B}" },
    { id: "recent", label: "Recent", glyph: "\u{F02DA}" },
    { id: "starred", label: "Starred", glyph: "\u{F04CE}" },
    { id: "trash", label: "Trash", glyph: "\u{F0A7A}" },
  ] },
  { title: "Places", items: [
    { id: "documents", label: "Documents", glyph: "\u{F0219}" },
    { id: "downloads", label: "Downloads", glyph: "\u{F01DA}" },
  ] },
];
const json = (v) => JSON.stringify(v).replace(/'/g, "&#39;");

export default {
  title: "Layout",
  html: `
<div style="position: relative; width: 720px; height: 220px">
  <omarchy-split-view style="position: absolute; inset: 0">
    <omarchy-sidebar preferred-width="200" minimum-width="120" sections='${json(sections)}' selected-id="recent"></omarchy-sidebar>
    <div fill-width style="display: flex; align-items: center; justify-content: center">
      <span class="text" style="color: var(--omarchy-text-faint)">No file selected</span>
    </div>
  </omarchy-split-view>
  <div style="position: absolute; inset: 0; pointer-events: none; border: var(--omarchy-space-hairline) solid var(--omarchy-track); border-radius: var(--omarchy-radius)"></div>
</div>
<omarchy-calendar year="2026" month="11" today="2026-11-08" selected="2026-11-14"></omarchy-calendar>`,
};
