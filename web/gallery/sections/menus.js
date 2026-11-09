// qt/gallery/sections/menus.qml: a menu and a context menu held open, a tooltip with its
// shortcut, keycaps and a combo, and a shortcut sheet.
const keys = ["Super", "Ctrl", "Alt", "Shift", "Tab", "Esc", "Enter", "Backspace", "Up", "Space", "K"];
const groups = [
  { title: "File", keys: [["Ctrl+N", "New file"], ["Ctrl+O", "Open"], ["Ctrl+S", "Save"]] },
  { title: "View", keys: [["Ctrl+B", "Sidebar"], ["Ctrl+Shift+P", "Commands"], ["F11", "Full screen"]] },
];

export default {
  title: "Menus",
  html: `
<div class="row top" style="gap: var(--omarchy-space-huge)">
  <omarchy-menu held>
    <omarchy-menu-section text="File"></omarchy-menu-section>
    <omarchy-menu-item text="New" icon="\u{F15B}" shortcut="Ctrl+N"></omarchy-menu-item>
    <omarchy-menu-item text="Open…" icon="\u{F07B}" shortcut="Ctrl+O" cursor></omarchy-menu-item>
    <omarchy-menu-item text="Save" icon="\u{F0C7}" shortcut="Ctrl+S"></omarchy-menu-item>
    <omarchy-menu-item text="Duplicate" icon="\u{F0C5}" disabled></omarchy-menu-item>
    <omarchy-menu-separator></omarchy-menu-separator>
    <omarchy-menu-section text="View"></omarchy-menu-section>
    <omarchy-menu-item text="Sidebar" checkable checked></omarchy-menu-item>
    <omarchy-menu-item text="Word wrap" checkable></omarchy-menu-item>
    <omarchy-menu-separator></omarchy-menu-separator>
    <omarchy-menu-item text="Export" icon="\u{F064}" has-submenu></omarchy-menu-item>
  </omarchy-menu>
  <omarchy-context-menu held>
    <omarchy-menu-item text="Cut" shortcut="Ctrl+X"></omarchy-menu-item>
    <omarchy-menu-item text="Copy" shortcut="Ctrl+C"></omarchy-menu-item>
    <omarchy-menu-item text="Paste" shortcut="Ctrl+V"></omarchy-menu-item>
    <omarchy-menu-separator></omarchy-menu-separator>
    <omarchy-menu-item text="By name" checkable exclusive checked></omarchy-menu-item>
    <omarchy-menu-item text="By date" checkable exclusive></omarchy-menu-item>
    <omarchy-menu-separator></omarchy-menu-separator>
    <omarchy-menu-item text="Delete" shortcut="Del" destructive></omarchy-menu-item>
  </omarchy-context-menu>
  <omarchy-tooltip held text="Save" shortcut="Ctrl+S"></omarchy-tooltip>
</div>
<div class="row" style="gap: var(--omarchy-space-lg)">
  ${keys.map((k) => `<omarchy-keycap key="${k}"></omarchy-keycap>`).join("\n  ")}
  <omarchy-key-combo keys="Ctrl+Shift+P"></omarchy-key-combo>
</div>
<omarchy-shortcut-sheet groups='${JSON.stringify(groups)}'></omarchy-shortcut-sheet>`,
};
