// The tiles section (spec/components/lists.md, Gallery; qt/gallery/sections/tiles.qml).
const g = (cp) => String.fromCodePoint(cp);
// The sample picture, shared with the Qt and Rune galleries.
const photo = new URL("../assets/avatar.png", import.meta.url).href;
const missing = new URL("../assets/missing.png", import.meta.url).href;
const ground = `ground="var(--omarchy-background)"`;

export default {
  title: "Tiles",
  html: `
<div class="row" style="gap: var(--omarchy-space-control-gap)">
  <omarchy-badge text="New"></omarchy-badge>
  <omarchy-badge text="3" tone="accent" filled></omarchy-badge>
  <omarchy-badge text="12" tone="urgent" filled></omarchy-badge>
  <omarchy-badge text="Online" tone="success"></omarchy-badge>
  <omarchy-badge text="Low" tone="warning"></omarchy-badge>
  <omarchy-badge text="Offline" tone="urgent"></omarchy-badge>
</div>
<div class="row" style="gap: var(--omarchy-space-xxl)">
  <omarchy-status-dot tone="success"></omarchy-status-dot>
  <omarchy-status-dot tone="warning"></omarchy-status-dot>
  <omarchy-status-dot tone="urgent"></omarchy-status-dot>
  <omarchy-status-dot tone="muted"></omarchy-status-dot>
  <omarchy-status-dot tone="accent"></omarchy-status-dot>
  <omarchy-avatar name="Ryan" round status="success" ${ground}></omarchy-avatar>
  <omarchy-avatar name="Photo" source="${photo}" status="warning" ${ground}></omarchy-avatar>
  <omarchy-avatar name="Server" glyph="${g(0xf048b)}" status="urgent" ${ground}></omarchy-avatar>
</div>
<div class="row" style="gap: var(--omarchy-space-xxl)">
  <omarchy-avatar name="ada" size="small"></omarchy-avatar>
  <omarchy-avatar name="Basecamp"></omarchy-avatar>
  <omarchy-avatar name="Omarchy" size="large" round></omarchy-avatar>
  <omarchy-avatar name="Robot" glyph="${g(0xf06a9)}"></omarchy-avatar>
  <omarchy-avatar name="Photo" source="${photo}" size="small"></omarchy-avatar>
  <omarchy-avatar name="Photo" source="${photo}" round></omarchy-avatar>
  <omarchy-avatar name="Photo" source="${photo}" size="large"></omarchy-avatar>
  <omarchy-avatar name="Missing" source="${missing}"></omarchy-avatar>
</div>
<div class="row" style="gap: var(--omarchy-space-xxl)">
  <omarchy-avatar name="Files" icon-name="org.gnome.Nautilus"></omarchy-avatar>
  <omarchy-avatar name="Terminal" icon-name="utilities-terminal"></omarchy-avatar>
  <omarchy-avatar name="Firefox" icon-name="firefox"></omarchy-avatar>
  <omarchy-avatar name="Chromium" icon-name="chromium" size="small"></omarchy-avatar>
  <omarchy-avatar name="Steam" icon-name="steam" size="large"></omarchy-avatar>
  <omarchy-avatar name="Btop" icon-name="btop" size="small"></omarchy-avatar>
  <omarchy-avatar name="Nothing" icon-name="no-such-app" glyph="${g(0xf120)}"></omarchy-avatar>
</div>
<omarchy-tile-grid columns="4" cursor-index="2"></omarchy-tile-grid>`,

  setup(body) {
    body.querySelector("omarchy-tile-grid").tiles = [
      { glyph: g(0xf120), label: "Terminal" },
      { glyph: g(0xf07b), label: "Files" },
      { glyph: g(0xf0ac), label: "Browser" },
      { glyph: g(0xf001), label: "Music" },
      { glyph: g(0xf013), label: "Settings" },
      { glyph: g(0xf073), label: "Calendar" },
      { glyph: g(0xf03d), label: "Screen Recorder" },
      { name: "Photos", source: photo, label: "Photos" },
    ];
  },
};
