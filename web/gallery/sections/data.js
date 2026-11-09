// qt/gallery/sections/data.qml: a Table and a Tree side by side.
const columns = [{ title: "Name" }, { title: "Kind", width: 90 }, { title: "Size", width: 70, align: "right" }, { title: "Modified", width: 100 }];
const rows = [
  ["assets", "Folder", "—", "2026-11-02"],
  ["Cargo.toml", "TOML", "1.2 KB", "2026-11-07"],
  ["README.md", "Markdown", "4.8 KB", "2026-11-05"],
  ["screenshot-of-the-new-settings-panel.png", "PNG image", "1.4 MB", "2026-11-08"],
  ["src", "Folder", "—", "2026-11-08"],
  ["theme.toml", "TOML", "812 B", "2026-10-30"],
];
const nodes = [
  { id: "root", label: "omarchy-ui", glyph: "\u{F0770}", children: [
    { id: "qt", label: "qt", glyph: "\u{F0770}", children: [
      { id: "qt/button", label: "Button.qml", glyph: "\u{F0219}" },
      { id: "qt/table", label: "Table.qml", glyph: "\u{F0219}" },
    ] },
    { id: "rune", label: "rune", glyph: "\u{F024B}", children: [{ id: "rune/lib", label: "lib.rs", glyph: "\u{F0219}" }] },
    { id: "spec", label: "spec", glyph: "\u{F0770}", children: [{ id: "spec/tokens", label: "tokens.md", glyph: "\u{F0219}" }] },
    { id: "readme", label: "README.md", glyph: "\u{F0219}" },
  ] },
];
const json = (v) => JSON.stringify(v).replace(/'/g, "&#39;");

export default {
  title: "Data",
  html: `
<div class="row top" style="gap: var(--omarchy-space-huge)">
  <omarchy-table style="width: 480px" columns='${json(columns)}' rows='${json(rows)}' sort-column="0" selected-row="1" cursor-row="3" cursor></omarchy-table>
  <omarchy-tree style="width: 220px" nodes='${json(nodes)}' expanded='["root","qt","spec"]' selected-id="qt/table" cursor-id="readme" cursor></omarchy-tree>
</div>`,
};
