// qt/gallery/sections/messages.qml
export default {
  title: "Messages",
  html: `
<div class="row" style="gap: var(--omarchy-space-huge)">
  <div class="col" style="width: 350px; height: 150px; justify-content: center; align-items: stretch">
    <omarchy-empty-state glyph="\u{f05aa}" title="No networks found" hint="Turn on Wi-Fi to scan" action-text="Scan"></omarchy-empty-state>
  </div>
  <div class="col" style="width: 350px; height: 150px; justify-content: center; align-items: stretch">
    <omarchy-empty-state size="overlay" glyph="\u{f014c}" title="Clipboard is empty" hint="Copy something to see it here"></omarchy-empty-state>
  </div>
</div>
<omarchy-banner class="fill" tone="info" text="Omarchy 3.2 is available" action-text="Update" dismissible></omarchy-banner>
<omarchy-banner class="fill" tone="success" text="Backup finished" dismissible></omarchy-banner>
<omarchy-banner class="fill" tone="warning" text="Battery is low (12%)" dismissible></omarchy-banner>
<omarchy-banner class="fill" tone="error" text="Could not reach the update server" dismissible></omarchy-banner>
<omarchy-toast glyph="\u{f012c}" title="Export finished" text="omarchy-demo.mp4 · 42 MB" action-text="Open" countdown="0.6"></omarchy-toast>`,
};
